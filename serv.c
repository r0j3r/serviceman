#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
    return 0;
}

int
main_loop()
{
    int ret;
    unsigned char read_buff[8192];
    int sock;

    sock = open_server_socket();
    while(running)
    {
        ret = read(sock, read_buff, sizeof(read_buff));
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
    close(sock);
    return 0;
}

int main(int argc, char *argv[])
{
    main_loop();
    return 0;
}
