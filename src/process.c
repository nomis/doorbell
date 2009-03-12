/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
	int status;
	unsigned int min = ~0, max = 0, ring = 0;
	unsigned long long now = time - persample*buflen;

	printf(", now %llu", now);
	if (waitpid(-1, &status, WNOHANG) > 0)
		printf(", <exit %d>", (signed char)status);
	for (i = 0; i < buflen; i++) {
		if (buf[i] < min)
			min = buf[i];
		if (buf[i] > max)
			max = buf[i];

		now += persample;

		if (buf[i] < 125 || buf[i] > 131) {
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
