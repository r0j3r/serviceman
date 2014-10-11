#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include "definition_packet.h"
#include "parse_def.h"

int
print_packet(struct svc_packet * p)
{
    char * s = (char *)p + p->stringtab;
    printf("label %s\n", p->label + s);
    printf("program %s\n", p->exec_file_path_offset + s);
    printf("program argc %d\nargv ", p->argv_count);
    unsigned int * arg_array = (unsigned int *)((char *)p + p->argv_offset);
    for(int i = 0; i < p->argv_count ;i++)
    {
        printf("%s ", s + arg_array[i]); 
    }
    printf("\nlogin_session %s\n", p->login_session ? "true":"false");
    if (p->keepalive_opts_offset)
    {
        unsigned char * keepalive_opts = (unsigned char *)p + p->keepalive_opts_offset;
        for(int i = 0; keepalive_opts[i] ; i += 2)
        {
           printf("%d: %d:%d ", i, keepalive_opts[i], keepalive_opts[i + 1]);
        }
    }
    printf("\n");
    return 0;      
}

int 
load_daemons(void)
{
    DIR * services_dir;
    struct svc_packet * p;
    struct dirent * service;

    services_dir = opendir("/etc/daemons");
    if (services_dir == 0)
        return -1;

    while((service = readdir(services_dir)))
    {
        if (service->d_name[0] == '.')
            continue;
        if (strchr(service->d_name, '~'))
            continue;

        char path[FILENAME_MAX];
        struct stat st_buf;

        printf("%s\n", service->d_name);
        strcpy(path, "/etc/daemons/");
        strcat(path, service->d_name);
        if (stat(path, &st_buf) == 0)
        {
            if (S_ISREG(st_buf.st_mode))
            { 
                p = parse_def_file(path);
                print_packet(p);
            }
        }
    }
    closedir(services_dir);
    return 0;
}

int
main()
{
    load_daemons();
}
