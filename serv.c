struct service
{
    pid_t pid;
    struct service * next;
    unsigned char keepalive;
    unsigned int argc;
    char * filename;
    char *(*argv[]);
    char buff[];
};

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
main_loop()
{
    while(running)
    {
        ret = read()
        {
            if (ret == -1)
            {
                if (got_sigchld(ret))
                {
                    child_id = waitpid(-1, &status, 0);
                    process_child_exit(child_id);
                }
            }
            else
            {
                process_packet();
            }  
        }
    }
}

int main()
{
    main_loop();
}
