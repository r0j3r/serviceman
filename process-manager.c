#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int setctty(char * name);

int
main()
{
    unsigned char running =1;
    int fd, wait_status;
    pid_t volume_manager_pid, udevd_pid, udevadm_pid, agetty1_pid, agetty2_pid;

    (void)fprintf(stderr, "process-manager: process-manager starting...\n");
       
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
    }

    udevd_pid = fork();
    if (!udevd_pid)
    {
        char * argv[2];

        argv[0] = "udevd";
        argv[1] = 0;
        execv("/sbin/udevd", argv);
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
    }

    wait(&wait_status);

    agetty1_pid = fork();
    if (!agetty1_pid)
    {
        char * argv[5];

        argv[0] = "agetty";
        argv[1] = "tty1";
        argv[2] = "9600";
        argv[3] = "linux";
        argv[4] = 0;
        execv("/sbin/agetty", argv);
    }

    agetty2_pid = fork();
    if (!agetty2_pid)
    {
        char * argv[5];

        argv[0] = "agetty";
        argv[1] = "tty2";
        argv[2] = "9600";
        argv[3] = "linux";
        argv[4] = 0;
        execv("/sbin/agetty", argv);
    }

    while(running)
    {
        int status;
        pid_t child_pid; 
 
        child_pid = wait(&status);
        if (child_pid == agetty1_pid)
        {
            agetty1_pid = fork();
            if (!agetty1_pid)
            {
                char * argv[5];

                argv[0] = "agetty";
                argv[1] = "tty1";
                argv[2] = "9600";
                argv[3] = "linux";
                argv[4] = 0;
                execv("/sbin/agetty", argv);
            }
        } 
        if (child_pid == agetty2_pid)
        {
            agetty2_pid = fork();
            if (!agetty2_pid)
            {
                char * argv[5];

                argv[0] = "agetty";
                argv[1] = "tty2";
                argv[2] = "9600";
                argv[3] = "linux";
                argv[4] = 0;
                execv("/sbin/agetty", argv);
            }
        }
    }    
}
