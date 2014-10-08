struct svc_packet
{
    int packet_size; 
    int stringtab; //offset after start of packet
    int exec_file_path_offset; //offset in stringtab
    int argv_offset; //offset of array of offsets into stringtab
    int argv_count;
    unsigned char login_session; 
    int label; //offset into string tab
    int keepalive_opts_offset; //array of unsigned chars
};
