struct child_process
{
    struct child_process * next; 
    pid_t pid;
    char * exec_file_path;
    char ** argv;
    unsigned char login_session;
    char * label;
    unsigned char * keepalive_opts;
    unsigned int throttle_count;
    struct timeval last_restart;
    unsigned char * data_buf;
    struct cron_spec * start_calendar_interval;
    char * username;
    char * groupname;
};

struct child_process * make_child_proc(struct svc_packet *);
void destroy_proc_struct(struct child_process *); 
