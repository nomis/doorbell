#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

static unsigned long long last = 0;

void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample) {
	int i;
	unsigned int min = ~0, max = 0, ring = 0;
	unsigned long long now = time - persample*buflen;
	for (i = 0; i < buflen; i++) {
		if (buf[i] < min)
			min = buf[i];
		if (buf[i] > max)
			max = buf[i];

		now += persample;

		if (buf[i] <= 112 || buf[i] >= 144) {
			/* ding dong! */
			if (last == 0 || (now - last >= 200000 && now > last)) {
				ring++;

				printf(", <fork>");
				if (fork() == 0) {
					char arg1[31];
					char arg2[21];
					time_t tmp = now/1000000;
					strftime(arg1, 30, "%Y-%m-%d %H:%M:%S", localtime(&tmp));
					snprintf(arg2, 20, "%llu", now % (unsigned long long)1e6);

					execlp("./dingdong", "dingdong", arg1, arg2, NULL);
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
	printf(", min=%d, max=%d, last=%llu, ring=%d\n", min, max, last, ring);
}
