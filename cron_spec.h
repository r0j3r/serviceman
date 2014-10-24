struct cron_spec
{
   unsigned char sec_flag;
   unsigned char sec[60];
   unsigned char min_flag;
   unsigned char min[60];
   unsigned char hour_flag;
   unsigned char hour[24];
   unsigned char mday_flag;
   unsigned char mday[32];
   unsigned char mon_flag;
   unsigned char mon[12];
   unsigned char year_flag;
   int year;
   unsigned char wday_flag;
   unsigned char wday[7];
   unsigned char yday_flag;
   unsigned char yday[366];
};
