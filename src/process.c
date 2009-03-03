#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

static unsigned long long last = ~0;

#define SAVE 1

void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample, int nosave) {
	int i;
	unsigned int min = ~0, max = 0, ring = 0;
	unsigned long long now = time - persample*buflen;

	printf(", now %llu", now);
	for (i = 0; i < buflen; i++) {
		if (buf[i] < min)
			min = buf[i];
		if (buf[i] > max)
			max = buf[i];

		now += persample;

		if (buf[i] < 126 || buf[i] > 130) {
			/* ding dong! */
			if (now > last && now - last >= 400000) {
				printf(", gap %llu", now - last);
				ring++;

				printf(", <fork>");
				if (fork() == 0) {
					char arg1[31];
					char arg2[21];
					char arg3[21];
					time_t tmp = now/1000000;
					strftime(arg1, 30, "%Y-%m-%d %H:%M:%S", localtime(&tmp));
					snprintf(arg2, 20, "%llu", now % 1000000);
					snprintf(arg3, 20, "%llu", now - last);

					execlp("./dingdong", "dingdong", arg1, arg2, arg3, NULL);
					_exit(1);
				} else {
					int status;
					if (waitpid(-1, &status, WNOHANG) > 0)
						printf(", <exit %d>", (signed char)status);
				}
			}
			last = now;
		}
	}

#if SAVE
	if (!nosave && (min != 128 || max != 128) && fork() == 0) {
		char fname[21];
		int fd;

		snprintf(fname, 20, "%llu", time - persample*buflen);

		fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (fd) {
			write(fd, buf, buflen);
			close(fd);
		}
		_exit(0);
	}
#endif

	if (last == ~0)
		last = 0;

	printf(", min=%d, max=%d, last=%llu, ring=%d\n", min, max, last, ring);
}
