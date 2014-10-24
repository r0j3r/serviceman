#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include "definition_packet.h"
#include "notification.h"
#include "servctl.h"
#include "message.h"
#include "error.h"
#include "protocol.h"
#include "parse_def.h"

int run = 1;
int signum = 0;

unsigned char timeout = 0;
char * progname = "test_servctl";

int send_servctl_packet(int, struct svc_packet *, struct sockaddr_un *, socklen_t);
int load_daemons(int, struct sockaddr_un *, socklen_t);
int load_agents(int, struct sockaddr_un *, socklen_t);

void
sighandler(int num)
{
    signum = num;
    if (SIGTERM == num)
    {
        run = 0;
    }  
}

void
alarm_handler(int num)
{
    signum = num;
    timeout = 1;
}

char process_manager_run_dir[1024];
char process_manager_socket_path[124];
char servctl_socket_path[1024];

int we_are_root = 0;

int
main(int argc, char * argv[])
{
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_handler = sighandler;
    sigaction(SIGTERM, &sig, 0);

    memset(&sig, 0, sizeof(sig));
    sigfillset(&sig.sa_mask);
    sig.sa_handler = alarm_handler;
    sigaction(SIGALRM, &sig, 0);

    struct passwd * pwd = 0;
    if (argc == 2)
    {
        if (argv[1])
        {
            errno = 0;
            pwd = getpwnam(argv[1]);
            if (pwd)
            {
                fprintf(stderr, "servctl: dropping privs\n");
                if (-1 == initgroups(pwd->pw_name, pwd->pw_gid)) fprintf(stderr, "initgroups failed %s\n", strerror(errno));  
                if (-1 == setuid(pwd->pw_uid)) fprintf(stderr, "setuid %d failed %s\n", pwd->pw_uid, strerror(errno));
            }
            else
            {
                fprintf(stderr, "getpwnam user %s failed: %s\n", argv[1], strerror(errno));
            } 
        }
    }
    else
    {
        pwd = getpwnam("servctld");
        if (pwd)
        {  
            if (-1 == initgroups(pwd->pw_name, pwd->pw_gid)) fprintf(stderr, "initgroups failed %s\n", strerror(errno));
            if (-1 == setuid(pwd->pw_uid)) fprintf(stderr, "setuid %d failed %s\n", pwd->pw_uid, strerror(errno));
            pwd = 0;
        }
        else
        {
            fprintf(stderr, "getpwnam servctld failed: %s\n", strerror(errno));
        }   
    }

    if (pwd)
    {
        sprintf(process_manager_run_dir, "/run/process-manager.%s", pwd->pw_name);
        sprintf(process_manager_socket_path, "/run/process-manager.%s/procman", pwd->pw_name);
        sprintf(servctl_socket_path, "/run/process-manager.%s/test_servctl", pwd->pw_name);
    }
    else 
    {
        sprintf(process_manager_run_dir, "/run/process-manager");
        sprintf(process_manager_socket_path, "/run/process-manager/procman");
        sprintf(servctl_socket_path, "/run/process-manager/test_servctl");
        we_are_root = 1;
    }

    int sd = create_endpoint(servctl_socket_path);
    struct sockaddr_un procman_sock;
    socklen_t procman_socklen = sizeof(procman_sock);

    memset(&procman_sock, 0, sizeof(procman_sock));
    procman_sock.sun_family = AF_UNIX;
    strcpy(procman_sock.sun_path, process_manager_socket_path);

    fprintf(stderr, "test_servctl starting\n");
   
    if (-1 == sendto(sd, SERVCTL_CHECKIN, 16, 0, (struct sockaddr *)&procman_sock, 
        sizeof(procman_sock.sun_family) + strlen(procman_sock.sun_path))) //process-manager
        fprintf(stderr, "test_servctl: failed sendto servctl checkin: %s\n", strerror(errno));

    unsigned char data[1024];
    size_t len = 1024;
    if (-1 == recvfrom(sd, data, len, 0, (struct sockaddr *)&procman_sock, &procman_socklen)) //process-manager
        fprintf(stderr, "test_servctl: failed recieve ack: %s\n", strerror(errno));

    if (-1 == sendto(sd, SERVCTL_ACK_ACK, 16, 0, (struct sockaddr *)&procman_sock, procman_socklen)) //process-manager
        fprintf(stderr, "test_servctl: failed to send ack ack: %s\n", strerror(errno));

    if (we_are_root)
        load_daemons(sd, &procman_sock, procman_socklen);
    else
        load_agents(sd, &procman_sock, procman_socklen);

    //send_message_un(sd, m, &procman_sock, procman_socklen); 
    //get_reply(sd, &procman_sock, procman_socklen);

    while(run)
        pause();

    unlink(servctl_socket_path);
 
    return 0;
}

int
send_servctl_packet(int sd, struct svc_packet * p, struct sockaddr_un * u, socklen_t l)
{
    struct request * req = create_servctl_request(CREATE, PROC_LIST_RESOURCE, p);

    struct message * m = create_message((unsigned char *)req, sizeof(*req) + req->data_len);
    free(req);
 
    if (-1 == sendto(sd, m, sizeof(*m) + m->payload_len, 0, (struct sockaddr *)u, l)) //process-manager
        fprintf(stderr, "servctld: failed to send servctl packet: %s\n", strerror(errno));
    free(m);
    return 0;
}

int 
load_daemons(int sd, struct sockaddr_un * u, socklen_t l)
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

        strcpy(path, "/etc/daemons/");
        strcat(path, service->d_name);
        if (stat(path, &st_buf) == 0)
        {
            if (S_ISREG(st_buf.st_mode))
            { 
                p = parse_def_file(path);
                send_servctl_packet(sd, p, u, l);
            }
        }
    }
    closedir(services_dir);
    return 0;
}

int 
load_agents(int sd, struct sockaddr_un * u, socklen_t l)
{
    DIR * services_dir;
    struct svc_packet * p;
    struct dirent * service;

    services_dir = opendir("/etc/agents");
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

        strcpy(path, "/etc/agents/");
        strcat(path, service->d_name);
        if (stat(path, &st_buf) == 0)
        {
            if (S_ISREG(st_buf.st_mode))
            { 
                p = parse_def_file(path);
                send_servctl_packet(sd, p, u, l);
            }
        }
    }
    closedir(services_dir);
    return 0;
}
