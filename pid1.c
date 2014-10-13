#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include <linux/kd.h>
#include <sys/reboot.h>
#include <sys/ioctl.h>

unsigned char sys_shutdown = 0;
unsigned char sys_reboot = 0;

void
handle_sigint(int num, siginfo_t * i, void * d)
{
    sys_reboot = 1;
}

void
handle_sigusr1(int num, siginfo_t * i, void * d)
{
    sys_shutdown = 1;
}

int
main()
{
    int status, i;
    sigset_t set;
    struct sigaction sig;

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

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigint;
    if (0 > sigaction(SIGINT, &sig, 0))
        fprintf(stderr, "failed to set sigaction SIGINT: %s\n", strerror(errno));

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigusr1;
    if (0> sigaction(SIGUSR1, &sig, 0))
        fprintf(stderr, "failed to set sigaction SIGUSR1: %s\n", strerror(errno));

    for(i = 3; i < NR_OPEN; i++) close(i);
    reboot(RB_DISABLE_CAD);

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, 0);

    pid_t proc_manager = fork();
    if (proc_manager)
    {
        sigprocmask(SIG_UNBLOCK, &set, 0);

        for(;;) 
        {
            int ret = waitpid(-1, &status, 0);
            if (ret < 0)
            {
                fprintf(stderr, "waitpid exited with: %s\n", strerror(errno));
                if (ECHILD == errno)
                {
                    break;
                } 
                else if (EINTR)
                {
                    if (sys_shutdown)
                    {
                        fprintf(stderr, "received SIGUSR1, shutting down...\n");
                        kill(proc_manager, SIGTERM);
                    }
                    else if (sys_reboot)
                    {    
                        fprintf(stderr, "received SIGINT, rebooting...\n");
                        kill(proc_manager, SIGTERM);
                    }
                    break;
                }
            }
        }         
        waitpid(proc_manager, &status, 0);
        if (0 == fork())
        {
            char * argv[3];
            argv[0] = "shutdown";
            if (sys_shutdown)
            {
                argv[1] = 0;
            }
            else if (sys_reboot)
            {     
                argv[1] = "-r";
                argv[2] = 0;
            }
            execv("/lib/process-manager/serviceman-shutdown", argv);
            _exit(4);  
        }
        else
        {
            wait(&status); 
            while(1)
                pause(); 
        }
    }
   
    sigprocmask(SIG_UNBLOCK, &set, 0);

    setsid();
    setpgid( 0, 0);   

    execve("/sbin/process-manager", (char *[]) {"process-manager", 0}, (char *[]) { 0 });
    fprintf(stderr, "child failed to exec process-manager, exiting...\n"); 
    _exit(4); 
}
