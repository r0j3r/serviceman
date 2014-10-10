#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <signal.h>
#include <mntent.h>
#include "error.h"
#include "protocol.h"
#include "servctl.h"
#include "message.h"
#include "definition_packet.h"
#include "notification.h"
#include "child_proc.h"

static const unsigned char SUCCESSFUL_EXIT = 1;
char * progname = "process-manager";

struct child_process * running;
struct child_process * waiting;

char * bootlogd_argv[] = {"bootlogd", "-d", 0};
struct child_process bootlogd = {0, 0, "/sbin/bootlogd", bootlogd_argv, 0, "bootlogd", 0, 0, {0, 0}};

char * arbitrator_argv[] = {"early-boot-arbitrator", 0};
struct child_process arbitrator = {0, 0, "/sbin/early-boot-arbitrator", arbitrator_argv, 0, "arbitrator", 0, 0, {0, 0}};

char * volume_manager_argv[] = {"volume-manager", 0};
struct child_process volume_manager = {0, 0, "/sbin/volume-manager", volume_manager_argv, 0, "volume-manager", 0, 0, {0, 0}}; 

char * udevd_argv[] = {"udevd", 0};
struct child_process udevd = {0, 0, "/sbin/udevd", udevd_argv, 0, "udev", 0, 0, {0, 0}};

char * udev_cold_boot_argv[] = {"udev_cold_boot", 0};
struct child_process udev_cold_boot = {0, 0, "/sbin/udev_cold_boot", udev_cold_boot_argv, 0, "udev_cold_boot", 0, 0, {0, 0}};

unsigned char getty1_keepalive_opts[3] = {1, 1, 0};
char * agetty1_argv[] = {"boot-wrapper", "/sbin/agetty", "agetty", "tty3", "9600", "linux", 0};
struct child_process agetty1 = {0, 0, "/sbin/boot-wrapper", agetty1_argv, 1, "agetty1", getty1_keepalive_opts, 0, {0, 0}};
 
unsigned char getty2_keepalive_opts[5] = {1, 1, 1, 0, 0}; //keepalive even if it crashed
char * agetty2_argv[] = {"boot-wrapper", "/sbin/agetty", "agetty", "tty2", "9600", "linux", 0};
struct child_process agetty2 = {0, 0, "/sbin/boot-wrapper", agetty2_argv, 1, "agetty2", getty2_keepalive_opts, 0, {0, 0}};

char * syslogd_argv[] = {"boot-wrapper", "/sbin/syslogd", "syslogd", "-n", 0};
struct child_process syslogd = {0, 0, "/sbin/boot-wrapper", syslogd_argv, 0, "syslogd", 0, 0, {0, 0}};

char * klogd_argv[] = {"boot-wrapper", "/sbin/klogd", "klogd", "-n", 0};
struct child_process klogd = {0, 0, "/sbin/boot-wrapper", klogd_argv, 0, "klogd", 0, 0, {0, 0}};

int setctty(char *);
unsigned char any_child_exists(void);
void process_child_shutdown(pid_t);
int unmounted_all(void);
int spawn_proc(struct child_process *);
struct child_process * find_process(pid_t);
struct child_process * get_process(pid_t);
void queue_proc(struct child_process *);
struct child_process * dequeue_proc(void);
int handle_data(unsigned char *, unsigned int);
int handle_start_proc(struct request *);

int run_state = 0;
unsigned char got_sigchild = 0;
unsigned char timeout = 0;
int signum;

void
handle_sigusr1(int num, siginfo_t * info, void * b)
{
    signum = num; 
    run_state = RB_POWER_OFF;
}

void
handle_sigterm(int num, siginfo_t * info, void * b)
{
    signum = num; 
    run_state = RB_AUTOBOOT;
}

void
handle_sigchild(int num, siginfo_t * info, void * b)
{
    signum = num; 
    got_sigchild = 1;
}

void
handle_sigalarm(int num, siginfo_t * info, void * b)
{
    timeout = 1;
    signum = num; 
}

int
main(void)
{
    int fd;
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
       
    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = handle_sigalarm;
    sigaction(SIGALRM, &sig, 0); 

    fd = open("/dev/console", O_RDWR | O_NOCTTY);
    if (fd >= 0)
    {
        dup2(fd,0); 
        dup2(fd,1);
        dup2(fd,2);
        if (fd > 2)
            close(fd);
        (void)fprintf(stderr, "process-manager: console reopened...\n");
    }
    else
    {
        (void)fprintf(stderr, "failed to open console");
    }     

    if (-1 == mount("none", "/run", "tmpfs", 0, 0))
    {
        fprintf(stderr, "failed to mount tmpfs on /run: %s\n", strerror(errno));
        while(1)
            pause();
    }

    running = malloc(sizeof(*running));
    memset(running, 0, sizeof(*running));
    running->next = running; 

    waiting = malloc(sizeof(*waiting));
    memset(waiting, 0, sizeof(*waiting));
    waiting->next = waiting;
    waiting->pid = 0x7fffffff;
    waiting->last_restart.tv_sec = 0x7fffffffffffffff; 

    struct sockaddr_un ctl_un;
    pid_t ctl_pid;
    socklen_t ctl_len; 
    int ctl_p_endp = launch_control_proc(&ctl_un, &ctl_len, &ctl_pid);

    spawn_proc(&arbitrator);
    /*spawn_proc(&volume_manager);
    spawn_proc(&udevd);
    spawn_proc(&udev_cold_boot);
    spawn_proc(&agetty1);
    spawn_proc(&agetty2);
    spawn_proc(&bootlogd);
*/
    while(0 == run_state)
    {
        int status;
        pid_t child_pid = -1;
        struct child_process * proc;  
        unsigned char restart_proc = 0; 

        if (waiting->next == waiting)
        {
            alarm(0);
        }
        else
        {
            struct timeval now;
            gettimeofday(&now, 0);
            alarm(waiting->next->last_restart.tv_sec - now.tv_sec); 
        }

        timeout = 0; 

        fprintf(stderr, "waiting for exiting childern...\n");
        unsigned char data[1024];
        unsigned int len = sizeof(data);
        if (-1 == recvfrom(ctl_p_endp, data, len, 0, &ctl_un, &ctl_len))
        {
            if (errno == EINTR)
            {
                fprintf(stderr, "process-manager: signal received: run_state == %d\n", run_state);
                if (timeout)
                {
                    timeout = 0;
                    struct timeval now;
                    gettimeofday(&now, 0);  
                    fprintf(stderr, "we got timeout. next timeout %ld, it is now %ld\n", waiting->next->last_restart.tv_sec,
                        now.tv_sec);
                    while(waiting->next->last_restart.tv_sec <= now.tv_sec)
                    { 
                        struct child_process * c = dequeue_proc();
                        fprintf(stderr, "restarting queued proc %s\n", c->label);
                        spawn_proc(c);
                    }
                }
                if (got_sigchild)
                {
                    got_sigchild = 0;
                    while((child_pid = waitpid(-1, &status, WNOHANG)))
                    {
                        if (child_pid < 0)
                        {
                            fprintf(stderr, "faild to get child status: %s\n", strerror(errno));
                            if (EINTR != errno) break;  
                        }
                        else
                        {
                            proc = get_process(child_pid);
                            child_pid = -1;
                            if (proc)
                            {
                                restart_proc = 0; 
                                if (proc->keepalive_opts)
                                {
                                    int i = 0;
                                    while(proc->keepalive_opts[i])
                                    {
                                        if (SUCCESSFUL_EXIT == proc->keepalive_opts[i])
                                        {
                                            if (0 <  proc->keepalive_opts[i + 1])
                                            {
                                                if (WIFEXITED(status))
                                                {
                                                    restart_proc |= 1;
                                                }
                                            }
                                            else
                                            {
                                                if (WIFSIGNALED(status))
                                                {
                                                    restart_proc |= 1;
                                                }  
                                            } 
                                        }
                                        i += 2; 
                                    } 
                                }
            
                                if (restart_proc)
                                {
                                    fprintf(stderr, "restarting %s, exec file %s\n", proc->label, proc->exec_file_path);
                                    struct timeval now;
                                    gettimeofday(&now, 0);
                                    fprintf(stderr, "%s last restart %ld it is now %ld\n", 
                                        proc->label, proc->last_restart.tv_sec, now.tv_sec);
                                    if ((proc->last_restart.tv_sec + 10) < now.tv_sec)
                                    {
                                        gettimeofday(&proc->last_restart, 0);
                                        fprintf(stderr, "spawn %s\n", proc->exec_file_path);
                                        spawn_proc(proc);
                                    }
                                    else
                                    {
                                        if (proc->throttle_count < 40)
                                        {   
                                            fprintf(stderr, "throttling %s\n", proc->label);
                                            gettimeofday(&proc->last_restart, 0);
                                            proc->last_restart.tv_sec += 10;
                                            proc->throttle_count++;
                                            fprintf(stderr, "%s throttle_count %d restart at %ld\n",
                                                 proc->label, proc->throttle_count, proc->last_restart.tv_sec);
                                            queue_proc(proc);
                                        } 
                                        else
                                        {
                                            fprintf(stderr, "%s exceeded throttle limit\n", proc->label);
                                        } 
                                    } 
                                }
                                else
                                {
                                    if (WIFEXITED(status))
                                        fprintf(stderr, "process %s exited with %d\n", proc->label, WEXITSTATUS(status));
                                    else if (WIFSIGNALED(status)) 
                                        fprintf(stderr, "process %s signaled with %d\n", proc->label, WTERMSIG(status));
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
                sleep(3); 
            }  
        }
        else
        {
            handle_data(data, len);
        }
    }

    fprintf(stderr, "process-manager shutting down system...\n");
    unsigned char any_child_exists = 1;
    int status = 0;

    fprintf(stderr, "sending SIGTERM to all processes...\n");
    if (-1 == kill(-1, SIGTERM))
    {
        fprintf(stderr, "process-manager: kill failed: %s\n", strerror(errno));
    }
    
    alarm(10);

    while (any_child_exists)
    {
        pid_t child = wait(&status);
        if (0 > child) 
        {
            if (ECHILD == errno) 
            {
                any_child_exists = 0;
                fprintf(stderr, "process-manager: no more processes left...\n");
            }
            else if (EINTR == errno)
            {
                if (timeout)
                {
                    kill(-1, SIGKILL);
                    timeout = 0;
                    alarm(3); 
                }
            } 
        }   
    }
    alarm(0);

    fprintf(stderr, "nuke stragglers\n");
    kill(-1, SIGKILL);
    sleep(1); 
    close(ctl_p_endp);
    unlink("/run/process-manager/procman");
    if (-1 == mount("", "/", 0, MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount root read only: %s\n", strerror(errno));
    }

    sync();

    unsigned char root_busy = 5;
    while(root_busy)
    {
        if (unmounted_all()) 
        {
            root_busy = 0;
        } 
        else
        {
            kill(-1, SIGKILL); 
            sleep(1);
            root_busy--;
            fprintf(stderr, "process-manager: retrying unmount\n");
        }
    }

    if (-1 == umount("/proc"))
    { 
        fprintf(stderr, "failed to umount /proc: %s\n", strerror(errno));
        sleep(3);
    }
    if (-1 == umount("/dev"))
    { 
        fprintf(stderr, "failed to umount /dev: %s\nremounting read only...\n", strerror(errno));
        if (-1 == mount("", "/dev", 0, MS_REMOUNT | MS_RDONLY, 0))
        { 
            fprintf(stderr, "failed to remount /dev read only: %s\n", strerror(errno));
            sleep(3);
        }
    }

    if (-1 == mount("", "/", 0, MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount root read only: %s\n", strerror(errno));
        sleep(3);
    }

    sync();

    if (-1 == umount("/"))
    { 
        fprintf(stderr, "failed to umount /: %s\n", strerror(errno));
        sleep(3);
    }

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
                        if (-1 == mount("", entry->mnt_dir, 0, MS_REMOUNT|MS_RDONLY, 0))
                        {
                            fprintf(stderr, "remount %s readonly failed: %s\n", entry->mnt_dir, strerror(errno));
                        }   
                    }
                }        
            }
        }
        else
        {
            fprintf(stderr, "failed to open /proc/mounts: %s...\n", strerror(errno));
            sleep(10);  
        } 
        fprintf(stderr, "to_unmount = %d, unmounted = %d\n", to_unmount, unmounted);
        sleep(3);
        if (to_unmount == unmounted)
            tries = 0;
        else
        {
            to_unmount = 0;
            unmounted = 0;
            tries--;
        }
        endmntent(mounts_file);
    }
 
    return (to_unmount == unmounted);
}

int
spawn_proc(struct child_process * c)
{
    c->pid = fork();
    if (0 == c->pid)
    {
        if (c->login_session)
        {
            close(0);
            close(1);
            close(2);

            setsid();

            open("/dev/tty0", O_RDONLY);
            open("/dev/tty0", O_WRONLY);
            open("/dev/tty0", O_WRONLY);
        } 
        execv(c->exec_file_path, c->argv);
        _exit(4);
    }
    else if (-1 == c->pid)
    {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
    }
    else
    {
        c->next = running->next;
        running->next = c;
    }
    return c->pid; 
}

struct child_process *
find_process(pid_t p)
{
    struct child_process * proc; 
    running->pid = p;
    for(proc = running->next; proc->pid != p; proc = proc->next);
    if (proc == running) return 0;
    else return proc;
}


struct child_process *
get_process(pid_t p)
{
    struct child_process * proc, * prev; 
    running->pid = p;
    prev = running;
    while(prev->next->pid != p)
    {
        prev = prev->next; 
    }
    if (prev->next == running) 
        return 0;
    else 
    {
        proc = prev->next;
        prev->next = proc->next; 
        return proc;
    } 
}

void
queue_proc(struct child_process * c)
{
    struct child_process * proc;

    for(proc = waiting; proc->next->last_restart.tv_sec < c->last_restart.tv_sec; proc = proc->next);
    c->next = proc->next;
    proc->next = c;
}

struct child_process *
dequeue_proc(void)
{
    struct child_process * p;
    if (waiting->next != waiting)
    {
        p = waiting->next;
        waiting->next = p->next;
        p->next = 0; 
        return p;  
    }
    return 0; 
}

int
handle_data(unsigned char * d, unsigned int l)
{
    fprintf(stderr, "got data %d bytes\n", l);
    struct message * m = get_message(d, l);
    if (m)
    {
        struct request * req = get_servctl_request(m->payload, m->payload_len);
        if (req)
        {
            if (0 == memcmp(req->resource_id, PROC_LIST_RESOURCE, MP_ID_LEN))
            {
                if (CREATE == req->op)
                {
                    return handle_start_proc(req); 
                }
                else
                {
                    fprintf(stderr, "unrecognised op\n");
                }
            } 
            else
            {
                fprintf(stderr, "unrecognised resource id\n");
            }
        }
        else
        {
            fprintf(stderr, "req is invalid\n");
        }  
    } 
    else
    {
        fprintf(stderr, "message is invalid\n");
    }
    return -1;
}

int
handle_start_proc(struct request * r)
{
    fprintf(stderr, "starting proc\n");
    struct child_process * c = make_child_proc(r->data);
    if (0 == c)
    {
        fprintf(stderr, "failed to created child process structure\n");
        return -1;
    } 
    else
    {
        fprintf(stderr, "starting proc %s\n", c->label);
        gettimeofday(&c->last_restart, 0);
        spawn_proc(c);
        return 0;
    }
}
