
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include "definition_packet.h"
#include "notification.h"
#include "message.h"
#include "servctl.h"
 
struct request * create_request_buf(unsigned int);
extern unsigned char timeout;
extern unsigned char * progname;

int
create_endpoint(char * name)
{
    int fd;
    struct sockaddr_un endp;
    pid_t our_pid = getpid();

    int try_bind = 3;
    while(try_bind > 0)
    {
        fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd < 0)
        { 
            fd = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (fd < 0)
            {
                fd = socket(AF_UNIX, SOCK_DGRAM, 0);
                if (fd < 0)
                {    
                    return -1;
                }
            }
        }
        fprintf(stderr, "%s:%d create_endpoint(), socket fd is %d\n", progname, our_pid, fd);   
        memset(&endp, 0, sizeof(endp));
        endp.sun_family = AF_UNIX;
        strcpy(endp.sun_path, name);    
        if (-1 == bind(fd, (struct sockaddr *)&endp, sizeof(struct sockaddr_un)))
        {
            fprintf(stderr, "%s:%d, bind failed %s\n", progname, our_pid, strerror(errno)); 
            if (EADDRINUSE == errno)
            {
                unsigned char p[16];
                memcpy(p, PING_PROTO_ID, 16);
                struct sockaddr_un u;
                memset(&u, 0, sizeof(u));
                u.sun_family = AF_UNIX;
                strcpy(u.sun_path, name);
                int ret = sendto(fd, p, sizeof(p), 0, (struct sockaddr *)&u, sizeof(u.sun_family) + strlen(name));
                if (-1 == ret) 
                {
                    if (ECONNREFUSED == errno)
                    {
                        unlink(name);
                        close(fd);
                        try_bind--;
                        fprintf(stderr, "%s:%d, taking over socket. retrying bind %s\n", progname, our_pid, name);
                    }
                    else
                    {
                        fprintf(stderr, "%s:%d, bind %s failed: %s\n", progname, our_pid, name, strerror(errno));
                        try_bind = 0;
                        close(fd);
                        fd = -2;
                    }
                }  
                else
                {
                    char data[1024];
                    alarm(1);
                    timeout = 0;      
                    ret = read(fd, data, sizeof(data));
                    if (EINTR == errno)
                    {
                        if (timeout)
                            timeout = 0;
                    }
                    try_bind = 0;
                    close(fd);
                    fd = -3;
                    alarm(0);
                    fprintf(stderr, "%s:%d, failed to bind %s: someone else is using the socket\n", progname, our_pid, name);
                }
            
            }
            else
            {
                fprintf(stderr, "%s:%d, bind %s failed: %s\n", progname, our_pid, name, strerror(errno));
                if (EINTR != errno)
                {
                    close(fd);
                    try_bind--;
                    fd = -errno;
                    sleep(1);
                } 
            }
        }
        else
        {
            try_bind = 0; 
        }         
    } 
    if (0 <= fd)
        chmod(name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    return fd;
}

struct request *
create_request(unsigned int op, const unsigned char * resource_id, unsigned char * sender_endp)
{
    struct request * r = create_request_buf(0);
    r->op = op;
    memcpy(r->resource_id, resource_id, MP_ID_LEN);
    memcpy(r->sender_endp, sender_endp, MP_ID_LEN);
    return r;
}

struct request *
create_request_buf(unsigned int len)
{
    struct request * r = malloc(sizeof(struct request) + len);
    memset(r, 0, sizeof(struct request) + len);
    memcpy(r->protocol_id, NOTIFICATION_PROTO_ID, MP_ID_LEN);
    r->data_len = len;
    return r;
}


struct request *
create_servctl_request_buf(unsigned int len)
{
    struct request * r = malloc(sizeof(struct request) + len);
    memset(r, 0, sizeof(struct request) + len);
    memcpy(r->protocol_id, SERVCTL_PROTO_ID, MP_ID_LEN);
    r->data_len = len;
    return r;
}

struct request *
create_servctl_request(unsigned int op, const unsigned char * resource_id, struct svc_packet * p)
{
    struct request * r = create_servctl_request_buf(p->packet_size);
    r->op = op;
    memcpy(r->resource_id, resource_id, MP_ID_LEN);
    memcpy(r->data, p, p->packet_size);
    return r;
}

int
send_notify(int fd, const unsigned char resource_id[16], unsigned char endp_name[16])
{
    int status_code = 0;
    struct request * r = create_request_buf(sizeof(status_code));
    r->op = CREATE;
    memcpy(r->resource_id, resource_id, MP_ID_LEN);
    memcpy(r->sender_endp, endp_name, MP_ID_LEN);
    memcpy(r->data, &status_code, 4);
    r->data_len = 4;
    send_message(fd, (unsigned char *)"early-boot-arbit", 16, (unsigned char *)r, sizeof(*r));
    unsigned char data[4096];
    if (-1 == read(fd, data, 4096))
    {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
    } 
    return 0;
}
