#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#define LENGTH (2<<12)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */
#define BUFSIZE (LENGTH*SIZE*CHANNELS/8)

extern unsigned long long getTime();
extern void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample);

unsigned long long now;

void readfile(char *fname) {
	unsigned char buf[BUFSIZE];
	int fd, status;

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
		process_input(buf, status, now, (1000000/(RATE*(SIZE/8))));
	} else if (status == -1) {
		perror("Read failed");
	}

	close(fd);
}

int main(int argc, char *argv[]) {
	now = getTime();

	readfile("1235988436651746");
	readfile("1235989354421965");
	readfile("1235989355446039");
	readfile("1235989356470112");
	readfile("1235989357494183");
	readfile("1235989358518255");
	readfile("1235989359542324");
	readfile("1235989360566396");
	readfile("1235989368758957");

	return 0;
}
