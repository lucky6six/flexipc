#include <stdio.h>

int main(int argc, char *argv[], char *envp[])
{
        int i = 0;

	printf("testing argc and argv...\n");

	printf("argc = %d\n", argc);

	for (i = 0; i < argc; ++i) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}

	printf("test finished.\n");

	return 0;
}
