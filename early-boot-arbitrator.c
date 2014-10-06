#include <sys/mount.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "unknown_proto.h"
#include "message.h"
#include "notification.h"
#include "ping_pong.h"

struct waiter; 

struct item
{
    struct item *next;
    unsigned char id[16];
    int request_count;
    struct waiter * request_queue;
    int datalen;
    unsigned char * data;
};

struct waiter
{
    struct waiter * next;
    struct request * r;
};

struct item * resources = 0;

struct item * create_item(struct request *);
struct item * find_item(unsigned char[16]);
void queue_request(struct item *, struct request *);
void send_reply(struct item *, struct request *);
struct reply_buf * handle_create_request(struct request *);
struct reply_buf * handle_read_request(struct request *); 
struct reply_buf * handle_update_request(struct request *);
struct reply_buf * handle_delete_request(struct request *); 
struct reply_buf * notify_waiters(struct item *);
int send_error_reply(struct request *);
struct reply_buf * process_payload(unsigned char *, unsigned int);
struct reply_buf * invalid_op_reply(struct request *);
struct reply_buf * create_reply(struct request *, void *, unsigned int);
struct item * find_prev_item(unsigned char[16]);
int update_item(struct item *, unsigned char *, unsigned int);
void add_data(struct item *, struct request *);
struct reply_buf * process_ping_payload(unsigned char *, unsigned int);
struct reply_buf * process_notification_payload(unsigned char *, unsigned int);

static const unsigned int data_buf_size = 4096;

int sig_num = 0;

void
signal_handler(int num)
{
    sig_num = num;
}

int
main(void)
{
    unsigned char running;
    int ret, endp, bytes_read = 0;
    unsigned char * data;
    struct message * m;
    struct sigaction sig;
    fprintf(stderr, "early-boot-arbitrator starting...\n");

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_handler = signal_handler;
    sigaction(SIGTERM, &sig, 0);

    resources = malloc(sizeof(*resources));
    memset(resources, 0, sizeof(*resources));
    resources->next = resources; 

    running = 1;
    endp = create_endpoint("/run/early-boot-arbit");
    if (endp < 0)
    {
        data = malloc(data_buf_size);
        m = (struct message *)data;
        fprintf(stderr, "early-boot-arbitrator ready...\n");  
        while(running)
        {
            ret = recvfrom(endp, data + bytes_read, data_buf_size - bytes_read, 0, 0, 0);
            if (ret > 0)
            {
                printf("received %d bytes\n", ret);
                bytes_read += ret;
                while (bytes_read >= sizeof(*m))
                {
                    if (!memcmp(m->protocol_id, MESSAGE_PROTO_ID, MP_ID_LEN))
                    {   
                        if (bytes_read >= (m->payload_len + sizeof(*m)))
                        {
                            struct reply_buf * reply = process_payload(m->payload, m->payload_len);
                            if (reply)
                            {
                                if (reply->next)
                                {
                                    struct reply_buf * spent_reply, * next_reply = reply->next;
                                    while(reply != next_reply)
                                    {
                                        send_message(endp, next_reply->reply_dest, MP_ID_LEN, next_reply->data, next_reply->data_len);    
                                        spent_reply = next_reply; 
                                        next_reply = next_reply->next;
                                        free(spent_reply); 
                                    }
                                    free(reply);
                                }
                                else
                                {
                                    send_message(endp, reply->reply_dest, MP_ID_LEN, reply->data, reply->data_len);
                                    free(reply);
                               }
                            } 
                        }
                        bytes_read -= (sizeof(*m) + m->payload_len);
                        if (bytes_read > 0)
                        {
                            int message_len = sizeof(*m) + m->payload_len;
                            memmove(data, data + message_len,  message_len);
                        }
                    }
                    else
                    {
                        bytes_read = 0;
                    }
                } 
            }
            else
            {
                if (ret < 0)
                {
                    if (EINTR == errno)
                    {
                        if (SIGTERM == sig_num)
                        {
                            running = 0;
                        }
                    }
                    else 
                        fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
                }
            }
        }
        unlink("/run/early-boot-arbit");
        close(endp);
    } 
}

struct reply_buf *
process_payload(unsigned char * d, unsigned int len)
{
    struct unknown_request * r;

    r = (struct unknown_request *)d;
    if (len < sizeof(*r)) return 0; 
    if (0 == memcmp(r->protocol_id, NOTIFICATION_PROTO_ID, MP_ID_LEN))
        return process_notification_payload(d, len);
    else if (0 == memcmp(r->protocol_id, PING_PROTO_ID, MP_ID_LEN))
        return process_ping_payload(d, len);
    else
        return 0;
}

struct reply_buf *
process_ping_payload(unsigned char * d, unsigned int len)
{
     struct reply_buf * reply;
     struct ping_pong_request * ping, * pong;
     ping = (struct ping_pong_request *)d;
     reply = malloc(sizeof(*reply) + sizeof(struct ping_pong_request));
     reply->data_len = sizeof(struct ping_pong_request);
     pong = (struct ping_pong_request *)&reply->data;
     memcpy(pong->protocol_id, PONG_PROTO_ID, MP_ID_LEN);
     memset(&pong->sender_endp, 0, MP_ID_LEN);
     memcpy(reply->reply_dest, ping->sender_endp, MP_ID_LEN);
     return reply;            
}

struct reply_buf *
process_notification_payload(unsigned char * d, unsigned int len)
{
    struct request * r;

    r = (struct request *)d;
    if (len < sizeof(*r)) return 0; 
    if (READ == r->op)
    {   
        return handle_read_request(r);
    }
    else if (CREATE == r->op)
    {
        return handle_create_request(r);
    }
    else if (UPDATE == r->op)
    {
        return handle_update_request(r);
    }
    else if (DELETE == r->op)
    {
        return handle_delete_request(r);
    }
    else
    {
        return invalid_op_reply(r);
    }
}  

struct reply_buf *
invalid_op_reply(struct request * r)
{
    int retcode = -1;

    return create_reply(r, &retcode, sizeof(retcode));
}

struct reply_buf *
ok_reply(struct request * r)
{
    int retcode = 0;

    return create_reply(r, &retcode, sizeof(retcode));
}

struct reply_buf *
item_not_found_reply(struct request * r)
{
    int retcode = -2;

    return create_reply(r, &retcode, sizeof(retcode));
}

struct reply_buf *
item_exists_reply(struct request * r)
{
    int retcode = -3;

    return create_reply(r, &retcode, sizeof(retcode));
}

struct reply_buf *
handle_delete_request(struct request * r)
{
    struct item * i;

    i = find_prev_item(r->resource_id);
    if (i)
    {
        struct item * old_i = i->next;
        i->next = old_i->next;
        if (old_i->request_queue)
        {
            struct waiter * q, * f;
            q = i->request_queue->next;
            while(q!=i->request_queue)
            {
                f = q;
                q = q->next;
                free(f->r);
                free(f);        
            }   
            free(i->request_queue);
        }
        free(old_i->data);
        free(old_i);
        return ok_reply(r);
    }
    else
    {
        return item_not_found_reply(r);
    }
}

struct reply_buf *
create_reply(struct request * r, void * d, unsigned int len)
{
    struct reply_buf * reply;

    reply = malloc(sizeof(*reply) + len);
    memset(reply, 0, sizeof(*reply));
    memcpy(reply->reply_dest, r->sender_endp, MP_ID_LEN);
    if (len)
    {   
        memcpy(reply->data, d, len);
        reply->data_len = len;
    }
    return reply;
}

struct reply_buf *
handle_update_request(struct request * r)
{
   struct item * i;

    i = find_item(r->resource_id);
    if (i)
    {
        update_item(i, r->data, r->data_len);
        return ok_reply(r);
    }
    else
    {
        return item_not_found_reply(r);
    }
}

int
update_item(struct item * i, unsigned char * data, unsigned int len)
{

    free(i->data);
    i->data = malloc(len);
    memcpy(i->data, data, len);
    i->datalen = len;
    return 0;
}

struct reply_buf *
handle_create_request(struct request * r)
{
    struct item * i;

    i = find_item(r->resource_id);
    if (0 == i) 
    {
        struct item * n_i = create_item(r);
        add_data(n_i, r); //only the owner of the item can update 
        return ok_reply(r);
    }
    else if (i->datalen == 0)
    {
        struct reply_buf * rep, * w_r;
        add_data(i, r);
        rep = ok_reply(r);
        w_r = notify_waiters(i);
        rep->next = w_r->next;
        w_r->next = rep;
        return w_r;
    }
    else
    {
        return item_exists_reply(r);
    }
}

struct reply_buf *
handle_read_request(struct request * r) 
{
    struct item * i;
    i = find_item(r->resource_id);
    if (0 == i)
    {
        struct item * new_item = create_item(r);
        queue_request(new_item, r);
        return 0;
    }
    else if (i->datalen)
    {
        return create_reply(r, i->data, i->datalen);
    }  
    else
    {
        queue_request(i, r);
        return 0;
    } 
}

struct item *
create_item(struct request * r)
{
    struct item * i;

    i = malloc(sizeof(*i));
    memset(i, 0, sizeof(*i));
    memcpy(i->id, r->resource_id, 16);
    i->request_queue = malloc(sizeof(struct waiter));
    memset(i->request_queue, 0, sizeof(struct waiter));
    i->request_queue->next = i->request_queue;
    i->next = resources->next;
    resources->next = i;
    return i;
}

void
add_data(struct item * i, struct request * r)
{
    if (r->data_len)
    {
        i->data = malloc(r->data_len);
        memcpy(i->data, r->data, r->data_len);
        i->datalen = r->data_len; 
    }
}

void
queue_request(struct item * i, struct request * r)
{

    struct waiter * new_req;
   
    new_req = malloc(sizeof(struct waiter));
    memset(new_req, 0, sizeof(struct waiter));
    new_req->r = malloc(sizeof(*r) + r->data_len); 
    memcpy(new_req->r, r, sizeof(*r) + r->data_len);

    new_req->next = i->request_queue->next;
    i->request_queue->next = new_req;
}

struct reply_buf *
notify_waiters(struct item * i)
{
    if (i)
    {
        if (i->request_queue->next != i->request_queue)
        {
            struct reply_buf * r;
            r = malloc(sizeof(*r));
            memset(r, 0, sizeof(*r));
            r->next = r;
            struct waiter * s, * w = i->request_queue->next;
            while(w != i->request_queue)
            {
                struct reply_buf * n_r = create_reply(w->r, i->data, i->datalen);
                n_r->next = r->next;
                r->next = n_r; 
                s = w;
                w = w->next;
                free(s->r);
                free(s);
            }
            free(i->request_queue);
            i->request_queue = 0; 
            return r; 
        }
    }
    return 0;
}

struct item *
find_item(unsigned char id[16])
{
    struct item * i;

    memcpy(resources->id, id, 16);
    for(i = resources->next; memcmp(i->id, id, 16); 
        i = i->next);
    memset(resources->id, 0, 16);
    if (i == resources) return 0; 
    else return i; 
}

struct item *
find_prev_item(unsigned char id[16])
{
    struct item * i;

    memcpy(resources->id, id, 16);
    for(i = resources->next; memcmp(i->next->id, id, 16); 
        i = i->next);
    memset(resources->id, 0, 16);
    if (i->next == resources) return 0; 
    else return i; 
}
