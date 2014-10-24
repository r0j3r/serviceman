#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h> 
#include <stdio.h>
#include "cron_spec.h"

struct time_state
{
   unsigned char sec;
   unsigned char min;
   unsigned char hour;
   unsigned char mday;
   unsigned char mon;
   int year;
   unsigned char wday;
   unsigned short yday;
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
struct timeval * next_start(struct timeval *, struct cron_spec *);

struct timeline_tree_mon
{
    unsigned char days[32];
};

struct timeline_tree
{
    unsigned int year;
    unsigned char mday[366];
    unsigned char mon[366];
    unsigned char yday[366];
    unsigned char hour[24];
    unsigned char min[60];
    unsigned char sec[60];
};

void
init_timeline(struct timeline_tree * t_line, struct cron_spec * n)
{
    unsigned int cur[6];
    memset(&cur, 0, sizeof(cur));

    int year = t_line->year;
    memset(t_line, 0, sizeof(*t_line));
    t_line->year = year;

    unsigned short * days_in_month;
    unsigned char active_mon[12];

    if (t_line->year == 0)
    {
        if (n->year > 0)
            t_line->year = n->year - 1900;
        else
        {
            struct timeval now;
            gettimeofday(&now, 0);
            struct tm * g = gmtime(&now.tv_sec);
            t_line->year = g->tm_year;
        }  
    }

    if(leap_year(t_line->year + 1900))
        days_in_month = leap_mon_end_days;
    else
        days_in_month = mon_end_days;

    memset(active_mon, 0, sizeof(active_mon));

    if (n->mon_flag)
    {
        for(int i = 0; i < 12; i++)
        {
            if (n->mon[i])
                active_mon[i] = 1;
        } 
    }
    else
    {
        for(int i = 0; i < 12; i++) active_mon[i] = 1;
    } 

    if (n->mday_flag)
    { 
        int mon = 0;
        int md = 1;

        for(int i = 0; i < 366; i++,md++)
        {
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 1;
            }
            if (mon < 12)
            { 
                if (n->mday[md])
                {  
                    if (active_mon[mon])
                    {
                        t_line->yday[i] = 1;
                        t_line->mday[i] = md;
                        t_line->mon[i] = mon; 
                    }
                }
            } 
        }
    }

    if (n->wday_flag)
    { 
        int mon = 0;
        int w_d = dow(t_line->year + 1900, 1, 1);
        int md = 1; 
        for(int i = 0; i < 366; i++, w_d++, md++)
        {
            if (w_d > 6)
                w_d = 0;
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 1;
            }
            if (mon < 12)
            {
                if (n->wday[w_d])
                {  
                    if (active_mon[mon])
                    {
                        t_line->yday[i] = 1;
                        t_line->mday[i] = md;
                        t_line->mon[i] = mon; 
                    }
                }
            }
        }
    }

    if ((n->wday_flag == 0) && (n->mday_flag == 0))
    {
        int mon = 0;
        int md = 1;
        for(int i = 0; i < 366; i++)
        {
            if (active_mon[mon])
            {
                t_line->yday[i] = 1;
            }
            md++;
            if (md > days_in_month[mon])
            {
                mon++; 
                md = 1;
            }
        }
    }

    if (n->hour_flag)
    {
        for(int i = 0; i < 24; i++)
        {
            if (n->hour[i])
                 t_line->hour[i] = 1;
        }
    }
    else
    {
        for(int i = 0; i < 24; i++)
            t_line->hour[i] = 1;
    }

    if (n->min_flag)
    {
        for(int i = i; i < 60; i++)
        {
            if (n->min[i])
                t_line->min[i] = 1;
        }
    }
    else
    {
        for(int i = 0; i < 60; i++)
            t_line->min[i] = 1;
    }

    if (n->sec_flag)
    {
        for(int i = 0; i < 60; i++)
        {
            if (n->sec[i])
                t_line->sec[i] = 1;
        }
    }
    else
    {
        for(int i = 0; i < 60; i++)
            t_line->sec[i] = 1;
    }
}

struct timeval *
next_start(struct timeval * c, struct cron_spec * n)
{
    struct timeline_tree tline;
    memset(&tline, 0, sizeof(tline));
    struct tm * t_now = gmtime(&c->tv_sec);
    struct time_state * s = malloc(sizeof(*s));
    s->sec = t_now->tm_sec; 
    s->min = t_now->tm_min; 
    s->hour = t_now->tm_hour; 
    s->mday = t_now->tm_mday; 
    s->mon = t_now->tm_mon; 
    s->year = t_now->tm_year; 
    s->wday = t_now->tm_wday; 
    s->yday = t_now->tm_yday;
    tline.year = s->year;  

    struct time_state cur;
    memset(&cur, 0, sizeof(cur));
    cur.year = tline.year; 
    while(1)
    {
        init_timeline(&tline, n);
        if ((n->year > 0) & (s->year > n->year)) return 0;
        int i;

        cur.yday = s->yday;
        while(1)
        {
            for(i = cur.yday; (i < 366) & (tline.yday[i] == 0); i++);
            if (i >= 366) break;
            else if (i > s->yday)
            {
                s->hour = 0;
                s->min = 0;
                s->sec = 0;
            }
            cur.yday = i;
            cur.hour = s->hour;
            while(1)
            {
                for (i = cur.hour; (i < 24) & (tline.hour[i] == 0); i++);
                if (i >= 24) break;
                else if (i > s->hour)
                {
                    s->min = 0;
                    s->sec = 0;
                }
                cur.hour = i;  
                cur.min = s->min;
                while(1)
                {
                    for(i = cur.min; (i < 60) & (tline.min[i] == 0); i++);
                    if (i >= 60) break;
                    else if (i > s->min)
                    {
                        s->sec = 0;
                    } 
                    cur.min = i;
                    cur.sec = s->sec;
                    while(1)
                    {
                        for(i = cur.sec; (i < 60) & (tline.sec[i] == 0); i++);
                        if (i >= 60) break;
                        cur.sec = i;
                        struct tm t_out;
                        memset(&t_out, 0, sizeof(t_out));
                        t_out.tm_sec = cur.sec;
                        t_out.tm_min = cur.min; 
                        t_out.tm_hour = cur.hour;
                        t_out.tm_mday = tline.mday[cur.yday];
                        t_out.tm_mon = tline.mon[cur.yday];
                        t_out.tm_year = tline.year;
                        struct timeval * tval_out = malloc(sizeof(*tval_out));
                        tval_out->tv_sec = mktime(&t_out);
                        tval_out->tv_usec = 0;
 
                        if (tval_out->tv_sec <= c->tv_sec)
                        {
                            cur.sec++;
                        }
                        else
                        {
                            free(s); 
                            return tval_out;
                        } 
                    }
                    cur.min++;
                }
                cur.hour++; 
            }
            cur.yday++;  
        }
        tline.year++;
        if ((tline.year + 1900) > 2037 ) break;
        cur.year = tline.year; 
        memset(s, 0, sizeof(*s));
        s->year = tline.year;
    }
    free(s);
    return 0;
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
