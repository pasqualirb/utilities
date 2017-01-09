/*
 * Copyright (C) 2016,2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 * under the terms of the GNU General Public License (see LICENSE file)
 *
 * create X forks and kill them
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* used by child to determine whether it was killed by parent or by itself */
int child_killed;

/* SIGUSR1 handler */
void
child_sigusr_handler (int signum) {
	child_killed = 1;
}

int
main (int argc, char **argv) {
	pid_t *pids; /* children pid list */
	int n; /* how many children we should create */
	int t; /* sleep time before parent start killing */
	int i; /* a counter ... */

	if (argc < 3) {
		printf("usage: cmd <N_children> <parent_wait_time>\n");
		exit(1);
	}
	n = atoi(argv[1]);
	t = atoi(argv[2]);

	pids = malloc(n * sizeof(pid_t)); /* allocate memory for pid list */

	/* start children */
	for (i = 0; i < n; ++i) {
		if ((pids[i] = fork()) < 0) {
			perror("fork");
			abort();
		} else if (pids[i] == 0) { /* we're child */
			child_killed = 0; /* signal handler will turn it '1' */
			signal(SIGUSR1, child_sigusr_handler);
			sleep(5); /* child waits for 5 secs to exit by itself */

			printf("[%d] child ", getpid());
			if (child_killed)
				printf("has been killed by parent\n");
			else
				printf("has ended by itself\n");

			exit(0);
		}
	}

	sleep(t);

	/* kill children */
	for (i = 0; i < n; ++i) {
		kill(pids[i], SIGUSR1);
		waitpid(pids[i], NULL, 0);
	}

	free(pids);
	return 0;
}
