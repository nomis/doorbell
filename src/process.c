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

static unsigned long long last = ~0ULL;
static unsigned long long start = 0;
static unsigned long long burst = 0;

#define SAVE 1

extern void do_ring(unsigned long long last, unsigned long long now);

void process_input(unsigned char *buf, int buflen, unsigned long long sampletime, unsigned long long persample, int test) {
	int i;
	int status;
	unsigned int min = ~0, max = 0, ring = 0;
	unsigned long long now = sampletime - persample*buflen;

	if (!test)
		printf(", now %llu", now);
	if (waitpid(-1, &status, WNOHANG) > 0)
		if (!test)
			printf(", <exit %d>", (signed char)status);
	for (i = 0; i < buflen; i++) {
		if (buf[i] < min)
			min = buf[i];
		if (buf[i] > max)
			max = buf[i];

		now += persample;

		if (buf[i] < 126 || buf[i] > 130) {
			burst++;
			if (burst < 5)
				continue;

			/* ding dong! */
			if (now > last && now - last >= 400000) {
				if (!test)
					printf(", gap %llu", now - last);
				ring++;

				do_ring(last, start);
			}
			last = now;
		} else {
			burst = 0;
		}
	}

#if SAVE
	if (!test && (min != 128 || max != 128) && fork() == 0) {
		char fname[21];
		int fd;

		snprintf(fname, 20, "%llu", sampletime - persample*buflen);

		fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (fd) {
			write(fd, buf, buflen);
			close(fd);
		}
		_exit(0);
	}
#endif

	if (last == ~0ULL)
		last = 0;

	if (!test)
		printf(", min=%d, max=%d, last=%llu, ring=%d\n", min, max, last, ring);
}
