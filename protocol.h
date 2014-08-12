struct proto_meta
{
    char protocol_id[16];
    char version[4];
    char op;
    char resource_id[16];
    int payload_len;
};
