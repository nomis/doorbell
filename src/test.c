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

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#define LENGTH (2<<14)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */
#define BUFSIZE (LENGTH*SIZE*CHANNELS/8)

extern void process_input(unsigned char *buf, int buflen, unsigned long long sampletime, unsigned long long persample, int test);

struct ring {
	unsigned long long now;
	unsigned long long file;
	int ring;
	struct ring *next;
};

static unsigned long long file = 0;
static struct ring *invalid_t = NULL;
static struct ring *valid_t = NULL;
static struct ring *invalid_h = NULL;
static struct ring *valid_h = NULL;

void do_ring(unsigned long long last, unsigned long long now) {
	struct ring *tmp = valid_h;
	int ok = 0;
	(void)last;

	while (tmp != NULL) {
		if (tmp->now == now) {
			tmp->ring++;
			ok = 1;
		}
		tmp = tmp->next;
	}

	if (!ok) {
		tmp = invalid_h;
		while (tmp != NULL) {
			if (tmp->now == now) {
				tmp->ring++;
				ok = 0;
			}
			tmp = tmp->next;
		}
	}

	if (!ok) {
		tmp = malloc(sizeof(*tmp));
		tmp->next = NULL;
		tmp->file = file;
		tmp->ring = 1;
		tmp->now = now;
		if (invalid_h == NULL) {
			invalid_h = invalid_t = tmp;
		} else {
			invalid_t->next = tmp;
			invalid_t = tmp;
		}
	}

//	printf("Ring: last=%llu, now=%llu\n", last, now);
}

void readfile(unsigned long long now) {
	unsigned char buf[BUFSIZE];
	char fname[21];
	int fd, status;

	if (now > 0) {
		snprintf(fname, 25, "data/%llu", now);

		/* open saved sound */
		fd = open(fname, O_RDONLY);
		if (fd < 0) {
			perror(fname);
			exit(1);
		}

		status = read(fd, buf, sizeof(buf));
		close(fd);
	} else {
		memset(buf, 0x80, sizeof(buf));
	}

	file = now;

	now += BUFSIZE * (1000000/(RATE*(SIZE/8)));
	if (status == BUFSIZE) {
//		printf("Read %u bytes from %s\n", status, fname);
		process_input(buf, status, now, (1000000/(RATE*(SIZE/8))), 1);
	} else if (status == -1) {
		perror("Read failed\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	struct dirent **list;
	struct ring *tmp;
	unsigned long long now;
	int c_hit = 0, c_miss = 0, c_dup = 0, c_invalid = 0;
	int n, i;
	FILE *fd;
	(void)argc;
	(void)argv;

	fd = fopen("data/valid", "r");
	if (fd == NULL) {
		perror("data/valid");
		exit(1);
	}
	while (fscanf(fd, "%llu", &now) == 1) {
		tmp = malloc(sizeof(*tmp));
		tmp->next = NULL;
		tmp->file = 0;
		tmp->ring = 0;
		tmp->now = now;
		if (valid_h == NULL) {
			valid_h = valid_t = tmp;
		} else {
			valid_t->next = tmp;
			valid_t = tmp;
		}
	}
	fclose(fd);

	n = scandir("data/", &list, 0, alphasort);
	if (n < 0) {
		perror("scandir");
		exit(1);
	} else {
		unsigned long long last = 0;

		for (i = 0; i < n; i++) {
			now = strtoull(list[i]->d_name, NULL, 10);
			if (now > 0) {
				if (now - last - 4096000 > 4096)
					readfile(0ull);
				last = now;
				printf(".");
				readfile(now);
				fflush(stdout);
			}
			free(list[i]);
		}
		free(list);
		printf("\n");
	}

	tmp = valid_h;
	while (tmp != NULL) {
		if (tmp->ring == 0)
			c_miss++;
		else if (tmp->ring > 1)
			c_dup += tmp->ring - 1;
		if (tmp->ring == 1)
			c_hit++;
		tmp = tmp->next;
	}

	tmp = invalid_h;
	while (tmp != NULL) {
		char buf[256];
		time_t timep = tmp->now/1000000;
		struct tm *tm = localtime(&timep);

		c_invalid += tmp->ring;

		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
		printf("%llu (%s.%06llu) %llu ?\n", tmp->now, buf, tmp->now%1000000, tmp->file);

		tmp = tmp->next;
	}

	printf("Hit: %5d\nDup: %5d\nMiss: %4d\nInv: %5d\n", c_hit, c_dup, c_miss, c_invalid);

	return 0;
}
