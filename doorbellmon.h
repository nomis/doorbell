/*
 * Outputs: TIOCM_DTR(4), TIOCM_RTS(7)
 * Inputs:  TIOCM_DSR(6), TIOCM_CTS(8), TIOCM_DCD(1)
 * Useless: TIOCM_RNG(9)
 *
 * TIOCMIWAIT on TIOCM_RNG only returns on the 1->0 transition
 */
#define SERIO_OUT (TIOCM_DTR)
#define SERIO_OFF (0)
#define SERIO_IN  (TIOCM_CTS)

#define xerror(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define cerror(msg, expr) do { if (expr) xerror(msg); } while(0)

#ifdef FORK
# undef VERBOSE
#endif

#ifdef VERBOSE
# define _printf(...) printf(__VA_ARGS__)
#else
# define _printf(...) do { } while(0)
#endif
