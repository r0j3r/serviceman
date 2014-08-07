#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

unsigned char sigchld_flag = 0;

struct service
{
    pid_t pid; //main key  
    struct service * next; //chain in hash table
    unsigned char keepalive;
    unsigned short restart_count;
    unsigned short restart_limit;
    unsigned int argc;
    char * filename;
    char *argv[];
};

unsigned char running = 1;

int
got_sigchld()
{
    return sigchld_flag == 1;
}


struct service * serv[1 << 10]; //array of dummy nodes

struct service * remove_service(pid_t);
int insert_service(struct service *);

const char * SERVCTL_SOCKET = "/tmp/servctl";

struct service *
remove_service(pid_t p)
{
    return 0;
}

int
insert_service(struct service * s)
{
    return 0;
}

int
spawn(struct service * s)
{
    pid_t child_id;

    child_id = fork(); 
    if (child_id != 0) return child_id;
    else execve(s->filename, s->argv, 0);
    return 0;
}

int 
process_child_exit(pid_t child_id, int status)
{
    struct service * service;

    //get service using child_id
    service = remove_service(child_id); 
    if (service->keepalive)
    {
        if (service->restart_count < service->restart_limit)
        {
            if (spawn(service) == 0)
            {
                service->restart_count += 1;
                insert_service(service);
            }
            else
            {
                
            } 
        }
    }
    return 0;
}

int
parse_payload()
{
    return 0;
}

int
parse_signature()
{
    return 0;
}

int
process_packet()
{
    parse_signature();
    parse_payload();
    return 0;
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
    if (unlink(SERVCTL_SOCKET) < 0) printf("failed to unlink socket: %s\n",
        strerror(errno));
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

int
main_loop()
{
    int ret;
    unsigned char read_buff[8192];
    int sock, peer_sock;
    struct sockaddr_un peer_addr;
    socklen_t peer_socklen;

    sock = open_server_socket();
    while(running)
    {
        peer_sock = accept(sock, (struct sockaddr *)&peer_addr, &peer_socklen);
        if (peer_sock > 0)
        { 
            ret = read(peer_sock, read_buff, sizeof(read_buff));
            {
                if (ret == -1)
                {
                    if (got_sigchld())
                    {
                        int status;
                        pid_t child_id;        

                        child_id = waitpid(0, &status, 0);
                        process_child_exit(child_id, status);
                    }
                }
                else
                {
                    process_packet(read_buff);
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

int main(int argc, char *argv[])
{
    struct sigaction sig;

    memset(&sig, 0, sizeof(sig));

    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = stop_running;
    sigaction(SIGINT, &sig, 0);
    sigaction(SIGTERM, &sig, 0);

    main_loop();
    return 0;
}
