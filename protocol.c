#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include "error.h"
#include "protocol.h"
#include "message.h"
#include "notification.h"
#include "servctl.h"

const char * SERVCTL_SOCKET = "/tmp/servctl";

enum err_code
get_reply_code(unsigned char * d, unsigned int l)
{
    if (l > sizeof(struct reply))
    {
        struct reply * rep = (struct reply *)d;
        if (0 == memcmp(rep->protocol_id, PROTO_ID_REPLY_V1, MP_ID_LEN))
        {
            return rep->err_code;
        }
    }
    return INVALID_PARMS;
}

enum err_code
get_reply(int sd, struct sockaddr_un * s, socklen_t * l)
{
    int ret;
    unsigned char data[1024];
    unsigned int data_len = sizeof(data);

    ret = recvfrom(sd, data, data_len, 0, (struct sockaddr *)s, l);
    if (-1 == ret)
    {
        fprintf(stderr, "get_reply: revcfrom failed: %s\n", strerror(errno));
        sleep(3);
    }
    else
    {
        struct message * m = get_message(data, data_len);
        if (m)
        {
            enum err_code error_code = get_reply_code((unsigned char *)m, sizeof(*m) + m->payload_len);
            if (0 == error_code) 
            {
                fprintf(stderr, "op succeeded\n");
            } 
            else
            {
                fprintf(stderr, "op_failed\n");
            }
        }
    }
    return INVALID_PARMS;
}

struct request *
get_servctl_request(unsigned char * d, unsigned int len)
{
    if (len > sizeof(struct request))
    {
        struct request * req = (struct request *)d;
        if (0 == memcmp(req->protocol_id, SERVCTL_PROTO_ID, MP_ID_LEN))
        {
            if (len >= req->data_len + sizeof(*req))
            {
                return req;
            }
        }
    }
    return 0; 
}
