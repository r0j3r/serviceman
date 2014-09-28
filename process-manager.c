#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/reboot.h>
#include <signal.h>
#include <mntent.h>

int setctty(char *);
unsigned char any_child_exists(void);
void process_child_shutdown(pid_t);
int unmounted_all(void);

int run_state = 0;
pid_t volume_manager_pid = 0, udevd_pid = 0, agetty1_pid = 0, agetty2_pid = 0;
unsigned char got_sigchild = 0;

void
handle_sigusr1(int num, siginfo_t * info, void * b)
{
    run_state = RB_POWER_OFF;
}

void
handle_sigterm(int num, siginfo_t * info, void * b)
{
    run_state = RB_AUTOBOOT;
}

void
handle_sigchild(int num, siginfo_t * info, void * b)
{
    got_sigchild = 1;
}

int
main(void)
{
    int fd, wait_status;
    pid_t udevadm_pid;
    struct sigaction sig;

    (void)fprintf(stderr, "process-manager: process-manager starting...\n");

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigusr1;
    sigaction(SIGUSR1, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigterm;
    sigaction(SIGTERM, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigchild;
    sigaction(SIGCHLD, &sig, 0);
       
    fd = open("/dev/console", O_RDWR | O_NOCTTY);
    if (fd >= 0)
    {
        dup2(fd,0); 
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        (void)fprintf(stderr, "process-manager: console reopened...\n");
    }
    else
    {
        (void)fprintf(stderr, "failed to open console");
    }     
    
    volume_manager_pid = fork();
    if (!volume_manager_pid) 
    {
        char * argv[2];

        argv[0] = "volume-manager";
        argv[1] = 0; 
        execv("/sbin/volume-manager", argv);
        _exit(4);
    }

    udevd_pid = fork();
    if (!udevd_pid)
    {
        char * argv[2];

        argv[0] = "udevd";
        argv[1] = 0;
        execv("/sbin/udevd", argv);
        _exit(4);  
    }

    (void)fprintf(stderr, "loading subsystems...\n");
    udevadm_pid = fork();
    if (!udevadm_pid)
    {
        char * argv[4];

        argv[0] = "udevadm";
        argv[1] = "trigger";
        argv[2] = "--type=subsystems";
        argv[3] = 0;
        execv("/sbin/udevadm", argv);
        _exit(4);
    }

    wait(&wait_status);

    udevadm_pid = fork();
    if (!udevadm_pid)
    {
        char * argv[3];

        argv[0] = "udevadm";
        argv[1] = "settle";
        argv[2] = 0;
        execv("/sbin/udevadm", argv);
        _exit(4);
    }

    wait(&wait_status);

    (void)fprintf(stderr, "loading devices...\n");

    udevadm_pid = fork();
    if (!udevadm_pid)
    {
        char * argv[4];

        argv[0] = "udevadm";
        argv[1] = "trigger";
        argv[2] = "--type=devices";
        argv[3] = 0;
        execv("/sbin/udevadm", argv);
        _exit(4);
    }

    wait(&wait_status);

    udevadm_pid = fork();
    if (!udevadm_pid)
    {
        char * argv[3];

        argv[0] = "udevadm";
        argv[1] = "settle";
        argv[2] = 0;
        execv("/sbin/udevadm", argv);
        _exit(4);
    }

    wait(&wait_status);

    agetty1_pid = fork();
    if (!agetty1_pid)
    {
        char * argv[5];
        int fd; 

   
        close(0);
        close(1);
        close(2);
        setsid();
        fd = open("/dev/tty1", O_RDWR);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);        

        argv[0] = "agetty";
        argv[1] = "tty1";
        argv[2] = "9600";
        argv[3] = "linux";
        argv[4] = 0;
        execv("/sbin/agetty", argv);
        _exit(4);   
    }

    agetty2_pid = fork();
    if (!agetty2_pid)
    {
        char * argv[5];
        int fd;

        close(0);
        close(1);
        close(2);
        setsid();
        fd = open("/dev/tty2", O_RDWR);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);        

        argv[0] = "agetty";
        argv[1] = "tty2";
        argv[2] = "9600";
        argv[3] = "linux";
        argv[4] = 0;
        execv("/sbin/agetty", argv);
        _exit(4); 
    }

    while(0 == run_state)
    {
        int status;
        pid_t child_pid; 
 
        child_pid = wait(&status);

        if (child_pid < 0)
        {
            if (errno == EINTR)
            {
                fprintf(stderr, "process-manager: signal received: run_state == %d\n", run_state); 
            } 
        }

        if (child_pid == agetty1_pid)
        {
            agetty1_pid = fork();
            if (!agetty1_pid)
            {
                char * argv[5];
                int fd;

                close(0);
                close(1);
                close(2);
                setsid(); 
                fd = open("/dev/tty1", O_RDWR);
                dup2(fd, 0);
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);        

                argv[0] = "agetty";
                argv[1] = "tty1";
                argv[2] = "9600";
                argv[3] = "linux";
                argv[4] = 0;
                execv("/sbin/agetty", argv);
                _exit(4);
            }
        } 
        if (child_pid == agetty2_pid)
        {
            agetty2_pid = fork();
            if (!agetty2_pid)
            {
                char * argv[5];
                int fd;

                close(0);
                close(1);
                close(2);
                setsid();
                fd = open("/dev/tty2", O_RDWR);
                dup2(fd, 0);
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);        

                argv[0] = "agetty";
                argv[1] = "tty2";
                argv[2] = "9600";
                argv[3] = "linux";
                argv[4] = 0;
                execv("/sbin/agetty", argv);
                _exit(4);
            }
        }
    }

    fprintf(stderr, "process-manager shutting down system...\n");
    unsigned char any_child_exists = 1, timeout = 0;
    int status = 0;

    fprintf(stderr, "sending SIGTERM to all processes...\n");
    if (-1 == kill(-1, SIGTERM))
    {
        fprintf(stderr, "process-manager: kill failed: %s\n", strerror(errno));
    }
    while (any_child_exists)
    {
        pid_t child = wait(&status);
        if (0 > child) 
        {
            if (ECHILD == errno) 
            {
                any_child_exists = 0;
                fprintf(stderr, "process-manager: no children remains...\n");
            } 
        }   
    }

    unsigned char root_busy = 5;
    while(root_busy)
    {
        if (unmounted_all()) 
        {
            root_busy = 0;
        } 
        else
        {
            fprintf(stderr, "process-manager: unmount failed: sending SIGKILL to stragglers...\n");
            kill(-1, SIGKILL);
            sleep(10);
            root_busy--;
            fprintf(stderr, "process-manager: retrying unmount\n");
        }
    } 
    if (-1 == mount("", "/dev", "", MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount dev read only: %s\n", strerror(errno));
    }
    if (-1 == mount("", "/proc", "", MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount proc read only: %s\n", strerror(errno));
    }
    if (-1 == mount("", "/", "", MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount root read only: %s\n", strerror(errno));
    }

    sync();
    fprintf(stderr,"root synced calling reboot...\n"); 
    sleep(20);
    reboot(run_state); 
    while(1);
        pause();      
}

int
unmounted_all(void)
{
    FILE * mounts_file;

    fprintf(stderr, "trying to unmount all partitions...\n");
    int tries = 10, to_unmount = 0, unmounted = 0;
    while(tries)
    {
        mounts_file = setmntent("/proc/mounts", "r");
        if (mounts_file)
        {
            struct mntent * entry;
            while((entry = getmntent(mounts_file)))
            {
                if (0 == strcmp(entry->mnt_dir, "/"))
                    continue;
                else if (0 == strcmp(entry->mnt_dir, "/proc"))
                    continue;
                else if (0 == strcmp(entry->mnt_dir, "/dev"))
                    continue;                
                else 
                {
                    fprintf(stderr, "trying to unmount %s\n", entry->mnt_dir);
                    to_unmount++; 
                    if (0 == umount (entry->mnt_dir))
                    {
                        unmounted++;
                    }
                    else
                    {
                        fprintf(stderr, "unmount failed: %s\n", strerror(errno)); 
                    }
                }        
            }
            endmntent(mounts_file);
        }
        else
        {
            fprintf(stderr, "failed to open /proc/mounts...\n");
            sleep(10);  
        } 
        fprintf(stderr, "to_unmount = %d, unmounted = %d\n", to_unmount, unmounted);
        if (to_unmount == unmounted)
            tries = 0;
        else
        {
            to_unmount = 0;
            unmounted = 0;
            tries--;
        }
    }
 
    return (to_unmount == unmounted);
}
