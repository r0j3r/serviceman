
struct request
{
    unsigned char protocol_id[16];
    int op; 
    unsigned char resource_id[16];
    int payloadlen;
    unsigned char payload[];
};

static const int CREATE=1;
static const int READ=2;
static const int UPDATE=3;
static const int DELETE=4;

static const char ROOT_STATUS[16] = "root_status00000";

int create_endpoint(char *);
