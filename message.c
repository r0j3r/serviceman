#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "message.h"

int
send_message(int endp, unsigned char * dest, unsigned int dest_len, unsigned char * d, unsigned int len)
{
    struct sockaddr_un u;
    struct message * m;
    unsigned char dest_path[128] = "/run/";

    if (dest_len > 64) return -1;
    
    m = malloc(sizeof(*m) + len);
    memcpy(m->protocol_id, MESSAGE_PROTO_ID, 16);
    m->payload_len = len;
    memcpy(m->payload, d, len); 

    memset(&u, 0, sizeof(u));
    u.sun_family = AF_UNIX;
    memcpy(dest_path + 5, dest, dest_len);
    dest_path[5 + dest_len] = 0;
    memcpy(u.sun_path, dest_path, strlen(dest_path));
    int ret = sendto(endp, m, sizeof(*m) + len, 0, (struct sockaddr *)&u, sizeof(u.sun_family) + strlen(dest_path));
    if (ret < 0)
    {
        fprintf(stderr, "failed sendto: %s: %s\n", dest_path, strerror(errno));
        sleep(10);
    }
    return ret;    
}
