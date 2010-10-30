/*
 * Copyright ©2010  Simon Arlott
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
#include <linux/soundcard.h>

#define LENGTH (2<<14)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */

extern unsigned long long getTime();
extern void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample, int nosave);

int main(int argc, char *argv[]) {
	/* this buffer holds the digitized audio */
	unsigned char buf[LENGTH*SIZE*CHANNELS/8];
	int fd;		/* sound device file descriptor */
	int arg;	/* argument for ioctl calls */
	int status;	/* return status of system calls */
	unsigned long long then, now;
	(void)argc;
	(void)argv;

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
			process_input(buf, status, now, (1000000/(RATE*(SIZE/8))), 0);
		} else if (status == -1) {
			perror("Read failed");
			exit(1);
		}
	}
}
