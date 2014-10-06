#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/swap.h>
#include "notification.h"

void
init_mtab(void)
{
    int mtab_fd = open("etc/mtab", O_RDWR | O_TRUNC);
    if (mtab_fd < 0)
    {
        return;
    } 
    int proc_mounts_fd = open("/proc/mounts", O_RDONLY);
    if (proc_mounts_fd < 0)
    {
        close(mtab_fd);
        return;
    }
    
    struct stat stat_buf;
    memset(&stat_buf, 0, sizeof(stat_buf));

    if (0 == fstat(proc_mounts_fd, &stat_buf))
    {
        
        unsigned char * mtab_buf = malloc(1024);
        unsigned char data_needed = 1;
        int bytes_read = 0;
 
        while(data_needed)
        {
            int ret = read(proc_mounts_fd, mtab_buf + bytes_read, 1024 - bytes_read);
            if (ret > 0)
            {
                bytes_read += ret;
                if (bytes_read == 1024)
                {
                    data_needed = 0;
                }
            }
            else if (ret < 0)
            {
                if (EINTR != errno)
                {
                    data_needed = 0;
                } 
            }
            else
            {
                data_needed = 0;
            }
        }
        close(proc_mounts_fd);
    
        if (bytes_read)
        {
            int tries = 3;
            unsigned char file_open = 1;
            unsigned char have_data = 1; 
            while(tries)
            {
                int bytes_written = 0;
                while(have_data)
                {
                    int ret = write(mtab_fd, mtab_buf + bytes_written, bytes_read - bytes_written);
                    if (ret > 0)
                    {
                         bytes_written += ret;
                         if (bytes_written == bytes_read)
                         {
                             have_data = 0;
                         } 
                    }
                    else if (ret < 0)
                    {
                        if (EINTR != errno)
                        {
                            have_data = 0;
                            tries = 0;
                        }
                    }
                } 
                while(file_open)
                {
                    int ret = close(mtab_fd);
                    if (ret < 0)
                    {
                        (void)fprintf(stderr, "failed to close mtab file: %s\n", strerror(errno));
                        if (errno == EBADF)
                        {
                            file_open = 0;
                            tries--;
                        }
                        else if (errno == EIO)
                        {
                            file_open = 0;
                            tries--; 
                        }
                    }
                    else
                    {
                        file_open = 0;
                        tries = 0;
                    }
                }
            }
        }
    }
}
