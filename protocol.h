extern const char * SERVCTL_SOCKET;

static const unsigned char PROTO_ID_V1[] 
    = {0x49, 0xaf, 0x99, 0xaa, 0x37, 0x18, 0x47, 0xa0, 0x87, 0x05, 
0x03, 0x9d, 0xd9, 0xb7, 0x3b, 0x25};

static const unsigned char PROTO_ID_REPLY_V1[]
    = {0x21, 0xac, 0x5a, 0x68, 0xfd, 0xe3, 0x44, 0x7f, 0x9e, 0xaf, 
    0xd5, 0xb8, 0x58, 0x43, 0x83, 0xf6};

static const unsigned int OP_CREATE = 1;
static const unsigned int OP_READ = 2;
static const unsigned int OP_UPDATE = 3;
static const unsigned int OP_DELETE = 4;

struct proto_meta
{
    unsigned char protocol_id[16];
    unsigned char op;
    unsigned char resource_id[16];
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
