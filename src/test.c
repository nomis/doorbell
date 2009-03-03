#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#define LENGTH (2<<14)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */
#define BUFSIZE (LENGTH*SIZE*CHANNELS/8)

extern void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample, int nosave);

void readfile(unsigned long long now) {
	unsigned char buf[BUFSIZE];
	char fname[21];
	int fd, status;

	snprintf(fname, 20, "%llu", now);

	/* open sound device */
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror("open of sound file failed");
		exit(1);
	}

	status = read(fd, buf, sizeof(buf)); /* record some sound */
	now += BUFSIZE * (1000000/(RATE*(SIZE/8)));
	if (status == BUFSIZE) {
		printf("Read %u bytes", status);
		process_input(buf, status, now, (1000000/(RATE*(SIZE/8))), 1);
	} else if (status == -1) {
		perror("Read failed");
	}

	close(fd);
}

int main(int argc, char *argv[]) {
	readfile(1236047295328452ull);
	readfile(1236069790091182ull);
	readfile(1236072695893112ull);
	readfile(1236072700757055ull);
	readfile(1236072705621376ull);

	return 0;
}
