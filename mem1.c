#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>

static char msg[] = "%d Hilarious Man Walks Into A Bar\n";

int main()
{
	printf("My PID is %d\n", getpid());

	for (int i=0; i<100; i++) {
		printf(msg, i);
		sleep(1);
	}

	return 0;
}