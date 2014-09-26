
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "notification.h"

int
create_endpoint(char * name)
{
    int fd;
    struct sockaddr_un endp;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    { 
        (void)fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    memset(&endp, 0, sizeof(endp));
    endp.sun_family = AF_UNIX;
    strcpy(endp.sun_path, name);    

    if (-1 == bind(fd, (struct sockaddr *)&endp, sizeof(struct sockaddr_un)))
    {
        (void)fprintf(stderr, "failed to bind socket: %s\n", strerror(errno));
        close(fd);
        return -1; 
    }    
    chmod(name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    return fd;
}
