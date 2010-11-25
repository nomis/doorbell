#define xerror(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define cerror(msg, expr) do { if (expr) xerror(msg); } while(0)

#define tv_to_ull(x) (unsigned long long)((unsigned long long)(x).tv_sec*1000000 + (unsigned long long)(x).tv_usec)

/* 10ms / 10000µs */
#define MIN_PRESS 10000

#ifdef VERBOSE
# ifdef FORK
#  define SYSLOG
#  define _printf(...) syslog(LOG_INFO, __VA_ARGS__)
# else
#  define _printf(...) printf(__VA_ARGS__)
# endif
#else
# define _printf(...) do { } while(0)
#endif

void select_doorbell(const char *value);
bool press_on(const struct timeval *on);
bool press_off(const struct timeval *on, const struct timeval *off);
bool press_on_off(const struct timeval *on, const struct timeval *off);
bool press_cancel(const struct timeval *on);
bool press_resume(const struct timeval *on);
