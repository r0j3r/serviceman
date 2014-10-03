#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>  
#include <stdio.h>
#include "message.h"
#include "notification.h"

struct request * create_request(int, unsigned char[16], unsigned char[16]);

int
main(void)
{
    int endp = create_endpoint("/run/volume-manager11");
    if (endp > 0)
    { 
        int ret = send_notify(endp, TMP_STATUS, "volume-manager11");
        printf("sent %d bytes\n", ret);
    }
    unlink("/run/volume-manager11");
}
