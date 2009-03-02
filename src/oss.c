#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/soundcard.h>

#define LENGTH (2<<14)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */

extern unsigned long long getTime();
extern void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample);

int main(int argc, char *argv[]) {
	/* this buffer holds the digitized audio */
	unsigned char buf[LENGTH*SIZE*CHANNELS/8];
	int fd;		/* sound device file descriptor */
	int arg;	/* argument for ioctl calls */
	int status;	/* return status of system calls */
	unsigned long long then, now;

	/* open sound device */
	fd = open("/dev/dsp", O_RDONLY);
	if (fd < 0) {
		perror("open of /dev/dsp failed");
		exit(1);
	}

	/* set sampling parameters */
	arg = SIZE;		 /* sample size */
	status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
	if (status == -1)
		perror("SOUND_PCM_WRITE_BITS ioctl failed");
	if (arg != SIZE)
		perror("unable to set sample size");

	arg = CHANNELS;	/* mono or stereo */
	status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if (status == -1)
		perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
	if (arg != CHANNELS)
		perror("unable to set number of channels");

	arg = RATE;		 /* sampling rate */
	status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
	if (status == -1)
		perror("SOUND_PCM_WRITE_WRITE ioctl failed");

	while (1) {
		then = getTime();
		status = read(fd, buf, sizeof(buf)); /* record some sound */
		now = getTime();
		if (status >= 0) {
			printf("Read %u bytes in %8.3fms", status, (double)(now-then)/1000);
			process_input(buf, status, now, (1000000/(RATE*(SIZE/8))));
		} else if (status == -1) {
			perror("Read failed");
		}
	}
}
