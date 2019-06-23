// Danilo Marques Araujo dos Santos - 8598670

#include "syserror.h"

pid_t Fork(void) {
	int pid=-1;
	
	if((pid=fork())<0) {
		unixError("fork error");
		exit(EXIT_SUCCESS);
	}
	
	return pid;
}

int Pipe(int p[]) {
	int ret=-1;
	
	if((ret=pipe(p))<0) {
		unixError("pipe error");
		exit(EXIT_SUCCESS);
	}
	
	return ret;
}

void Kill(pid_t pid, int sig) {
	if(kill(pid, sig)<0) {
		unixError("kill error");
		exit(EXIT_SUCCESS);
	}
}

pid_t Setpgid(pid_t pid, pid_t pgid) {
	pid_t ret=1;
	
	if((ret=setpgid(pid, pgid))<0) {
		unixError("setpgid error");
	}
	
	return ret;
}

int Sigaddset(sigset_t *set, int signum) {
	int ret;
	
	if((ret=sigaddset(set, signum))<0) {
		unixError("sigaddset error");
	}
	
	return ret;
}

int Sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
	int ret=1;
	
	if((ret=sigprocmask(how, set, oldset))<0) {
		unixError("sigprocmask error");
	}
	
	return ret;
}

int Sigemptyset(sigset_t *set) {
	int ret;
	
	if((ret=sigemptyset(set))<0) {
		unixError("sigemptyset error");
	}
	
	return ret;
}

pid_t Waitpid(pid_t pid, int *status, int options) {
	pid_t ret;
	
	if((ret=waitpid(pid, status, options))<0) {
		unixError("waitpid error");
		exit(EXIT_SUCCESS);
	}
	
	return ret;
}

void unixError(char *msg) {
	fprintf(stdout, "%s: %s\n", msg, strerror(errno));
	exit(1);
}