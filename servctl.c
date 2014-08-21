#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "protocol.h"

enum token
{
    STRING,
    NEWLINE,
    LABEL,
    PROGRAM,
    PROGRAMARGS,
    KEEPALIVE,
    SUCCESSFULEXIT,
    TRUE,
    FALSE,
    END_OF_FILE
};


int parse_keepalive_options(void);
struct packet * parse_def(char * buff);
struct packet * parse_def_file(char * filename);
int next_char(void);
int connect_to_service_manager();
int send_packet(unsigned char *, int);
int disconnect_from_service_manager();

int serv_sd;

int 
get_defs(const char * dirname)
{
    DIR * services_dir = opendir(dirname);
    struct packet * p;

    struct dirent * service = readdir(services_dir);
    connect_to_service_manager();
    while(service)
    {
        if (service->d_type == DT_REG)
        {
            if (service->d_name[0] != '.')
            {
                char path[NAME_MAX];
                printf("%s\n", service->d_name);
                strcpy(path, "daemons/");
                strcpy(path + 8, service->d_name);
                p = parse_def_file(path);
                printf("sending packet\n");
                send_packet((unsigned char *)p, p->packet_size);
            }
        }
        service = readdir(services_dir);
    }
    closedir(services_dir);
    disconnect_from_service_manager();
    return 0;
}

int
connect_to_service_manager()
{
    struct sockaddr_un unix_path;
    int ret;

    serv_sd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&unix_path, 0, sizeof(unix_path));
    unix_path.sun_family = AF_UNIX;
    strcpy(unix_path.sun_path, SERVCTL_SOCKET);

    ret = connect(serv_sd, (struct sockaddr *)&unix_path, sizeof(unix_path.sun_family)
        + strlen(unix_path.sun_path));
    return ret;
}

int
send_packet(unsigned char * p, int len)
{
    unsigned char pbuff[16384];
    unsigned char reply[4096];
    struct proto_meta * u;
    int ret;

    fprintf(stderr,"sending packet\n");
    memset(pbuff, 0, sizeof(pbuff));
    u = (struct proto_meta *)pbuff;
    memcpy(u->protocol_id, PROTO_ID_V1, 32);
    u->payload_len = len;
    u->op = OP_CREATE;
    memcpy(pbuff + sizeof(struct proto_meta), p, len);
 
    ret = write(serv_sd, pbuff, len + sizeof(struct proto_meta));
    if (ret < 0) return -1;
    ret = read(serv_sd, reply, sizeof(reply));
    if (ret < 0) return -1;
    return 0;
}

int
disconnect_from_service_manager()
{
    close(serv_sd);
    return 0;
}

char buff[4096];
int pos;

char lex_buff[4096];
int lex_pos;
int lex_start_pos;

char * lexeme;
int char_lookahead;
enum token token_lookahead;

enum token next_tok(void);

struct packet *
parse_def_file(char * filename)
{
    int fd;
    int ret;
    struct stat stat_buf;

    fd = open(filename, O_RDONLY);
    if (fd < 0) 
        return 0;
    ret = fstat(fd, &stat_buf);
    if (ret < 0) goto fail;
    if (stat_buf.st_size > sizeof(buff)) goto fail;
    ret = read(fd, buff, sizeof(buff));
    if (ret < 0) goto fail;
    close(fd);
    return parse_def(buff);
fail:
    printf("failed to parse file\n");
    close(fd);
    return 0;
}


unsigned char * staging_buffer;
int staging_buffer_pos = 0;
const int staging_buffer_size = 8192;

int
staging_buff_init()
{
    staging_buffer = malloc(staging_buffer_size);
    memset(staging_buffer, 0, staging_buffer_size);
    return 0;
}

void *
staging_buffer_alloc(int size)
{
    if (staging_buffer_pos < staging_buffer_size)
    {
        void * ret = staging_buffer + staging_buffer_pos;
        staging_buffer_pos += size;
        return ret;
    }
    return 0;
}

int
get_offset(unsigned char * addr)
{
    if (addr < (staging_buffer + staging_buffer_size))
        return addr - staging_buffer;
    return 0;
}

struct packet *
parse_def(char * buff)
{
    struct packet * packet; 
    int p_args[256];
    int p_argc_count = 0;

    unsigned char * p_args_staging;
    unsigned char * string_tab_staging;

    staging_buff_init();
    packet = (struct packet *)staging_buffer;
    char_lookahead = next_char();
    token_lookahead = next_tok();

    while(token_lookahead != END_OF_FILE)
    {
        if (token_lookahead == LABEL)
        {
            token_lookahead = next_tok();
            if (token_lookahead == STRING)
            {
                packet->label = lex_start_pos;
                token_lookahead = next_tok();
            }
        }
        else if (token_lookahead == PROGRAM)
        {
            token_lookahead = next_tok();
            if (token_lookahead == STRING)
            {
                packet->program = lex_start_pos;
                token_lookahead = next_tok();
            } 
        }
        else if (token_lookahead == PROGRAMARGS)
        {

            token_lookahead = next_tok();
            while (token_lookahead == STRING)
            {
                p_args[p_argc_count++] = lex_start_pos; 
            }
            p_args[p_argc_count++] = 0; 
         
            p_args_staging = staging_buffer_alloc(sizeof(int) * p_argc_count);
            if (!p_args_staging)
            {
                return 0;
            }
            memcpy(p_args_staging, p_args, sizeof(int) * p_argc_count);
            packet->program_args = get_offset(p_args_staging);  
        }
        else if (token_lookahead == KEEPALIVE)
        {
            token_lookahead = next_tok();
            packet->keepalive = parse_keepalive_options();
        }
        else
        {
            token_lookahead = next_tok();            
        } 
    }
    string_tab_staging = staging_buffer_alloc(lex_pos);
    memcpy(string_tab_staging, lex_buff, lex_pos);
    packet->stringtab_offset = get_offset(string_tab_staging); 
    packet->packet_size = staging_buffer_pos;
    return packet;   
}

int
parse_keepalive_options(void)
{
    int keepalive_options[256];
    int keepalive_pos = 0;

    int * keepalive_options_staging_buff;

    while (token_lookahead == SUCCESSFULEXIT)
    {
        token_lookahead = next_tok();
        if (token_lookahead == TRUE)
        {
            keepalive_options[keepalive_pos++] = 1;
            keepalive_options[keepalive_pos++] = 1;
            token_lookahead = next_tok();
        }
        else if (token_lookahead == FALSE)
        {
            keepalive_options[keepalive_pos++] = 1;
            keepalive_options[keepalive_pos++] = 0;
            token_lookahead = next_tok();
        }
        else
        {   
            //assume false
            keepalive_options[keepalive_pos++] = 1;
            keepalive_options[keepalive_pos++] = 0;
        } 
    }
    keepalive_options[keepalive_pos++] = 0;
    keepalive_options[keepalive_pos++] = 0;

    keepalive_options_staging_buff = malloc(sizeof(int) * keepalive_pos);
    if (!keepalive_options_staging_buff) return -1;
    memcpy(keepalive_options_staging_buff, &keepalive_options, 
        sizeof(int) * keepalive_pos);

    return get_offset((unsigned char *)keepalive_options_staging_buff);
}

int
next_char(void)
{
    if (pos >= sizeof(buff)) return 0;
    return buff[pos++];
}

enum tokenizer_state
{
    START,GOT_STRING
};

struct token_tab
{
    char * token_string;
    enum token tok_id;
};

struct token_tab tok_tab[] = 
{{"label", LABEL}, {"program", PROGRAM}, {"programargs", PROGRAMARGS},
{"keepalive", KEEPALIVE},{"successfulexit", SUCCESSFULEXIT},{"true", TRUE},
{"false", FALSE}, {0, STRING}}; 

int keywords_count = 7;
 
enum token
find_token(char * lexeme)
{
    int i;

    tok_tab[keywords_count].token_string = lexeme;
    for(i = 0; strcasecmp(tok_tab[i].token_string, lexeme); i++);
    return tok_tab[i].tok_id;
}

enum token
next_tok()
{
    enum tokenizer_state tok_state = START;

    lexeme = &(lex_buff[lex_pos]);
    lex_start_pos = lex_pos; 
    while(1)
    {
        switch(tok_state)
        {
        case START:
            switch(char_lookahead)
            {
            case 0:
                return END_OF_FILE; 
            case '\n':
               char_lookahead = next_char(); 
               return NEWLINE;
            case ' ':
                break;  
            default:
                lex_buff[lex_pos++] = char_lookahead;
                tok_state = GOT_STRING;
                break;
            }
            break;
        case GOT_STRING:
            switch(char_lookahead)
            {
            case 0:
            case ' ':
            case '\n':
                lex_buff[lex_pos++] = 0;
                return find_token(lexeme);
                break;
            default:
                lex_buff[lex_pos++] = char_lookahead;  
                break;
            }
            break;
        default:
            break;
        }
        char_lookahead = next_char();
    }
    return END_OF_FILE;
}


int
main()
{
    get_defs("daemons");
    return 0;
}
