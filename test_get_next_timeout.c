#include <string.h>
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

unsigned short mon_end_days[] = 
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
unsigned short mon_start_ydays[] = 
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
unsigned short leap_mon_end_days[] = 
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
unsigned short leap_mon_start_ydays[] = 
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
int field_cmp(int , int);
struct timeval * next_timeout(struct timeval *, struct time_state *);
int day_field_cmp(struct time_state *, struct time_state *);
int dow(int y, int m, int d);
int leap_year(int);

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
};

struct timeline_tree
{
    unsigned int year;
    unsigned int yday[366];
    unsigned int hour[24];
    unsigned int min[60];
    unsigned int sec[60];
};

struct timeline_tree timeline;

void
init_timeline(struct timeline_tree * t_line, struct time_state * n)
{
    unsigned int cur[6];
    memset(&cur, 0, sizeof(cur));
    memset(&t_line, 0, sizeof(t_line));
    unsigned char days_of_week[366];
    unsigned short * days_in_month;
    unsigned char active_mon[12];

    t_line->year = n->year - 1900;

    if(leap_year(n->year))
        days_in_month = leap_mon_end_days;
    else
        days_in_month = mon_end_days;

    if (n->mon > 0)
    {
        active_mon[n->mon] = 1; 
    }
    else
    {
        for(int i = 0; i < 12; i++) active_mon[i] = 1;
    } 

    if (n->mday > 0)
    { 
        int mon = 0;
        int md = 0;
        for(int i = 0; i < 366; i++)
        {
            if ((md == n->mday) && (active_mon[mon]))
            {
                t_line->yday[i] = 1;
            }
            md++;
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 0;
            }
        }
    }

    if (n->wday >= 0)
    { 
        int mon = 0;
        int w_d = dow(n->year, 0, 0);
        int md = 0; 
        for(int i = 0; i < 366; i++)
        {
            if ((w_d == n->wday) && (active_mon[mon]))
            {
                t_line->yday[i] = 1;
            }
            w_d++;
            if (w_d > 6)
                w_d = 0;
            md++;
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 0;
            }
        }
    }

    if ((n->wday < 0) && (n->mday < 0))
    {
        int mon = 0;
        int md = 0;
        for(int i = 0; i < 366; i++)
        {
            if (active_mon[mon])
                t_line->yday[i] = 1;
            md++;
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 0;
            }
        }
    }

    if (n->hour >= 0)
    {
        t_line->hour[n->hour] = 1;
    }
    else
    {
        for(int i = 0; i < 24; i++)
            t_line->hour[i] = 1;
    }

    if (n->min >= 0)
    {
        t_line->min[n->min] = 1;
    }
    else
    {
        for(int i = 0; i < 60; i++)
            t_line->min[i] = 1;
    }

    if (n->sec > 0)
    {
        t_line->sec[n->sec] = 1;
    }
    else
    {
        for(int i = 0; i < 60; i++)
            t_line->sec[i] = 1;
    }
}

unsigned char mon_tab[366];
unsigned char w_days[366];

struct timeval *
next_start(struct time_state * s, struct time_state * n)
{
    struct time_state cur;
    memset(&cur, 0, sizeof(cur));
    while(1)
    {
        if ((n->year > 0) && (s->year > n->year)) return 0;
        struct timeline_tree tline;
        init_timeline(&tline, n);
        int i;
    
        while(1)
        {
            for(i = 0; (i < 366) && ((i <= s->yday) || (tline.yday[i] == 0)); i++);
            if (i >= 366) break;
            cur.yday = i;
            while(1)
            {
                for(i = 0; (i < 24) && ((i <= s->hour) || (tline.hour[i] == 0)); i++);
                if (i >= 24) break;
                cur.hour = i;
                while(1)
                {
                    for(i = 0; (i < 60) && ((i <= s->min) || (tline.min[i] == 0)); i++);
                    if (i >= 24) break;
                    cur.min = i;
                    while(1)
                    {
                        for(i = 0; (i < 60) && ((i <= s->sec) || (tline.sec[i] == 0)); i++);
                        if (i >= 60) break;
                        else 
                        {
                            cur.sec = i;
                            struct tm t_out; 
                            t_out.tm_sec = cur.sec;
                            t_out.tm_min = cur.min;
                            t_out.tm_hour = cur.hour;
                            t_out.tm_yday = cur.yday;
                            t_out.tm_year = cur.year;
                            struct timeval * tval_out = malloc(sizeof(struct timeval));  
                            tval_out->tv_sec = mktime(&t_out);
                            tval_out->tv_usec = 0;
                            return tval_out; 
                        }
                        s->sec++;                        
                    }
                    s->min++; 
                    s->sec = 0;
                }
                s->hour++;
                s->min = 0;
                s->sec = 0;
            }
            s->yday++;
            s->hour = 0;
            s->min = 0;
            s->sec = 0; 
        }
        tline.year++;
        if ((tline.year + 1900) > 2037 ) break; 
        memset(s, 0, sizeof(s));
        s->year = tline.year + 1900;
    }
    return 0;
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

int dow(int y, int m, int d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int
leap_year(int y)
{
    if (y % 4)
        return 0;
    else if (y % 100)
        return 1;
    else if (y % 400)
        return 0;
    else
        return 1;
}
