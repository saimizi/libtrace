#ifndef __LIBFFLAG_H__
#define __LIBFFLAG_H__

int wait_flag_on(const char *flag);
int wait_flag_off(const char *flag);
int set_flag(const char *flag);
int clear_flag(const char *flag);

#endif
