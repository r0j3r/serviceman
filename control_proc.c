#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include "definition_packet.h"
#include "notification.h"
#include "servctl.h"

extern int signum;

int
launch_control_proc(struct passwd * pwd, char * socket_path, struct sockaddr_un * u, socklen_t * l, pid_t * p)
{
    int endp = -1;
    pid_t child_pid = fork();
    *p = child_pid;
    if (child_pid)
    {
        if (pwd)
        {
            fprintf(stderr, "dropping privs\n");
            setuid(pwd->pw_uid);
            setgid(pwd->pw_gid);
        }

        endp = create_endpoint(socket_path);

        if (endp < 0)
            return -1;
        struct sockaddr_un ctl_sock;
        memset(&ctl_sock, 0, sizeof(ctl_sock));
        socklen_t ctl_socklen = sizeof(ctl_sock);
        size_t len = 4096;
        unsigned char data[4096];
        alarm(1);
        int ret = recvfrom(endp, data, len, 0, (struct sockaddr *)&ctl_sock, &ctl_socklen);
        if (ret < 0)
        { 
            if (EINTR == errno)
            {
                if (SIGALRM == signum)
                {
                    fprintf(stderr, "test_procman: recvfrom timed out\n");
                }
                else if (SIGCHLD == signum)
                {
                    int status;
                    pid_t p = wait(&status);
                    if (WIFEXITED(status))
                        fprintf(stderr, "child %d exited with %d\n", p, WEXITSTATUS(status));
                    else if (WIFSIGNALED(status))
                        fprintf(stderr, "child %d terminated with signal %d\n", p, WTERMSIG(status));
                }
            }
            else
            {
                fprintf(stderr, "launch_control_proc: recvfrom failed: %s\n", strerror(errno));
                sleep(3); 
            }
        } 
        else 
        {  
            if (ctl_checkin(data, ret))
            {
                if (-1 == sendto(endp, SERVCTL_ACK, 16, 0, (struct sockaddr *)&ctl_sock, ctl_socklen))
                    fprintf(stderr, "sendto failed: %s\n", strerror(errno));
                else 
                    fprintf(stderr, "test_procman: sent ack\n"); 

                unsigned char ack_ack[1024];
                size_t acklen = 1024;  
                if (-1 == recvfrom(endp, ack_ack, acklen, 0, (struct sockaddr *)&ctl_sock, &ctl_socklen))
                {
                    fprintf(stderr, "test_procman: failed to recieve ack ack: %s\n", strerror(errno));
                    if (EINTR == errno)
                    {
                        fprintf(stderr, "singum %d, %s\n", signum, strsignal(signum));
                    } 
                }
                else 
                {
                    fprintf(stderr, "test_procman: recieved ack ack\n");
                    if (len)
                    {
                        *l = ctl_socklen;
                        if (u)
                        {
                            memcpy(u, &ctl_sock, ctl_socklen);
                        } 
                    }
                } 
            }
        }
    }
    else
    {
 
        char * argv[3];
        argv[0] = "servctl";
        if (pwd)
        { 
            argv[1] = pwd->pw_name;
            argv[2] = 0;
        }
        else
            argv[1] = 0;

        if (-1 == execv("/lib/process-manager/servctl", argv))
            fprintf(stderr, "execv failed: %s\n", strerror(errno));
        _exit(4);
    }

    return endp;
}

int
ctl_checkin(unsigned char * d, size_t len)
{
    if (len == 16)
    {
        return (0 == memcmp(d, SERVCTL_CHECKIN, 16));
    }
    else
    {
        return 0;
    }  
}
