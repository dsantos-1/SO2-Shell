// Danilo Marques Araujo dos Santos - 8598670

#ifndef _SYSERROR_H_
#define _SYSERROR_H_

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

pid_t Fork(void);

int Pipe(int *);

void Kill(pid_t, int);

pid_t Setpgid(pid_t, pid_t);

int Sigaddset(sigset_t *, int);

int Sigprocmask(int, const sigset_t *, sigset_t *);

int Sigemptyset(sigset_t *);

pid_t Waitpid(pid_t, int *, int);

void unixError(char *);

#endif