#define xerror(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define cerror(msg, expr) do { if (expr) xerror(msg); } while(0)

#ifdef VERBOSE
# define _printf(...) printf(__VA_ARGS__)
#else
# define _printf(...) do { } while(0)
#endif
