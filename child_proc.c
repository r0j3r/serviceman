#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "definition_packet.h"
#include "cron_spec.h"
#include "child_proc.h"

struct child_process *
make_child_proc(struct svc_packet * p)
{
    struct child_process * proc;
    unsigned char * d = (unsigned char *)p;
    unsigned char * s;

    proc = malloc(sizeof(*proc));
    memset(proc, 0, sizeof(*proc));
    proc->data_buf = malloc(p->packet_size);
    memcpy(proc->data_buf, p, p->packet_size);
    s = proc->data_buf + p->stringtab; 
    proc->exec_file_path = s + p->exec_file_path_offset;
    proc->label = s + p->label;
    if (p->argv_count)
    {
        fprintf(stderr, "argv_count %d\n", p->argv_count);
        proc->argv = malloc(sizeof(long) * (p->argv_count + 1));
        unsigned int * packet_argv = d + p->argv_offset; 
        for(int i = 0; i < p->argv_count; i++)
        {
            proc->argv[i] = s + packet_argv[i];
            fprintf(stderr, "%s argv[%d] %s\n", proc->label, i, proc->argv[i]);
        }
        proc->argv[p->argv_count] = 0; 
    }
     
    if (p->keepalive_opts_offset)
    {
        proc->keepalive_opts = proc->data_buf + p->keepalive_opts_offset;
        fprintf(stderr, "make_child_proc: keepalive opts ");
        for(int i = 0; proc->keepalive_opts[i]; i += 2)
        {
            fprintf(stderr, "%d:%d ", proc->keepalive_opts[i], proc->keepalive_opts[i + 1]);
        }
        fprintf(stderr, "\n");
    }
    proc->login_session = p->login_session;
    return proc;
}

void
destroy_proc_struct(struct child_process * c)
{ 
    if (c->argv) free(c->argv);
    if (c->data_buf) free(c->data_buf);
    free(c);
}
