
static unsigned const char SERVCTL_CHECKIN[16] = 
{0x10, 0x0d, 0x94, 0xdc, 0x80, 0xe3, 0x43, 0x0b, 0x94, 0xc5, 0x4f, 0xfe, 0xee, 0x4a, 0xc2, 0x8a};
static unsigned const char SERVCTL_ACK[16] =
{0xd6, 0xed, 0x20, 0x8d, 0xce, 0xf1, 0x46, 0xb4, 0x8c, 0x78, 0xa4, 0x24, 0xe3, 0xf8, 0x75, 0x38};
static unsigned const char SERVCTL_ACK_ACK[16] =
{0x89, 0x64, 0x68, 0xb9, 0x67, 0xe7, 0x4b, 0xa0, 0xba, 0x80, 0x8b, 0x40, 0x07, 0x73, 0xbd, 0x52};

int ctl_checkin(unsigned char *, size_t);
int launch_control_proc(struct sockaddr_un *, socklen_t *, pid_t *);