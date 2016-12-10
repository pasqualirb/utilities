#include <signal.h>
#include <stdio.h>
#include <unistd.h>

/**
 * block all signals
 * @param save  we fill with current signal mask, that's before blocking signals
 * 		(aka  already blocked signals)
 */
void
signal_lock (sigset_t *save) {
	sigset_t set;

	sigfillset(&set); /* fill `set` with __all__ signals */
	sigprocmask(SIG_BLOCK, &set, save); /* no more signals :-)  */
}

/**
 * unblock all signals except those in `save` variable
 * @param save  signal mask before blocking signals in signal_lock()
 */
void
signal_unlock (sigset_t *save) {
	sigprocmask(SIG_SETMASK, save, NULL); /* enable again */
}

void
handler (int sig) {
	printf("signal %d was caught\n", sig);
}

int
signal_parent (void) {
	pid_t ppid = getppid();

	for (;;)
		kill(ppid, SIGUSR1);
}

int
main (void) {
	pid_t pid;
	sigset_t tmp;

	signal(SIGUSR1, handler);

	pid = fork();
	if (pid == 0)
		return signal_parent();

	for (;;) {
		signal_lock(&tmp);
		printf("Innocuous message\n");
		signal_unlock(&tmp);
	}
}
