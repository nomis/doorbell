#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "doorbellmon.h"
#include "doorbellq.h"

char *device;
char *mqueue;
int fd;
mqd_t q;

static void setup(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <device> <mqueue>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	device = argv[1];
	mqueue = argv[2];
}

static void init_root(void) {
	if (geteuid() == 0) {
		struct sched_param schedp;

		cerror("Failed to lock memory pages", mlockall(MCL_CURRENT | MCL_FUTURE));
		cerror("Failed to get max scheduler priority", (schedp.sched_priority = sched_get_priority_max(SCHED_FIFO)) < 0);
		schedp.sched_priority -= 25;
		cerror("Failed to set scheduler policy", sched_setscheduler(0, SCHED_FIFO, &schedp));
		cerror("Failed to drop SGID permissions", setregid(getgid(), getgid()));
		cerror("Failed to drop SUID permissions", setreuid(getuid(), getuid()));
	}
}

static void init(void) {
	struct mq_attr q_attr = {
		.mq_flags = 0,
		.mq_maxmsg = 4096,
		.mq_msgsize = sizeof(press_t)
	};
#if (SERIO_OUT|SERIO_OFF) != 0
	int state;
#endif

	init_root();

	fd = open(device, O_RDONLY|O_NONBLOCK);
	cerror(device, fd < 0);

#if (SERIO_OUT|SERIO_OFF) != 0
	cerror("Failed to get serial IO status", ioctl(fd, TIOCMGET, &state) != 0);
# if SERIO_OUT != 0
	state |= SERIO_OUT;
# endif
# if SERIO_OFF != 0
	state &= ~SERIO_OFF;
# endif
	cerror("Failed to set serial IO status", ioctl(fd, TIOCMSET, &state) != 0);
#endif

	q = mq_open(mqueue, O_WRONLY|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &q_attr);
	cerror(mqueue, q < 0);
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

static void report(bool on) {
	press_t press;

	gettimeofday(&press.tv, NULL);
	press.on = on;

	_printf("%lu.%06u: %d\n", (unsigned long int)press.tv.tv_sec, (unsigned int)press.tv.tv_usec, press.on);
	mq_send(q, (const char *)&press, sizeof(press), 0);
}

static bool check(void) {
	static bool first = true;
	static int last;
	bool changed = false;
	int state;

	cerror("Failed to get serial IO status", ioctl(fd, TIOCMGET, &state) != 0);
	state &= SERIO_IN;

	if (first) {
		first = false;
	} else {
		if (last != state) {
			changed = true;
			report(state != 0);
		}
	}

	last = state;
	return changed;
}

static bool wait(void) {
	bool ok = ioctl(fd, TIOCMIWAIT, SERIO_IN) == 0;
	if (!ok)
		perror("Failed to wait for serial IO status");
	return ok;
}

static void loop(void) {
	do {
		while (check())
			usleep(CHECK_INTERVAL);
	} while (wait());
}

static void cleanup(void) {
	cerror(device, close(fd));
	cerror(mqueue, mq_close(q));
}

int main(int argc, char *argv[]) {
	setup(argc, argv);
	init();
	daemon();
	loop();
	cleanup();
	exit(EXIT_FAILURE);
}
