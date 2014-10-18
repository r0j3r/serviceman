#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h> 
#include <stdio.h>

struct time_state
{
   int sec;
   int min;
   int hour;
   int mday;
   int mon;
   int year;
   int wday;
   int yday;
};

unsigned short mon_end_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int field_cmp(int , int);
struct timeval * next_timeout(struct timeval *, struct time_state *);
int day_field_cmp(struct time_state *, struct time_state *);

int
main()
{
    struct time_state test_spec = {10, 3, 11, 15, -1, -1, 4, -1};
    struct timeval now;
    gettimeofday(&now, 0);  
    struct timeval * timeout = next_timeout(&now, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    timeout = next_timeout(timeout, &test_spec);
    
    return 0;
}

struct timeval *
next_timeout(struct timeval * now, struct time_state * n)
{
    struct time_state * s = malloc(sizeof(*s));
    
    unsigned long running = 1;

    struct tm * t_now = gmtime(&now->tv_sec);
    s->sec = t_now->tm_sec; 
    s->min = t_now->tm_min; 
    s->hour = t_now->tm_hour; 
    s->mday = t_now->tm_mday; 
    s->mon = t_now->tm_mon; 
    s->year = t_now->tm_year; 
    s->wday = t_now->tm_wday; 
    s->yday = t_now->tm_yday; 
    running = now->tv_sec;
    int loop_count = 0;
    while(running >= 0)
    {
        loop_count++;
        running++;
        s->sec++;
        if (s->sec > 59)
        {
            s->sec = 0;
            s->min++;
            if (s->min > 59)
            {
                s->min = 0;
                s->hour++;
                if (s->hour > 23)
                {
                    s->hour = 0;
                    s->mday++;
                    if (s->mday > mon_end_days[s->mon])
                    {
                        s->mday = 1;
                        s->mon++;
                        if (s->mon > 11)
                        {
                            s->mon = 0;
                        } 
                    }
                    s->wday++;
                    if (s->wday > 6)
                    {
                        s->wday = 0;
                    }
                    s->yday++; 
                    if (s->yday > 365)
                    {
                        s->yday = 0;
                        s->year++;
                    } 
                }
            }  
        }
        if (day_field_cmp(s, n)
            && field_cmp(s->sec, n->sec)
            && field_cmp(s->min, n->min)
            && field_cmp(s->hour, n->hour)
            && field_cmp(s->mon, n->mon)
            && field_cmp(s->year, n->year))
        {
            fprintf(stderr, "%d:%d:%d mday %d mon %d year %d wday %d yday %d: %d loops\n", s->sec, s->min, s->hour + 1, 
                s->mday, s->mon + 1, s->year + 1900, s->wday, s->yday, loop_count);
            break;
        }
    }
    now->tv_sec = running;
    now->tv_usec = 0;
    return now;  
}

struct timeline_tree_mon
{
    unsigned char days[32];
}

struct timeline_tree
{
    unsigned int year;
    unsigned int mon[12];
    unsigned int mday[366];
    unsigned int yday[366];
    unsigned int hour[24];
    unsigned int min[60];
    unsigned int sec[60];
};

struct timeval *
next_start(struct timeval * now, struct time_state * n)
{
    unsigned int cur pos[6];
    struct timeline_tree t_line;
    memset(&pos, 0, sizeof(pos));
    memset(&t_line, 0, sizeof(t_line));

    t_line.year = n->year;

    if (n->mon >= 0)
    {
        if (n->mon > s->mon)
            t_line.mon[n->mon] = 1;
    }
    else
    {
        for(i = s->mon; i < 12; i++)
        {
            t_line_mon[i] = 1;
        }
    }
    if (n->mday > 0)
    { 
        int mon = s->mon;
        int md = s->mday;
        for(i = s->yday; i < 366; i++)
        {
            if (md == n->mday)
            {
                yday[i] = mon;
            }
            md++;
            if (md > mon_end_days[s->mon])
            {
                mon++; 
                md = 0;
            }
        }
    }
    if (n->wday > 0)
    { 
        int mon = s->mon;
        int wd = s->wday
        int md = s->mday; 
        for(i = s->yday; i < 366; i++)
        {
            if (wd == n->wday)
            {
                yday[i] = mon;
            }
            w_d++;
            if (w_d > 6)
                w_d = 0
            md++;
            if (md > mon_end_days[mon])
            {
                mon++; 
                md = 0;
            }
        }
    }

    if ((n->wday < 0) && (n-mday < 0))
    {
        int mon = s->mon;
        int md = s->mday;
        for(int i = s->yday; i < 366; i++)
        {
            yday[i] = mon;
            md++;
            if (md > mon_end_days[s->mon])
            {
                mon++; 
                md = 0;
            }
        }
    }

    if (n->hour > 0)
    {
        t_line.hour[n->hour];
    }
    else
    {
        for(i = s->hour; i < 24; i++)
            t_line.hour = 1;
    }

    if (n->min > 0)
    {
        t_line.min[n->min] = 1;
    }
    else
    {
        for(i = s->min; i < 60; i++)
            t_line.min[i] = 1;
    }

    if (n->sec > 0)
    {
        t_line.sec[n->sec] = 1;
    }
    else
    {
        for(i = s->min; i < 60; i++)
            t_line.sec[i] = 1;
    }

        
      
}


int
field_cmp(int live, int spec)
{
    if (-1 == spec) return 1;
    else return spec == live;
}

int
day_field_cmp(struct time_state * live, struct time_state * spec)
{
    if ((spec->yday < 0) && (spec->mday < 0) && (spec->wday < 0))
    {
        return 1;
    } 
    else
    {
        return (spec->yday == live->yday) 
            || (spec->mday == live->mday)
            || (spec->wday == live->wday);
    }
}
