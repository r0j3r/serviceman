
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "notification.h"
#include "message.h"
 
struct request * create_request_buf(unsigned int);

int
create_endpoint(unsigned char * name)
{
    int fd;
    struct sockaddr_un endp;

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
        fprintf(stderr, "socket fd is %d\n", fd);   
        memset(&endp, 0, sizeof(endp));
        endp.sun_family = AF_UNIX;
        strcpy(endp.sun_path, name);    
        if (-1 == bind(fd, (struct sockaddr *)&endp, sizeof(struct sockaddr_un)))
        {
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
                        fprintf(stderr, "retrying bind %s\n", name);
                    }
                    else
                    {
                        fprintf(stderr, "failed to bind %s\n", name);
                        try_bind = 0;
                        close(fd);
                        fd = -2;
                    }
                }  
                else
                {
                    char data[1024];
                    ret = read(fd, data, sizeof(data));
                    try_bind = 0;
                    close(fd);
                    fd = -3;
                    fprintf(stderr, "failed to bind %s: someone else is using the socket\n", name);
                }
            
            }
            else
            {
                fprintf(stderr, "bind %s failed: %s\n", name, strerror(errno));
                close(fd);
                try_bind--;
                fd = -4;
                sleep(1); 
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
create_request(unsigned int op, unsigned char * resource_id, unsigned char * sender_endp)
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

int
send_notify(int fd, unsigned char resource_id[16], unsigned char endp_name[16])
{
    int status_code = 0;
    struct request * r = create_request_buf(sizeof(status_code));
    r->op = CREATE;
    memcpy(r->resource_id, resource_id, MP_ID_LEN);
    memcpy(r->sender_endp, endp_name, MP_ID_LEN);
    memcpy(r->data, &status_code, 4);
    r->data_len = 4;
    int ret = send_message(fd, "early-boot-arbit", 16, r, sizeof(*r));
    unsigned char data[4096];
    if (-1 == read(fd, data, 4096))
    {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
    } 
    return 0;
}
