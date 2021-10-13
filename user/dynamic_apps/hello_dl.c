#include <stdio.h>
#include <pthread.h>
#include <assert.h>

extern void foo(int);

void *func(void *arg)
{
	printf("I am a new thread.\n");
	foo(10);
	return NULL;
}

int main()
{
#if 1
	pthread_t tid;

	printf("Hello, dynamic loader.\n");
	pthread_create(&tid, NULL, func, NULL);
	assert(pthread_join(tid, NULL) == 0);
#else

	printf("Hello, dynamic loader.\n");
	func(NULL);
#endif

	return 0;
}
