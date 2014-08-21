#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int
main()
{
   int status;
   sigset_t set;

    if (getuid() != 0) 
    {
        (void)fprintf(stderr, "init: %s\n", strerror(EPERM));
        return 1;
    }

    if (getpid() != 1)
    { 
        (void)fprintf(stderr, "init: already running\n");
        return 1;
    }

    close(0);
    close(1);
    close(2);

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, 0);

    if (fork()) for(;;) waitpid(-1, &status, 0);
   
    sigprocmask(SIG_UNBLOCK, &set, 0);

    setsid();
    setpgid( 0, 0);   

    return execve("/sbin/serv", (char *[]) {"serv", 0}, (char *[]) { 0 });
}
