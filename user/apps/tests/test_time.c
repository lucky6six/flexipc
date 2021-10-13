#include <stdio.h>
#include <time.h>
#include <assert.h>

#include <sys/time.h>

int main(int argc, char *argv[])
{
	int ret;
	struct timespec t;
	struct timeval tv;

	ret = clock_gettime(0, &t);
	assert(ret == 0);
	printf("[clock_gettime] t->secs: 0x%lx, t->nsecs: 0x%lx\n",
	       t.tv_sec, t.tv_nsec);

	ret = gettimeofday(&tv, NULL);
	assert(ret == 0);
	printf("[gettimeofday] t->secs: 0x%lx, t->usecs: 0x%lx\n",
	       tv.tv_sec, tv.tv_usec);

	return 0;
}
