#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "doorbellfake.h"
#include "doorbellq.h"

char *mqueue;
struct timeval tv;
bool on;
mqd_t q;

static void setup(int argc, char *argv[]) {
	unsigned long secs;
	unsigned int usecs;
	int ret;

	if (argc != 4) {
		printf("Usage: %s <mqueue> <time> <press>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	mqueue = argv[1];

	ret = sscanf(argv[2], "%lu.%06u", &secs, &usecs);
	if (ret == 2) {
		tv.tv_sec = secs;
		tv.tv_usec = usecs;
	} else {
		printf("Invalid time value '%s'", argv[2]);
		exit(EXIT_FAILURE);
	}

	if (!strcmp(argv[3], "on") || !strcmp(argv[3], "1")) {
		on = true;
	} else if (!strcmp(argv[3], "off") || !strcmp(argv[3], "0")) {
		on = false;
	} else {
		printf("Invalid press value '%s'", argv[3]);
		exit(EXIT_FAILURE);
	}
}

static void init(void) {
	q = mq_open(mqueue, O_WRONLY|O_NONBLOCK, S_IRUSR|S_IWUSR, NULL);
	cerror(mqueue, q < 0);
}

static void report(void) {
	press_t press;

	press.tv = tv;
	press.on = on;

	_printf("%lu.%06u: %d\n", (unsigned long int)press.tv.tv_sec, (unsigned int)press.tv.tv_usec, press.on);
	mq_send(q, (const char *)&press, sizeof(press), 0);
}

static void cleanup(void) {
	cerror(mqueue, mq_close(q));
}

int main(int argc, char *argv[]) {
	setup(argc, argv);
	init();
	report();
	cleanup();
	exit(EXIT_FAILURE);
}
