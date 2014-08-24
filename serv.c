#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include "error.h"
#include "protocol.h"

unsigned char sigchld_flag = 0;

struct service
{
    pid_t pid; //main key  
    unsigned char * label;
    struct service * next; 
    unsigned char * keepalive;
    unsigned short restart_count;
    unsigned short restart_limit;
    unsigned char login_session;
    struct timeval last_restart_tstamp;
    unsigned int throttle_interval; 
    unsigned int argc;
    unsigned char * filename;
    char * const * argv;
    unsigned char * data;
};

unsigned char running = 1;

int
got_sigchld()
{
    return sigchld_flag == 1;
}

struct service * s_list; //dummy node

struct service * remove_service(pid_t);
int insert_service(struct service *);

void
init_service_list(void)
{
    s_list = malloc(sizeof(*s_list));
    memset(s_list, 0,  sizeof(*s_list));
    s_list->next = s_list;
}

struct service *
create_service(unsigned char * p)
{
    struct service * s;
    struct packet * pack = (struct packet *)p;
    unsigned char * string_buffer;

    s = malloc(sizeof(*s));
    if (s == 0)
    {
        return 0;
    }
    memset(s, 0, sizeof(*s));
    s->throttle_interval = 10;    
    s->login_session = pack->login_session;
 
    string_buffer = p + pack->stringtab_offset;
    s->label = string_buffer + pack->label;
    if (pack->program)
    {
        s->filename = string_buffer + pack->program;
        printf("filename is: %s\n", s->filename);
    }
    else
    {
        if (pack->program_args)
        {
            s->filename = p + pack->program_args; 
        }
    }
    if (pack->program_args)
    {
        s->argv = (char * const *)(p + pack->program_args);
    }        
    s->argc = pack->program_args_count;
   
    return s;
}

struct service *
find_service_with_pid(pid_t p)
{
    struct service * n;

    s_list->pid = p;

    n = s_list->next;
    while(n->pid != p) n = n->next;
    if (n->next != n)
    {
        return n; 
    }
    return 0;
}

int
insert_service(struct service * s)
{
    s->next = s_list->next;
    s_list->next = s;
    return 0;
}

int
spawn(struct service * s)
{
    pid_t child_id;

    child_id = fork();
    printf("forking\n");
    if (child_id < 0)
    {
        printf("fork failed\n");
        return -1;
    } 
    if (child_id != 0)
    {
        s->pid = child_id;   
        return child_id;
    }
    else
    {
        if (s->login_session)
            setsid(); 
        printf("executing %s\n", s->filename); 
        if (execvp((const char *)s->filename, s->argv) < 0)
        {
            printf("child: exec failed\n");
            exit(1);
        }
    }
    return 0;
}

int 
process_child_exit(pid_t child_id, int status)
{
    struct service * service;

    printf("processing child exit\n");
    //get service using child_id
    service = find_service_with_pid(child_id);
    if (!service) return -1; 
    service->pid = 0; 
    if (service->keepalive)
    {
        int restart_option; 
        int restart_flag = 0;
        int option_pos = 0;

        while(service->keepalive[option_pos])
        {
            restart_option = service->keepalive[option_pos];
            option_pos++;
            if (restart_option == 1) //successful exit
            {
                if (service->keepalive[option_pos] == 0) //successfulexit false
                {  
                    if (WIFEXITED(status))
                    { 
                         restart_flag |= (WEXITSTATUS(status) != 0);
                    }
                }
                else //successfulexit true
                {
                    if (WIFEXITED(status))
                    { 
                         restart_flag |= (WEXITSTATUS(status) == 0);
                    }                    
                }
            } 
            option_pos++;
        }
        if (restart_flag == 1)
        { 
            struct timeval t_now;

            memset(&t_now, 0, sizeof(t_now));
            gettimeofday(&t_now, 0);
            if ((service->throttle_interval 
                + service->last_restart_tstamp.tv_sec) 
                < t_now.tv_sec)
            {
                if (service->restart_limit)
                { 
                    if (service->restart_count < service->restart_limit)
                    {
                        if (spawn(service) == 0)
                        {
                            service->restart_count += 1;
                            service->last_restart_tstamp = t_now;
                        }
                    }
                }
                else if (spawn(service) == 0)
                {
                    service->restart_count += 1;
                    service->last_restart_tstamp = t_now;
                }
            }
        }
    }
    return 0;
}

enum err_code
process_packet(unsigned char * p)
{
    struct proto_meta * p_m = (struct proto_meta *)p;
    struct service * service;

    if (memcmp(p_m->protocol_id, PROTO_ID_V1, 16) == 0)
    {
        switch(p_m->op)
        {
        case 1:
            printf("creating service\n");
            service = create_service(p + sizeof(struct proto_meta));
            if(service)
            {
                service->data = p; 
                if (spawn(service) < 0)
                {
                    printf("spawn failed\n");
                    return E_SPAWN_FAILED;
                }       
                else
                {
                    return SUCCESS;
                }
            }
            break;
        default:
            return E_INVALID_OP;
            break;      
        }
    }
    printf("protocol unknown\n");
    return E_UNKNOWN_PROTOCOL;
}

int
open_server_socket()
{
    int s;
    struct sockaddr_un unix_path;
    int ret;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) return -1;

    memset(&unix_path, 0, sizeof(unix_path));
    unix_path.sun_family = AF_UNIX;
    strcpy(unix_path.sun_path, SERVCTL_SOCKET);

    ret = bind(s, (struct sockaddr *)&unix_path, sizeof(unix_path.sun_family) 
        + strlen(unix_path.sun_path));
    if (ret < 0) 
    {
        close(s);
        return -1;
    } 
    ret = listen(s, 5);
    if (ret < 0) 
    {
        close(s);
        return -1;
    }
    return s;
}

void
block_sigchld()
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, 0);    
}

void
unblock_sigchld()
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &mask, 0);    
}

int
send_reply(enum err_code e, int s)
{

    struct reply r;
    int bytes_written = 0, ret = 0;
  
    memset(&r, 0, sizeof(r));
    memcpy(r.protocol_id, PROTO_ID_REPLY_V1, sizeof(PROTO_ID_REPLY_V1));
    r.err_code = e;
 
    while (bytes_written < sizeof(r))
    {
        ret = write(s, &r + bytes_written, sizeof(r) - bytes_written);
        if (ret > 0)
        {
            bytes_written += ret;
        }
        if (ret < 0)
        {
            return -1; 
        } 
        if (ret == 0)
        {
            return -1;
        }
    } 
    return 0;
}

int
main_loop()
{
    int ret;
    unsigned char read_buff[8192];
    int sock, peer_sock;
    struct sockaddr_un peer_addr;
    socklen_t peer_socklen;
    unsigned char peer_is_connected, proto_meta_needed, payload_needed;
    unsigned int bytes_received = 0;

    sock = open_server_socket();
    if (sock < 0)
    {
        printf("failed to open socket for incoming connections\n");
    }
    while(running)
    {
        peer_sock = accept(sock, (struct sockaddr *)&peer_addr, &peer_socklen);
        if (peer_sock > 0)
        { 
            block_sigchld();
            printf("peer is connected\n");
            peer_is_connected = 1;
            while(peer_is_connected)
            {
                if (bytes_received < sizeof(struct proto_meta))
                {
                    proto_meta_needed = 1;
                }
                while(proto_meta_needed)
                { 
                    printf("reading wait for proto_meta\n");
                    ret = read(peer_sock, read_buff + bytes_received, sizeof(read_buff) - bytes_received);
                    if (ret == 0)
                    {
                        proto_meta_needed = 0;
                    }
                    else
                    {
                        if (ret > 0)
                        {
                            
                            bytes_received += ret;
                            printf("proto_meta bytes read %u\n", bytes_received);
                            if (bytes_received > sizeof(struct proto_meta))
                            {
                                proto_meta_needed = 0;
                            }
                        }
                        else
                        {
                            proto_meta_needed = 0;
                        } 
                    }
                }
                printf("we have some bytes check if its is enough\n");

                printf("bytes_received %u, sizeof struct proto_meta %lu\n", bytes_received, sizeof(struct proto_meta));
                if (bytes_received > sizeof(struct proto_meta))
                {  
                    struct proto_meta * payload = (struct proto_meta *)read_buff;

                    printf("payload len %d\n", payload->payload_len);
                    if ((payload->payload_len + sizeof(struct proto_meta)) 
                            > bytes_received)
                    {
                       payload_needed = 1;
                    }
                    else
                    {
                        payload_needed = 0;
                    }
                    while(payload_needed)
                    {
                        printf("reading waiting for payload\n");
                        ret = read(peer_sock, read_buff + bytes_received, 
                            sizeof(read_buff) - bytes_received);
                        if (ret > 0)
                        {
                            printf("payload bytes read\n");
                            bytes_received += ret;
                            if ((payload->payload_len + sizeof(struct proto_meta)) 
                                <= bytes_received)
                            {
                                payload_needed = 0;
                            }
                        } 
                        else
                        {
                            payload_needed = 0;
                        }
                    } 
                    if (ret <= 0)
                    {
                        peer_is_connected = 0;
                    }
                    if ((payload->payload_len + sizeof(struct proto_meta)) 
                            <= bytes_received)
                    {
                        unsigned int remaining_bytes;
                        int err;               
                        unsigned char * d;       
   
                        printf("processing\n");
                        d = malloc(payload->payload_len 
                            + sizeof(struct proto_meta));
                        memcpy(d, read_buff, payload->payload_len 
                            + sizeof(struct proto_meta)); 
                        err = process_packet(d);
                        send_reply(err, peer_sock);
                        remaining_bytes = bytes_received - (payload->payload_len + sizeof(struct proto_meta));
                        if (remaining_bytes > 0)
                            memmove(read_buff, read_buff + (payload->payload_len + sizeof(struct proto_meta)),
                                remaining_bytes);
                        bytes_received = remaining_bytes;
                        if (bytes_received < sizeof(struct proto_meta))
                        {
                            proto_meta_needed = 1;
                        } 
                    }
                }
                if (ret <= 0)
                {
                    printf("peer stopped connecting\n"); 
                    peer_is_connected = 0;
                }
                printf("going back to top\n");
            }
            printf("peer disconnected\n"); 
            close(peer_sock);
            unblock_sigchld();
        }
        else
        {
            if (got_sigchld())
            {
                int status;
                pid_t child_id;        

                block_sigchld();
                child_id = waitpid(0, &status, WNOHANG);
                while(child_id) 
                {                 
                    process_child_exit(child_id, status);
                    child_id = waitpid(0, &status, 0);
                };
                sigchld_flag = 0; 
                unblock_sigchld();
            }
            if (peer_sock < 0)
            {
                if (EINTR != errno)
                {
                    running = 0;
                }       
            } 
        }
    }
    close(sock);
    if (unlink(SERVCTL_SOCKET) < 0) 
    {
        printf("failed to unlink socket: %s\n", strerror(errno));
    }
    return 0;
}

void
stop_running(int num, siginfo_t * info, void * b)
{
    running = 0;
}

void
hard_stop(int num, siginfo_t * info, void * b)
{
    syslog(LOG_EMERG, "fatal signal %s", strsignal(num));
    sleep(30);
    _exit(1);
}

void
catch_sig(int num, siginfo_t * info, void * b)
{
    static int catch_count = 0;

    catch_count++;
}

void
catch_sigchld(int num, siginfo_t * info, void * b)
{
    sigchld_flag = 1;
}

void
badsys(int num, siginfo_t * info, void * b)
{
    static int badcount = 0;
    if (badcount++ < 25)
    {
        return;
    }
    hard_stop(num, info, b);
}

int main(int argc, char *argv[])
{
    struct sigaction sig;
    sigset_t mask;

    init_service_list(); 

    openlog("serv", LOG_CONS|LOG_ODELAY, LOG_AUTH);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = stop_running;
    sigaction(SIGTERM, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = catch_sigchld;
    sigaction(SIGCHLD, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = badsys;
    sigaction(SIGSYS, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = hard_stop;
    sigaction(SIGABRT, &sig, 0);
    sigaction(SIGFPE, &sig, 0); 
    sigaction(SIGILL, &sig, 0);
    sigaction(SIGSEGV, &sig, 0);
    sigaction(SIGBUS, &sig, 0);
    sigaction(SIGXCPU, &sig, 0);
    sigaction(SIGXFSZ, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = catch_sig;
    sigaction(SIGHUP, &sig, 0);    
    sigaction(SIGINT, &sig, 0);
    sigaction(SIGTSTP, &sig, 0);
    sigaction(SIGUSR1, &sig, 0); 
    sigaction(SIGUSR2, &sig, 0);
    sigaction(SIGALRM, &sig, 0);

    sigfillset(&mask);
    sigdelset(&mask, SIGABRT);
    sigdelset(&mask, SIGFPE);
    sigdelset(&mask, SIGILL);
    sigdelset(&mask, SIGSEGV);
    sigdelset(&mask, SIGBUS);
    sigdelset(&mask, SIGSYS);
    sigdelset(&mask, SIGXCPU);
    sigdelset(&mask, SIGXFSZ);
    sigdelset(&mask, SIGHUP);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGTERM);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGUSR2);
    sigdelset(&mask, SIGTSTP);
    sigdelset(&mask, SIGALRM);
    sigdelset(&mask, SIGCHLD);
    sigprocmask(SIG_SETMASK, &mask, 0);        

    memset(&sig, 0, sizeof(sig));
    sigemptyset(&sig.sa_mask);
    sig.sa_handler = SIG_IGN;
    sigaction(SIGTTIN, &sig, 0);
    sigaction(SIGTTOU, &sig, 0);

    main_loop();
    return 0;
}
