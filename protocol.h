extern const char * SERVCTL_SOCKET;

static const unsigned char PROTO_ID_V1[] 
    = {0x49, 0xaf, 0x99, 0xaa, 0x37, 0x18, 0x47, 0xa0, 0x87, 0x05, 
0x03, 0x9d, 0xd9, 0xb7, 0x3b, 0x25};

static const unsigned char PROTO_ID_REPLY_V1[]
    = {0x21, 0xac, 0x5a, 0x68, 0xfd, 0xe3, 0x44, 0x7f, 0x9e, 0xaf, 
0xd5, 0xb8, 0x58, 0x43, 0x83, 0xf6};

static const unsigned char JOB_LIST_ID[] 
    = {0x65, 0xcb, 0x37, 0xb4, 0xc2, 0x00, 0x46, 0xbc, 0x82, 0x70, 
0xf3, 0x60, 0xe8, 0x2e, 0x06, 0x60};

static const unsigned char RUN_VAR_ID[] 
    = {0x4b, 0x8e, 0xcf, 0xd3, 0xd4, 0x18, 0x47, 0xf4, 0x9c, 0xba, 
0x45, 0x92, 0x58, 0x5d, 0xc9, 0x26};

static const unsigned char POWEROFF_MAGIC[]
    = {0x76, 0xf7, 0x9d, 0x72, 0x46, 0x78, 0x4a, 0x0b, 0x99, 0xe4, 
0x17, 0x39, 0x15, 0x27, 0x4b, 0xf9};

static const unsigned char REBOOT_MAGIC[]
    = {0x86, 0x3d, 0xbe, 0xee, 0x11, 0x97, 0x4c, 0x44, 0x87, 0x67,
0x25, 0x87, 0x24, 0x0f, 0xe4, 0xc4}; 

 
static const unsigned int OP_CREATE = 1;
static const unsigned int OP_READ = 2;
static const unsigned int OP_UPDATE = 3;
static const unsigned int OP_DELETE = 4;

struct proto_meta
{
    unsigned char protocol_id[16];
    unsigned char op;
    unsigned char resource_id[16];
    unsigned char content_type_id[16]; //UTI?
    int payload_len;
};

struct reply
{
    unsigned char protocol_id[16];
    enum err_code err_code;
};

struct packet
{
    int packet_size;
    int stringtab_offset; //offset from start of packet
    int label; //offset in stringtab
    int program; //offset in stringtab  
    int program_args; //from start of packet; array of offsets into stringtab
    int program_args_count;
    unsigned char login_session; 
    int keepalive; //offset from start of packet
};

struct netbuff
{
    unsigned short len;
    unsigned char data[];
};

enum err_code get_reply(int sd, struct sockaddr_un *, socklen_t *);
struct request * get_servctl_request(unsigned char *, unsigned int);
