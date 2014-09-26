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

int write_to_control_file(char *, char *);
void init_mtab(void);
void sulogin(void);

int
main()
{
    unsigned char running =1;
    pid_t modprobe_pid, mkswap_pid, child_pid, fsck_pid, mkfs_pid, swapon_pid;
    int status = 0, destlen = 0, fsck_exit_code = 0, reply_packet = 0;

    (void)fprintf(stderr, "volume-manager starting...\n");

    if (-1 == mount("none", "/proc", "proc", 0, 0))
    {
        (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
    }
    if (-1 == mount("none", "/sys", "sysfs", 0, 0))
    {
        (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
    }

    if (-1 == mount("none", "/run", "tmpfs", 0, 0))
    {
        (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
    }

    mkdir("/dev/pts", 0755);
    if (-1 == mount("none", "/dev/pts", "devpts", 0, "mode=620"))
    {
        (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
    }

    mkdir("/dev/shm", 0755);
    if (-1 == mount("none", "/dev/shm", "tmpfs", 0, 0))
    {
        (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
    }    

    modprobe_pid = fork();
    if (0 == modprobe_pid)
    {
        char * argv[4];
        argv[0] = "modprobe";
        argv[1] = "zram";
        argv[2] = "num_devices=3";
        argv[3] = 0;

        execv("/sbin/modprobe", argv);
        _exit(4);
    }     
    child_pid = wait(&status);    

    int zram0_err = write_to_control_file("512M", "/sys/block/zram0/disksize");
    int zram1_err = write_to_control_file("512M", "/sys/block/zram1/disksize");
    int zram2_err = write_to_control_file("512M", "/sys/block/zram2/disksize");

    if (0 == zram2_err)
    {
        mkfs_pid = fork();
        if (0 == mkfs_pid)
        {
            char * argv[7];
            argv[0] = "mkfs";
            argv[1] = "-t";
            argv[2] = "ext4";
            argv[3] = "-O";
            argv[4] = "^has_journal";
            argv[5] = "/dev/zram2";
            argv[6] = 0;

            execv("/sbin/mkfs", argv);
            _exit(4);
        }
        child_pid = wait(&status);

        if (-1 == mount("/dev/zram2", "/tmp", "ext4", 0, 0))
        {
            (void)fprintf(stderr, "volume-manager: mount failed %s\n", strerror(errno));
        }
    }

    if (0 == zram0_err)
    {
        mkswap_pid = fork();
        if (0 == mkswap_pid)
        {
            char * argv[3];
            argv[0] = "mkswap";
            argv[1] = "/dev/zram0";
            argv[2] = 0;

            execv("/sbin/mkswap", argv);
           _exit(4);
        }     
        child_pid = wait(&status);
        swapon("/dev/zram0", ((1000 << SWAP_FLAG_PRIO_SHIFT) & SWAP_FLAG_PRIO_MASK) | SWAP_FLAG_PREFER);
    }
     
    if (0 == zram1_err)
    {
        mkswap_pid = fork();
        if (0 == mkswap_pid)
        {
            char * argv[3];
            argv[0] = "mkswap";
            argv[1] = "/dev/zram1";
            argv[2] = 0;

            execv("/sbin/mkswap", argv);
           _exit(4);
        }
        child_pid = wait(&status);
        swapon("/dev/zram1", ((1000 << SWAP_FLAG_PRIO_SHIFT) & SWAP_FLAG_PRIO_MASK) | SWAP_FLAG_PREFER);
    }
    
    swapon_pid = fork();
    if (0 == swapon_pid)
    {
        char * argv[3];
        argv[0] = "swapon";
        argv[1] = "-a";
        argv[2] = 0;

        execv("/sbin/swapon", argv);
        _exit(4);
    }     
    child_pid = wait(&status);

    fsck_pid = fork();
    if (0 == fsck_pid)
    {
        char * argv[4];
        argv[0] = "fsck";
        argv[1] = "-A";
        argv[2] = "-p";
        argv[3] = 0;

        execv("/sbin/fsck", argv);
        _exit(4);
    }     

    child_pid = wait(&status);
    if (WIFEXITED(child_pid))
    {
        fsck_exit_code = WEXITSTATUS(status);
        switch(fsck_exit_code)
        {
        case 0:
            break; 
        case 1:
            (void)fprintf(stderr, "filesystem errors corrected\n");
            break;
        case 2:
            (void)fprintf(stderr, "errors detected system needs rebooting\n");
            break;
        case 4:
            (void)fprintf(stderr, "filesystem errors uncorrected\n");
            break;
        case 8:
            (void)fprintf(stderr, "Operational error... MEH?\n");
            break;
        case 16:
            (void)fprintf(stderr, "fsck syntax error\n");
            break;
        case 32:
            (void)fprintf(stderr, "fsck cancelled by user\n");
            break;
        case 128:
            (void)fprintf(stderr, "fsck exited without errors\n");
            break;
        default:
            (void)fprintf(stderr, "invalid exit code\n");  
        }
    }

    if (fsck_exit_code  < 2)
    {
        if (-1 == mount("", "/", "", MS_REMOUNT, 0))
        {
            (void)fprintf(stderr, "volume-manager: remount root rw failed: %s\n", strerror(errno));
        } 
        else
        {
            chmod("/tmp", S_ISVTX| S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP 
                | S_IROTH | S_IWOTH | S_IXOTH );
            reply_packet = 0;

            init_mtab();

            int mount_pid = fork();
            if (0 == mount_pid)
            { 
                char * argv[5];
                argv[0] = "mount";
                argv[1] = "-a";
                argv[2] = "-v";
                argv[3] = "-tnonfs";
                argv[4] = 0;

                execv("/bin/mount", argv);
                _exit(4); 
            }
            (void)wait(&status); 
                        
        }
    }
    else
    {
        sulogin();
    }

    int endp;
    char endpoint_name[] = "/run/volume-manager-socket";
 
    endp = create_endpoint(endpoint_name);
    if (endp < 0)
    {
        fprintf(stderr, "failed to create endpoint\n");
        unlink(endpoint_name);
        endp = create_endpoint(endpoint_name);
        if (endp < 0)
        {
            fprintf(stderr, "failed to create endpoint\n");
        }
    }
    if (endp > 0)
    {
        while(running) 
        {
            unsigned char data[1024];
            int ret;
            int bytes_read = 0;  
            struct request *request = (struct request *)data;

            ret = read(endp, data + bytes_read, sizeof(data) - bytes_read);
            if (ret > 0)
            {
                bytes_read += ret;
                while (bytes_read > sizeof(struct request))
                {
                    if (bytes_read > request->payloadlen)
                    {
                        if (request->op == READ)
                        {
                            if (0 == memcmp(request->resource_id, ROOT_STATUS, sizeof(ROOT_STATUS)))
                            {
                                struct sockaddr_un dest;
                                memset(&dest, 0, sizeof(dest));
                                memcpy(dest.sun_path, request->payload, request->payloadlen);
                                dest.sun_family = AF_UNIX;
                            
                                destlen = sizeof(dest.sun_family) + strlen(dest.sun_path) + 1;
                                sendto(endp, &reply_packet, sizeof(reply_packet), MSG_CONFIRM, 
                                    (struct sockaddr *)&dest, destlen);                                
                            }    
                        }
                        if (bytes_read > (sizeof(*request) + request->payloadlen))
                        {
                            memmove(data, data + bytes_read, sizeof(data) - bytes_read);
                        }
                        bytes_read = bytes_read - (sizeof(*request) + request->payloadlen);
                    }
                }
            }
            if (ret < 0)
            {    
                (void)fprintf(stderr, "read failed: %s\n", strerror(errno));
                break;
            }  
        }
    }
    unlink("/run/volume-manager-socket");
    (void)fprintf(stderr, "volume-manager: bailing out"); 
}

int
write_to_control_file(char * val, char * cfile)
{
    int fd = 0, bytes_written = 0;
    unsigned char write_done = 0, file_open = 0;
    int tries = 0, err = 0;

    tries = 3;
    while(tries)
    {
        fd = open(cfile, O_RDWR);
        if (fd < 0)
        {
            (void)fprintf(stderr, "failed to open control file %s: %s\n", cfile, strerror(errno));
            write_done = 1;
            tries = 0;
            file_open = 0; 
        }
        else
        {
            file_open = 1;
            write_done = 0;
        } 
        while(!write_done)
        {
            int len = strlen(val);
            bytes_written = 0; 
            int ret = write(fd, val + bytes_written, len - bytes_written);
            if (ret > 0)
            {
                bytes_written += ret; 
                if (bytes_written == len)
                {
                    write_done = 1;
                } 
            }
            else if (ret == 0)
            {
                (void)fprintf(stderr, "no bytes written\n"); 
            }
            else
            {
                (void)fprintf(stderr, "write failed: %s\n", strerror(errno));
                if (EINTR != errno) 
                {
                    write_done = 1;
                }
            }
        }
        while(file_open)
        {
            int ret = close(fd);
            if (ret < 0)
            {
                (void)fprintf(stderr, "failed to close control file: %s\n", strerror(errno));
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
    return err; 
}

void
init_mtab(void)
{
    int mtab_fd = open("/etc/mtab", O_RDWR | O_TRUNC);
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

void
sulogin(void)
{
    return;
}
