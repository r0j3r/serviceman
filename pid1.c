#define _XOPEN_SOURCE 700
#include <signal.h>
#include <unistd.h>

int
main()
{
   int status;
   sigset_t set;

    if (getpid() != 1) return 1;

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, 0);

    if (fork()) for(;;) wait(&status);
   
    sigprocmask(SIG_UNBLOCK, &set, 0);

    setsid();
    setpgid( 0, 0);   

    return execve("/sbin/rc", (char *[]) {"rc", 0}, (char *[]) { 0 });
}
