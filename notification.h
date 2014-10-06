
struct request
{
    unsigned char protocol_id[16];
    unsigned int op; 
    unsigned char resource_id[16];
    unsigned char sender_endp[16];
    unsigned int data_len;
    unsigned char data[];
};

struct reply_buf
{
    struct reply_buf * next;
    unsigned char reply_dest[16];
    unsigned int data_len;
    unsigned char data[];
};

static const int CREATE=1;
static const int READ=2;
static const int UPDATE=3;
static const int DELETE=4;

static const unsigned char NOTIFICATION_PROTO_ID[16] = 
    {0xb5, 0x3f, 0xec, 0x2e, 0x0f, 0xcb, 0x4c, 0x74, 0xa6, 0x60, 0x30, 0x00, 0xe8, 0x42, 0x20, 0xa5};
static const int MP_ID_LEN = 16;
static const unsigned char ROOT_STATUS[16] = 
    {0xfe, 0x93, 0x44, 0xdb, 0x87, 0x06, 0x43, 0xdf, 0xb0, 0x05, 0xbf, 0xee, 0x80, 0xe9, 0xba, 0xdb};

int create_endpoint(unsigned char *);

static const unsigned char TMP_STATUS[16] =
 {0xa3, 0x09, 0xf8, 0x2e, 0x3b, 0x20, 0x45, 0x00, 0xb5, 0x0f, 0x7e, 0x3f, 0x54, 0x26, 0x76, 0x14};

int send_notify(int, unsigned char[16], unsigned char[16]);
