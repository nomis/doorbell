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

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define LENGTH (2<<12)	/* how many samples of audio to store */
#define RATE 8000	/* the sampling rate */
#define SIZE 8		/* sample size: 8 or 16 bits */
#define CHANNELS 1	/* 1 = mono 2 = stereo */

extern unsigned long long getTime();
extern void process_input(unsigned char *buf, int buflen, unsigned long long time, unsigned long long persample, int nosave);

int main(int argc, char *argv[]) {
	/* this buffer holds the digitized audio */
	unsigned char buf[LENGTH*SIZE*CHANNELS/8];
	snd_pcm_t *ch;	/* sound device capture handle */
	int status;	/* return status of system calls */
	unsigned long long then, now;
	(void)argc;
	(void)argv;

	/* open sound device */
	status = snd_pcm_open(&ch, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (status < 0) {
		fprintf(stderr, "open of \"default\" failed: %s\n", snd_strerror(status));
		exit(1);
	}

	/* set sampling parameters */
	status = snd_pcm_set_params(ch, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS, RATE, 1, 500000);
	if (status < 0) {
		fprintf(stderr, "cannot set sampling parameters: %s\n", snd_strerror(status));
		exit(1);
	}

	status = snd_pcm_prepare(ch);
	if (status < 0) {
		fprintf(stderr, "cannot prepare audio interface for use: %s\n", snd_strerror(status));
		exit(1);
	}

	while (1) {
		then = getTime();
		status = snd_pcm_readi(ch, buf, sizeof(buf)); /* record some sound */
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
