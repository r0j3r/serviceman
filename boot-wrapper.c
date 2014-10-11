#include <stdlib.h>
#include <unistd.h>  
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/un.h>
#include "message.h"
#include "definition_packet.h"
#include "notification.h"


char * create_endp_name(void);
char * create_endp_path(char *);
unsigned char * progname = "boot-wrapper";

unsigned char timeout = 0;

int
main(int argc, char * argv[])
{
    pid_t our_pid = getpid();
    char logname[1024];

    fprintf(stderr, "boot-wrapper %d starting...\n", our_pid);
    char * endp_name = create_endp_name();
    char * endp_path = create_endp_path(endp_name);
    int endp = create_endpoint(endp_path);
    if (endp >= 0)
    { 
        fprintf(stderr, "boot-wrapper %d: endpoint %s created successfully\n", our_pid, endp_path);
        struct request * r = create_request(READ, ROOT_STATUS, endp_name);
        int ret = send_message(endp, "early-boot-arbit", 16, r, sizeof(*r));
        if (ret > 0)
        {
            fprintf(stderr, "boot-wrapper %d: sent %d bytes\n", our_pid, ret);
            unsigned char data[4096];
            ret = read(endp, data, 4096);
            if (ret < 0)
            {
                fprintf(stderr, "boot-wrapper %d: read for reply failed: %s\n", our_pid, strerror(errno)); 
            }
            else
            {
                fprintf(stderr, "boot-wrapper %d: recieved %d bytes in reply\n", our_pid, ret);
            }
        }
        else
        {
            fprintf(stderr, "boot-wrapper %d: send_message failed\n", our_pid); 
        }
    
        close(endp);
        unlink(endp_path);
        free(endp_name);
        free(endp_path);
        if (ret > 0)
        {
            fprintf(stderr, "boot-wrapper %d: ready to boot %s\n", our_pid, argv[1]);
            execv(argv[1], &argv[2]);
            exit(1);
        }
        exit(2);
    
    }
    if (endp < 0)
    { 
        fprintf(stderr, "endp is %d\n", endp); 
        if (endp == -1)
            exit(3);
        else if (endp == -2)
            exit(4);
        else if (endp == -3)
            exit(5);
        else if (endp == -4)
            exit(6);
        else
           exit(7);
    }
    else
    {
        fprintf(stderr, "boot-wrapper %d: endp is %d\n", our_pid, endp);
        exit(8);
    }
}

char *
create_endp_name(void)
{
    char * n = malloc(1024);
    sprintf(n, "client%010d", getpid());
    return n; 
}

char *
create_endp_path(char * name)
{
    char * n = malloc(1024);
    sprintf(n, "/run/%s", name);
    return n; 
}
