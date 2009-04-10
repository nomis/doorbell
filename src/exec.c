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

void do_ring(unsigned long long last, unsigned long long now) {
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
