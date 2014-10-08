#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "child_proc.h"
#include "definition_packet.h"

struct child_process *
make_child_proc(struct svc_packet * p)
{
    struct child_process * proc;
    unsigned char * d = (unsigned char *)p;
    unsigned char * s = (unsigned char *)(d + p->stringtab);

    proc = malloc(sizeof(*proc));
    memset(proc, 0, sizeof(*proc));
    proc->exec_file_path = s + p->exec_file_path_offset;
    if (p->argv_count)
    {
        proc->argv = malloc(sizeof(long) * p->argv_count);
        unsigned char * packet_argv = d + p->argv_offset; 
        for(int i = 0; i < p->argv_count; i++)
        {
            proc->argv[i] = s + packet_argv[i]; 
        }
    }
    proc->keepalive_opts = d + p->keepalive_opts_offset;
    proc->label = s + p->label;
    if (p->login_session) proc->login_session = 1;
    return proc;
}
