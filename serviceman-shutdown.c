#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <string.h>
#include <mntent.h>

int unmounted_all(void);

int
main(int argc, char * argv[])
{
    int reboot_cmd = RB_POWER_OFF;
  
    if (argc == 2)
    {
        if (0 == memcmp(argv[1], "-r", 2))
        {
            reboot_cmd = RB_AUTOBOOT;  
        }
    }

    fprintf(stderr, "sending SIGTERM to all processes...\n");
    if (-1 == kill(-1, SIGTERM))
    {
        fprintf(stderr, "process-manager: kill failed: %s\n", strerror(errno));
    }

    fprintf(stderr, "sending SIGKILL to all processes...\n");
    if (-1 == kill(-1, SIGKILL))
    {
        fprintf(stderr, "process-manager: kill failed: %s\n", strerror(errno));
    }

    if (-1 == mount("", "/", 0, MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount root read only: %s\n", strerror(errno));
    }

    sync();

    unsigned char root_busy = 5;
    while(root_busy)
    {
        if (unmounted_all()) 
        {
            root_busy = 0;
        } 
        else
        {
            kill(-1, SIGKILL); 
            root_busy--;
            fprintf(stderr, "process-manager: retrying unmount\n");
        }
    }

    if (-1 == umount("/proc"))
    { 
        fprintf(stderr, "failed to umount /proc: %s\n", strerror(errno));
        sleep(1);
    }

    if (-1 == umount("/dev"))
    { 
        fprintf(stderr, "failed to umount /dev: %s\nremounting read only...\n", strerror(errno));
        if (-1 == mount("", "/dev", 0, MS_REMOUNT | MS_RDONLY, 0))
        { 
            fprintf(stderr, "failed to remount /dev read only: %s\n", strerror(errno));
            sleep(1);
        }
    }

    if (-1 == mount("", "/", 0, MS_REMOUNT | MS_RDONLY, 0))
    { 
        fprintf(stderr, "failed to remount root read only: %s\n", strerror(errno));
        sleep(1);
    }

    sync();

    if (-1 == umount("/"))
    { 
        fprintf(stderr, "failed to umount /: %s\n", strerror(errno));
        sleep(1);
    }

    reboot(reboot_cmd); 
    while(1);
        pause();      
}

int
unmounted_all(void)
{
    FILE * mounts_file;

    fprintf(stderr, "trying to unmount all partitions...\n");
    int tries = 10, to_unmount = 0, unmounted = 0;
    while(tries)
    {
        mounts_file = setmntent("/proc/mounts", "r");
        if (mounts_file)
        {
            struct mntent * entry;
            while((entry = getmntent(mounts_file)))
            {
                if (0 == strcmp(entry->mnt_dir, "/"))
                    continue;
                else if (0 == strcmp(entry->mnt_dir, "/proc"))
                    continue;
                else if (0 == strcmp(entry->mnt_dir, "/dev"))
                    continue;                
                else 
                {
                    fprintf(stderr, "trying to unmount %s\n", entry->mnt_dir);
                    to_unmount++; 
                    if (0 == umount (entry->mnt_dir))
                    {
                        unmounted++;
                    }
                    else
                    {
                        fprintf(stderr, "unmount failed: %s\n", strerror(errno));
                        if (-1 == mount("", entry->mnt_dir, 0, MS_REMOUNT|MS_RDONLY, 0))
                        {
                            fprintf(stderr, "remount %s readonly failed: %s\n", entry->mnt_dir, strerror(errno));
                        }   
                    }
                }        
            }
        }
        else
        {
            fprintf(stderr, "failed to open /proc/mounts: %s...\n", strerror(errno));
            sleep(1);  
        } 
        fprintf(stderr, "to_unmount = %d, unmounted = %d\n", to_unmount, unmounted);
        sleep(1);
        if (to_unmount == unmounted)
            tries = 0;
        else
        {
            to_unmount = 0;
            unmounted = 0;
            tries--;
        }
        endmntent(mounts_file);
    }
 
    return (to_unmount == unmounted);
}
