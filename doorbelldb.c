#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "doorbelldb.h"
#include "doorbellq.h"

#ifdef SYSLOG
# include <syslog.h>
#endif

#define PRESS_CACHE 3

char *mqueue_main;
char *mqueue_backup;
unsigned long int doorbell;
mqd_t qmain, qbackup;
bool process_on = true;
press_t press[PRESS_CACHE];
int count = 0;
#ifdef SYSLOG
char *ident;
#endif

void handle_signal(int sig);

struct sigaction sa_ign = { /* handle signals, but store them for later */
	.sa_handler = handle_signal,
	.sa_flags = 0
};
struct sigaction sa_dfl = { /* use default signal handler */
	.sa_handler = SIG_DFL,
	.sa_flags = 0
};
sigset_t die_signals;
int waiting_sig = 0;

void handle_signal(int sig) {
	if (waiting_sig == 0)
		waiting_sig = sig;
}

static void setup_syslog(void) {
#ifdef SYSLOG
	int ret;

	ident = malloc((strlen("doorbelldb") + strlen(mqueue_main) + 1) * sizeof(char));
	cerror("malloc", ident == NULL);

	ret = sprintf(ident, "doorbelldb%s", mqueue_main);
	cerror("snprintf", ret < 0);

	openlog(ident, LOG_PID, LOG_DAEMON);
#endif
}

static void setup(int argc, char *argv[]) {
	int ret;

	if (argc != 3) {
		printf("Usage: %s <mqueue> <doorbell>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	mqueue_main = argv[1];

	mqueue_backup = malloc((strlen(mqueue_main) + 2) * sizeof(char));
	cerror("malloc", mqueue_backup == NULL);

	ret = sprintf(mqueue_backup, "%s~", mqueue_main);
	cerror("snprintf", ret < 0);

	select_doorbell(argv[2]);

	setup_syslog();
}

static void signal_init(void) {
	/* only need to care about intentional kills of
	 * the process, as anything else is unrecoverable
	 */
	cerror("sigemptyset", sigemptyset(&die_signals) != 0);
	cerror("sigaddset SIGHUP", sigaddset(&die_signals, SIGHUP) != 0);
	cerror("sigaddset SIGINT", sigaddset(&die_signals, SIGINT) != 0);
	cerror("sigaddset SIGQUIT", sigaddset(&die_signals, SIGQUIT) != 0);
	cerror("sigaddset SIGTERM", sigaddset(&die_signals, SIGTERM) != 0);

	sa_ign.sa_mask = die_signals;
	cerror("sigemptyset", sigemptyset(&sa_dfl.sa_mask) != 0);
}

static void init(void) {
	struct mq_attr qmain_attr = {
		.mq_flags = 0,
		.mq_maxmsg = 4096,
		.mq_msgsize = sizeof(press_t)
	};
	struct mq_attr qbackup_attr = {
		.mq_flags = 0,
		.mq_maxmsg = PRESS_CACHE,
		.mq_msgsize = sizeof(press_t)
	};

	qmain = mq_open(mqueue_main, O_RDONLY|O_CREAT, S_IRUSR|S_IWUSR, &qmain_attr);
	cerror(mqueue_main, qmain < 0);

	qbackup = mq_open(mqueue_backup, O_RDWR|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &qbackup_attr);
	cerror(mqueue_backup, qbackup < 0);

	signal_init();
}

static void signal_hold(void) {
	cerror("sigprocmask SIG_BLOCK", sigprocmask(SIG_BLOCK, &die_signals, NULL) != 0);
}

static void try_signal_hold(void) {
	int ret = sigprocmask(SIG_BLOCK, &die_signals, NULL);
	if (ret != 0) {
		perror("sigprocmask SIG_BLOCK");
		/* need to continue */
	}
}
static void signal_dispatch(void) {
	cerror("sigprocmask SIG_UNBLOCK", sigprocmask(SIG_UNBLOCK, &die_signals, NULL) != 0);
}

static void backup_press(void) {
	int ret = mq_send(qbackup, (char *)&press[count], sizeof(press_t), 0);
	cerror("mq_send backup", ret != 0);
	_printf("wrote %d %lu.%06u %d to backup queue\n", count, (unsigned long int)press[count].tv.tv_sec, (unsigned int)press[count].tv.tv_usec, press[count].on);
}

static void backup_clear(void) {
	_printf("clearing backup queue\n");
	while (count > 0) {
		int ret = mq_receive(qbackup, (char *)&press[--count], sizeof(press_t), 0);
		cerror("mq_receive backup", ret != sizeof(press_t));
	}
}

static void backup_load(void) {
	int ret, loaded = 0;

	/* critical section:
	 *
	 * block (hold) all signals
	 */
	signal_hold();

	/* read from backup queue */
	while (loaded < PRESS_CACHE) {
		ret = mq_receive(qbackup, (char *)&press[loaded], sizeof(press_t), 0);
		if (ret != sizeof(press_t)) {
			cerror("mq_receive backup", errno != EAGAIN);
			break;
		} else {
			_printf("read %d %lu.%06u %d from backup queue\n", loaded, (unsigned long int)press[loaded].tv.tv_sec, (unsigned int)press[loaded].tv.tv_usec, press[loaded].on);
			loaded++;
		}
	}

	/* discard unknown off data
	 *
	 * this may happen if the backup queue is partially cleared
	 */

	/* from [on, off, on] to [off, on] instead of [on] */
	if (loaded == 2 && !press[0].on) {
		press[0] = press[1];
		loaded = 1;
	}

	/* from [on, off] to [off] instead of [] */
	if (loaded == 1 && !press[0].on)
		loaded = 0;

	/* write to backup queue */
	for (count = 0; count < loaded; count++)
		backup_press();

	/* non-critical section:
	 *
	 * unblock all signals (receive pending signals immediately)
	 */
	signal_dispatch();
}

static void daemon(void) {
#ifdef FORK
	pid_t pid = fork();
	cerror("Failed to become a daemon", pid < 0);
	if (pid)
		exit(EXIT_SUCCESS);
	close(0);
	close(1);
	close(2);
	setsid();
#endif
}

static bool __press_on(const struct timeval *on, const struct timeval *off) {
	(void)off;
	return press_on(on);
}

static bool __press_cancel(const struct timeval *on, const struct timeval *off) {
	(void)off;
	return press_cancel(on);
}

static bool __press_resume(const struct timeval *on, const struct timeval *off) {
	(void)off;
	return press_resume(on);
}

static void save(bool (*func)(const struct timeval *, const struct timeval *)) {
	int backoff = 1;

	while (!func(&press[0].tv, &press[1].tv)) {
		sleep(backoff);

		if (backoff < 256)
			backoff <<= 1;
	}
}

static void save_on(void) {
	assert(count == 1);
	assert(press[0].on);

	if (process_on) {
		_printf("process on press\n");
		save(__press_on);
		process_on = false;
	}
}

static void save_on_off(void) {
	bool ignore;
	assert(count == 2);
	assert(press[0].on);
	assert(!press[1].on);

	ignore = (tv_to_ull(press[1].tv) - tv_to_ull(press[0].tv) < MIN_PRESS);

	if (process_on) {
		if (ignore) {
			_printf("cancelling short on+off press\n");
			save(__press_cancel);

			backup_clear();
		} else {
			_printf("process on+off press\n");
			save(press_on_off);
		}
	} else {
		if (ignore) {
			_printf("cancelling short press\n");
			save(__press_cancel);

			backup_clear();
		} else {
			_printf("process off press\n");
			save(press_off);
		}
	}
}

static void save_on_off_on(void) {
	bool ignore;

	assert(count == 3);
	assert(press[0].on);
	assert(!press[1].on);
	assert(press[2].on);

	ignore = (tv_to_ull(press[2].tv) - tv_to_ull(press[1].tv) < MIN_PRESS);

	if (ignore) {
		press_t keep;

		if (process_on) {
			ignore = (tv_to_ull(press[1].tv) - tv_to_ull(press[0].tv) < MIN_PRESS);

			if (ignore) {
				_printf("cancelling short on+off press\n");
				save(__press_cancel);

				/* keep the third press and ignore the other two */
				press[0] = press[2];
			} else {
				_printf("fixing interrupted press\n");
			}
		} else {
			_printf("resuming interrupted press\n");
		}

		save(__press_resume);

		/* critical section:
		*
		* block (hold) all signals
		*/
		signal_hold();

		/* keep the first press */
		keep = press[0];

		/* clear everything */
		backup_clear();

		/* restore the press */
		press[0] = keep;
		backup_press();
		count = 1;

		/* non-critical section:
		*
		* unblock all signals (receive pending signals immediately)
		*/
		signal_dispatch();
	} else {
		if (process_on) {
			_printf("check on+off+on press\n");

			/* run on+off process */
			count = 2;
			save_on_off();
		} else {
			_printf("handle on+off+on press\n");
		}

		/* clear the first two records */
		count = 2;
		backup_clear();

		/* continue */
		press[0] = press[2];
		count = 1;
		process_on = true;
	}

	if (process_on) {
		assert(count == 1);
		save_on();
	}
}

static void handle_press(void) {
	if (count == 0) {
		/* no data */
		if (press[count].on) { /* new on press */
			backup_press();

			count++;
			process_on = true;
		} else { /* discard unknown off press */ }
	} else if (count == 1) {
		/* on press waiting for off press */
		if (!press[count].on) { /* matching off press */
			backup_press();

			count++;
		} else { /* duplicate on press */
			backup_clear();
			backup_press();

			press[0] = press[1];
			process_on = true;
		}
	} else {
		/* on+off press waiting for on press */
		assert(count == 2);

		if (press[count].on) { /* new on press */
			backup_press();

			count++;
		} else { /* discard unknown off press */
			backup_clear();

			count = 0;
		}
	}
}

static void signal_capture(void) {
	cerror("sigaction SIGHUP", sigaction(SIGHUP, &sa_ign, NULL) != 0);
	cerror("sigaction SIGINT", sigaction(SIGINT, &sa_ign, NULL) != 0);
	cerror("sigaction SIGQUIT", sigaction(SIGQUIT, &sa_ign, NULL) != 0);
	cerror("sigaction SIGTERM", sigaction(SIGTERM, &sa_ign, NULL) != 0);
}

static void signal_release(void) {
	cerror("sigaction SIGHUP", sigaction(SIGHUP, &sa_dfl, NULL) != 0);
	cerror("sigaction SIGINT", sigaction(SIGINT, &sa_dfl, NULL) != 0);
	cerror("sigaction SIGQUIT", sigaction(SIGQUIT, &sa_dfl, NULL) != 0);
	cerror("sigaction SIGTERM", sigaction(SIGTERM, &sa_dfl, NULL) != 0);
}

static void get_data(void) {
	int ret;

	assert(count >= 0);
	assert(count < PRESS_CACHE);

	/* managed section:
	*
	* handle signals, saving them for later
	*/
	signal_capture();

	/* although the signal handler will stop
	* signals killing the process, this will
	* return EINTR allowing it to be killed
	* in this non-critical section
	*/
	ret = mq_receive(qmain, (char *)&press[count], sizeof(press_t), 0);
	if (ret != sizeof(press_t)) {
		/* non-critical section:
		*
		* exit, possibly with interrupted status
		*/
		if (errno == 0)
			errno = EIO; /* message size mismatch */
		xerror("mq_receive main");
	} else {
		/* critical section:
		*
		* signals are currently handled, but
		* would cause mq_send to return EINTR
		*
		* block (hold) all signals
		*/
		try_signal_hold();

		_printf("read %d %lu.%06u %d from main queue\n", count, (unsigned long int)press[count].tv.tv_sec, (unsigned int)press[count].tv.tv_usec, press[count].on);
		handle_press();
	}

	/* non-critical section:
	*
	* unblock all signals (receive pending signals immediately)
	* resume use of default signal handlers
	*/
	signal_dispatch();
	signal_release();
}

static void loop(void) {
	do {
		_printf("main loop %d\n", count);
		assert(count >= 0);
		assert(count <= PRESS_CACHE);

		switch (count) {
		case 0: /* no data */
			break;

		case 1: /* press on */
			save_on();
			break;

		case 2: /* press on, press off */
			save_on_off();
			break;

		case 3: /* press on, press off, press on */
			save_on_off_on();
			break;
		}

		get_data();
	} while(waiting_sig == 0);

	/* resend first signal captured by signal handler */
	if (waiting_sig != 0)
		cerror("kill", kill(getpid(), waiting_sig) != 0);
}

static void cleanup_syslog(void) {
#ifdef SYSLOG
	closelog();
	free(ident);
#endif
}

static void cleanup(void) {
	cleanup_syslog();
	cerror(mqueue_main, mq_close(qmain));
	cerror(mqueue_backup, mq_close(qbackup));
	free(mqueue_backup);
}

int main(int argc, char *argv[]) {
	setup(argc, argv);
	init();
	backup_load();
	daemon();
	loop();
	cleanup();
	exit(EXIT_FAILURE);
}
