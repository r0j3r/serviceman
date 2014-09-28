#include <sys/mount.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "notification.h"

void handle_create_request(struct request *);
void handle_read_request(struct request *); 
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
    int namelen; 
    unsigned char * endpoint;
};

struct item * resources;

struct item * create_item(struct request *);
struct item * find_item(unsigned char id[16]);
void queue_request(struct item *, struct request *);


int
main()
{
    unsigned char running;
    int ret, endp, bytes_read = 0;
    char data[1024];
    struct request * request;

    request = (struct request *)data;  
    mount("none", "/run", "tmpfs", 0, 0);
 
    resources = malloc(sizeof(*resources));
    memset(resources, 0, sizeof(*resources));
    resources->next = resources; 

    running = 1;
    endp = create_endpoint("/run/early-boot-arbitrator");
    while(running)
    {
        ret = read(endp, data + bytes_read, sizeof(data) - bytes_read);
        if (ret > 0)
        {
            while (bytes_read > sizeof(request))
            {
                if (bytes_read > (request->payloadlen + sizeof(request)))
                {
                    if (request->op == CREATE)
                    {
                        handle_create_request(request);
                    }
                    else if (request->op == READ)
                    {
                        handle_read_request(request);
                    }
                    bytes_read -= (sizeof(request) + request->payloadlen);
                    if (bytes_read > 0)
                    {
                        memmove(data, data + bytes_read, sizeof(data) - bytes_read);
                    } 
                }   
            } 
        }
    } 
}

void
handle_create_request(struct request * r)
{
    struct item * i;

    i = find_item(r->resource_id);
    if (i == resources) 
    {
        create_item(r);
    }
    else if (i->datalen == 0)
    {
        i->datalen = r->payloadlen;
        memcpy(i->data, r->payload, r->payloadlen);
        notify_waiters(i); 
    }
    else
    {
        send_error_reply(r);
    }
}

void
handle_read_request(struct request * r) 
{
    struct item * i;
    memcpy(resources->id, r->resource_id, 16);
    for(i = resources->next; memcmp(i->id, r->resource_id, 16); 
        i = i->next)
    if (i == resources)
    {
        struct item * new_item = create_item(r);
        queue_request(new_item, r);
    }
    else if (i->datalen)
    {
        send_reply(i, r);
    }  
    else
    {
        queue_request(i, r);
    } 
}

struct item *
create_item(struct request * r)
{
    struct item * i;

    i = malloc(sizeof(*i));
    memset(i, 0, sizeof(*i));
    memcpy(i->id, r->resource_id, 16);
    i->next = resources->next;
    resources->next = i;
    return i;
}

void
queue_request(struct item * i, struct request * r)
{

    struct waiter * new_req;
   
    new_req = malloc(sizeof(struct waiter));
    memset(new_req, 0, sizeof(struct waiter));

    i->request_queue->next = malloc(sizeof (struct waiter));
    memset(i->request_queue->next, 0, sizeof (struct waiter));
    i->request_queue->next->namelen = r->payloadlen;
    i->request_queue->next->endpoint = malloc(r->payloadlen);
    memcpy(i->request_queue->next->endpoint, r->payload, r->payloadlen);
    i->request_queue->next->next = i->request_queue;
}

struct item *
find_item(unsigned char id[16])
{
    struct item * i;

    memcpy(resources->id, id, 16);
    for(i = resources->next; memcmp(i->id, id, 16); 
        i = i->next);
    return i; 
}

void
send_reply(struct item * i, struct request * r)
{
    return;
}
