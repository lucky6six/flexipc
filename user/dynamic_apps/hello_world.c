#include <stdio.h>

int main(int argc, char *argv[])
{
	int i;

	printf("Hello World!\n");

	for (i = 0; i < argc; ++i) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	return 0;
}
