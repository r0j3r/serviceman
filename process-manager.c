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

char * arbitrator_argv[] = {"early-boot-arbitrator", 0};
struct child_process arbitrator = {0, 0, "/sbin/early-boot-arbitrator", arbitrator_argv, 0, "arbitrator", 0, 0, {0, 0}, 0};

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
                                        fprintf(stderr, "keepalive opts %d:%d\n", proc->keepalive_opts[i], 
                                            proc->keepalive_opts[i + 1]);
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
                                        fprintf(stderr, "process %s:%d exited with %d\n", proc->label, proc->pid, WEXITSTATUS(status));
                                    else if (WIFSIGNALED(status)) 
                                        fprintf(stderr, "process %s:%d signaled with %d\n", proc->label, proc->pid, WTERMSIG(status));
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
    struct child_process * v = running->next;
    while(running != v)
    {
        if (-1 == kill(v->pid, SIGTERM))
        {
            fprintf(stderr, "process-manager: kill failed: %s\n", strerror(errno));
        }
        v = v->next;
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
                    timeout = 0;
                    any_child_exists = 0;
                }
            }
        }   
    }
    alarm(0);

    sleep(1);
 
    close(ctl_p_endp);
    unlink("/run/process-manager/procman");
    return 0;
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
        fprintf(stderr, "child %s: pid %d\n", c->exec_file_path, c->pid);
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
