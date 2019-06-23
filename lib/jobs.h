// Danilo Marques Araujo dos Santos - 8598670

#ifndef _JOBS_H_
#define _JOBS_H_

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

// Macro definitions
#define MAXLINE	1000
#define MAXARGS	  80
#define MAXJOBS	  32
#define MAXJID	1<<32

// Process states
#define UNDEF 0
#define FG 1
#define BG 2
#define ST 3

// Struct that defines a job
struct job_t {
	char cmdline[MAXLINE];
	int state;
	pid_t pid;
	int jid;
};

typedef void handler_t(int);

void listJobs(struct job_t *);

int pidToJid(pid_t);

struct job_t *getJobPid(struct job_t *, pid_t);

int removeJob(struct job_t *, pid_t);

int addJob(struct job_t *, pid_t, int, char *);

int maxJid(struct job_t *);

void startJobs(struct job_t *);

void resetJob(struct job_t *);

void waitFg(pid_t);

handler_t *Signal(int, handler_t *);

void ctrlZ_hdlr(int);

void ctrlC_hdlr(int);

void child_hdlr(int);

void Wait();

// Extern variables
extern int nextJid;

extern struct job_t jobs[MAXJOBS];

extern pid_t shell_pgid;

extern int shell_terminal;

#endif