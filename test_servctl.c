#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
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

int
main(void)
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

    int sd = create_endpoint((unsigned char *)"/run/process-manager/test_servctl");
    struct sockaddr_un procman_sock;
    socklen_t procman_socklen = sizeof(procman_sock);

    memset(&procman_sock, 0, sizeof(procman_sock));
    procman_sock.sun_family = AF_UNIX;
    strcpy(procman_sock.sun_path, "/run/process-manager/procman");

    fprintf(stderr, "test_servctl starting\n");
   
    sendto(sd, SERVCTL_CHECKIN, 16, 0, (struct sockaddr *)&procman_sock, 
        sizeof(procman_sock.sun_family) + strlen(procman_sock.sun_path)); //process-manager
    fprintf(stderr, "test_servctl: sent checkin\n");
    unsigned char data[1024];
    size_t len = 1024;
    if (-1 == recvfrom(sd, data, len, 0, (struct sockaddr *)&procman_sock, &procman_socklen)) //process-manager
        fprintf(stderr, "test_servctl: failed recieve ack: %s\n", strerror(errno));
    else
        fprintf(stderr, "test_servctl: recieved ack\n");

    if (-1 == sendto(sd, SERVCTL_ACK_ACK, 16, 0, (struct sockaddr *)&procman_sock, procman_socklen)) //process-manager
        fprintf(stderr, "test_servctl: failed to send ack ack: %s\n", strerror(errno));
    else
        fprintf(stderr, "test_servctl: sent ack ack\n");

    load_daemons(sd, &procman_sock, procman_socklen);

    //send_message_un(sd, m, &procman_sock, procman_socklen); 
    //get_reply(sd, &procman_sock, procman_socklen);

    while(run)
        pause();

    unlink("/run/process-manager/test_servctl");
 
    return 0;
}

int
send_servctl_packet(int sd, struct svc_packet * p, struct sockaddr_un * u, socklen_t l)
{
    struct request * req = create_servctl_request(CREATE, PROC_LIST_RESOURCE, p);

    struct message * m = create_message(req, sizeof(*req) + req->data_len);
    free(req);
 
    if (-1 == sendto(sd, m, sizeof(*m) + m->payload_len, 0, (struct sockaddr *)u, l)) //process-manager
        fprintf(stderr, "test_servctl: failed to send packet: %s\n", strerror(errno));
    else
        fprintf(stderr, "test_servctl: sent packet\n");
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

        printf("%s\n", service->d_name);
        strcpy(path, "/etc/daemons/");
        strcat(path, service->d_name);
        if (stat(path, &st_buf) == 0)
        {
            if (S_ISREG(st_buf.st_mode))
            { 
                p = parse_def_file(path);
                printf("sending packet\n");
                send_servctl_packet(sd, p, u, l);
            }
        }
    }
    closedir(services_dir);
    return 0;
}
