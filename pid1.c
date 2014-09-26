#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

int
main()
{
   int status, i;
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

    (void)fprintf(stderr, "pid1 starting...\n");

    for(i = 3; i < NR_OPEN; i++) close(i);

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, 0);

    if (fork())
    {
        for(;;) waitpid(-1, &status, 0);
    }
   
    sigprocmask(SIG_UNBLOCK, &set, 0);

    setsid();
    setpgid( 0, 0);   

    //mount /proc /sys /dev /dev/pts /dev/shm?

    return execve("/sbin/process-manager", (char *[]) {"process-manager", 0}, (char *[]) { 0 });
}
