#include <stdlib.h>
#include <unistd.h>  
#include <stdio.h>
#include "message.h"
#include "notification.h"

struct request * create_request(int, unsigned char[16], unsigned char[16]);

int
main(void)
{
    int endp = create_endpoint("/run/test_client12345");
    if (endp > 0)
    { 
        struct request * r = create_request(READ, ROOT_STATUS, "test_client12345");
        int ret = send_message(endp, "early-boot-arbit", 16, r, sizeof(*r));
        printf("sent %d bytes\n", ret);
        unsigned char data[4096];
        if (-1 != read(endp, data, 4096))
        {
            printf("received reply\n");
        }
    }
    unlink("/run/test_client12345");
}
