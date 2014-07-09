#include <sys/types.h>

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
    return sigchld_flag == 0;
}


struct service * serv[1 << 10]; //array of dummy nodes

int
spawn(struct service * s)
{
    pid_t child_id;

    child_id = fork(); 
    if (child_id != 0) return child_id;
    else execve(s->filename, *(s->argv), 0);
    return 0;
}

int 
process_child_exit(pid_t child_id, int status)
{
    //get service using child_id
    service = remove_service(); 
    if (service->keepalive)
    {
        if (service->restart_count < service->restart_limit)
        {
            if (spawn() == 0)
            {
                service->restart_count += 1;
                insert_service();
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
    l = next_tok();
    switch(l->tag)
    {
    case t_filename
    case t_argc
    case t_argv
    case t_keepalive
    case t_restart_limit
    }
}

int
parse_signature()
{
    ret = get_packet_data();

    if (ret == t_got_signal)
    {
        process_signal();
    }
    else
    {
        
    }
}

int
process_packet()
{
    parse_signature();
    parse_payload();
    return 0;
}

int
main_loop()
{
    int ret;

    while(running)
    {
        ret = read();
        {
            if (ret == -1)
            {
                if (got_sigchld())
                {
                    int status;
                    pid_t child_id;        

                    child_id = waitpid(-1, &status, 0);
                    process_child_exit(child_id, status);
                }
            }
            else
            {
                process_packet();
            }  
        }
    }
}

int main(int argc, char *argv[])
{
    main_loop();
}
