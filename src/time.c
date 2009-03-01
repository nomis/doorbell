// Get current time as unsigned long long to the nearest microsecond
// (on 64-bit systems this should be changed to use unsigned long)
// ©2005 Simon Arlott

#include <sys/time.h>
#include <unistd.h>

unsigned long long getTime() {
	struct timeval x;
	gettimeofday(&x, NULL);
	return (unsigned long long)x.tv_sec * 1000000 + (unsigned long long)x.tv_usec;
}

