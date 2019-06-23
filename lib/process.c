// Danilo Marques Araujo dos Santos - 8598670

#include "syserror.h"
#include "process.h"
#include "jobs.h"

// Extern global variables
extern int nPaths;
extern char ** PATHS;
extern sigset_t maskchld;
extern pid_t shell_pgid;
extern int shell_terminal;

// Redirection variables
int redir_erra=0;
int redir_app=0;
int redir_err=0;
int redir_out=0;
int redir_in=0;

// File pointers
int fpin, fpout, fperr;

// Position to stdin and stdout buffers
fpos_t pos_in, pos_out;

// Splits the command string by spaces
int parser(const char *cmdline, char **argv, int *argcc) {
	static char array[MAXLINE];
	char *buf=array;
	char *delim;
	int argc;
	int bg;

	strcpy(buf, cmdline);
	
	buf[strlen(buf)-1]=' ';

	while(*buf && (*buf==' '))
		buf++;

	argc=0;
	if(*buf=='\'') {
		buf++;
		delim=strchr(buf, '\'');
	}
	else {
		delim=strchr(buf, ' ');
	}

	while(delim) {
		argv[argc++]=buf;
		*delim='\0';
		buf=delim+1;

		while(*buf && (*buf==' '))
			buf++;

		if(*buf=='\'') {
			buf++;
			delim=strchr(buf, '\'');
		}
		else {
			delim=strchr(buf, ' ');
		}
	}

	argv[argc]=NULL;

	if(argc==0)
		return 1;

	*argcc=argc;

	// Checks if the process is running in background
	if((bg=(*argv[argc-1]=='&'))!=0) {
		argv[--argc]=NULL;
	}

	return bg;
}

// Runs in background
void procBg(char *argv[]) {
	char job_name[32];
	int pid=-1;

	// Uses the mask to block signals again
	Sigprocmask(SIG_BLOCK, &maskchld, NULL);

	pid=Fork();

	// Child process
	if(pid==0) {
		// Changes the group of the process
		Setpgid(0, 0);

		// Unblocks signals
		Sigprocmask(SIG_UNBLOCK, &maskchld, NULL);

		// Turns on default signals again
		Signal(SIGINT, SIG_DFL);
		Signal(SIGTSTP, SIG_DFL);

		// Runs the command that was typed
		if(execvp(argv[0], argv)<0) {
			printf("luna: %s: command not found\n", argv[0]);
			exit(1);
		}

		return;
	}

	// Parent process
	else {
		strcpy(job_name, argv[0]);
		strcat(job_name, "\n");

		// Adds to bg
		addJob(jobs, pid, BG, job_name);

		// Unblocks signals
		Sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
	}
}

// Runs in foreground
void procFg(char *argv[]) {
	char *job_name;
	int pid=-1;

	// Uses the mask to block signals again
	Sigprocmask(SIG_BLOCK, &maskchld, NULL);

	pid=Fork();

	if(pid==0) {
		Setpgid(0, 0);

		Signal(SIGINT, SIG_DFL);
		Signal(SIGTSTP, SIG_DFL);
		Signal(SIGCHLD, SIG_DFL);

		Sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
		
		tcsetpgrp(shell_terminal, shell_pgid);
		
		if(execvp(argv[0], argv)<0) {
			printf("luna: %s: command not found\n", argv[0]);
			exit(1);
		}
	}
	else {
		job_name=strcat(argv[0], "\n");
		
		addJob(jobs, pid, FG, job_name);
		
		Signal(SIGTSTP, ctrlZ_hdlr);
		Signal(SIGINT, ctrlC_hdlr);
		Signal(SIGCHLD, child_hdlr);
		
		Setpgid(pid, pid);
		tcsetpgrp(shell_terminal, pid);
		Sigprocmask(SIG_UNBLOCK, &maskchld, NULL);
		
		waitFg(pid);
		
		tcsetpgrp(shell_terminal, shell_pgid);
	}
	
	return;
}

// Processes commands with redirection/pipe
void procDirPipe(char *cmdline, int frompipe, int ifd[2], int fdstd[2]) {
	char for_cmdline[MAXLINE];
	char *after_cmdline;
	int i, pid, argcc;
	int topipe=0;
	char **argv;
	int pfd[2];

	if(frompipe) {
		fdstd[0]=dup(fileno(stdin));
		dup2(ifd[0], 0);
	}
	
	// Checks where the pipe goes
	if((after_cmdline=strstr(cmdline, "|"))!=NULL) {
		topipe=1;
		strncpy(for_cmdline, cmdline, strlen(cmdline)-strlen(after_cmdline));
		
		for_cmdline[strlen(cmdline)-strlen(after_cmdline)]='\0';
		
		after_cmdline+=2;
		
		if(pipe(pfd)==-1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		
		fdstd[1]=dup(fileno(stdout));
		dup2(pfd[1], 1);
	}
	else {
		strcpy(for_cmdline, cmdline);
	}
	
	// Matrix of arguments
	argv=(char **)calloc(MAXARGS, MAXLINE);
	
	for(i=0; i<MAXARGS; i++)
		argv[i]=(char *)calloc(MAXLINE, sizeof(char));
	
	// Split the arguments
	parser(for_cmdline, argv, &argcc);
	
	// Blocks the signals using masks
	sigset_t newmask;

	Sigemptyset(&newmask);
	Sigaddset(&newmask, SIGCHLD);
	Sigprocmask(SIG_BLOCK, &newmask, NULL);
	
	pid=fork();
	if(pid==0) {
		Sigprocmask(SIG_UNBLOCK, &newmask, NULL);

		Setpgid(0, 0);

		if(execvp(argv[0], argv)<0) {
			printf("luna: %s: command not found\n", argv[0]);
			exit(1);
		}
	}
	
	addJob(jobs, pid, FG, cmdline);
	Sigprocmask(SIG_UNBLOCK, &newmask, NULL);

	waitFg(pid);

	if(frompipe) {
		dup2(fdstd[0], fileno(stdin));
	}
	
	// Recursive call for the rest of the command
	if(topipe) {
		dup2(fdstd[1], fileno(stdout));
		close(pfd[1]);
		procDirPipe(after_cmdline, 1, pfd, fdstd);
	}
}

// Sets the redirection variables
void setPath(char *cmdline) {
	char file_in[50], file_out[50], file_app[50], file_err[50];
	char aux[1024]={0};
	int i, argcc;
	char **argv;
	
	// Allocates matrix of commands
	argv=(char **)calloc(MAXARGS, MAXLINE);
	for(i=0; i<MAXARGS; i++)
		argv[i]=(char *)calloc(MAXLINE, sizeof(char));
	
	parser(cmdline, argv, &argcc);

	for(i=0; i<MAXARGS; i++) {
		// Checks the arguments and sets the corresponding variables
		if(argv[i]!=NULL) {
			if(strcmp(argv[i], "<")==0) {
				redir_in=1;
				strncpy(file_in, argv[i+1], strlen(argv[i+1])+1);
				sprintf(argv[i], " ");
				sprintf(argv[i+1], " ");
			}
			if((strcmp(argv[i], ">>")==0) || (strcmp(argv[i], "1>>")==0)) {
				redir_app=1;
				strncpy(file_app, argv[i+1], strlen(argv[i+1])+1);
				sprintf(argv[i], " ");
				sprintf(argv[i+1], " ");
			}
			if((strcmp(argv[i], ">")==0) || (strcmp(argv[i], "1>")==0)) {
				redir_out=1;
				strncpy(file_out, argv[i+1], strlen(argv[i+1])+1);
				sprintf(argv[i], " ");
				sprintf(argv[i+1], " ");
			}

			if(strcmp(argv[i], "2>")==0) {
				redir_err=1;
				strncpy(file_err, argv[i+1], strlen(argv[i+1])+1);
				sprintf(argv[i], " ");
				sprintf(argv[i+1], " ");
			}
			if(strcmp(argv[i], "2>>")==0) {
				redir_erra=1;
				strncpy(file_err, argv[i+1], strlen(argv[i+1])+1);
				sprintf(argv[i], " ");
				sprintf(argv[i+1], " ");
			}
		}
	}
	
	// Closes stdin and opens from given file
	if(redir_in==1) {
		fflush(stdin);
		fgetpos(stdin, &pos_in);
		fpin=dup(fileno(stdin));
		freopen(file_in, "r", stdin);
	}
	
	// Closes stdout and opens from given file (write)
	if(redir_out==1) {
		fflush(stdout);
		fgetpos(stdout, &pos_out);
		fpout=dup(fileno(stdout));
		freopen(file_out, "w", stdout);
	}
	
	// Closes stdout and opens from given file (append)
	else if(redir_app==1) {
		fflush(stdout);
		fgetpos(stdout, &pos_out);
		fpout=dup(fileno(stdout));
		freopen(file_app, "a", stdout);
	}

	// Closes stderr and opens from given file (write)
	if(redir_err==1) {
		fflush(stderr);
		fgetpos(stdout, &pos_out);
		fperr=dup(fileno(stderr));
		freopen(file_err, "w", stderr);
	}

	// Closes stderr and opens from given file (append)
	else if(redir_erra==1) {
		fflush(stderr);
		fgetpos(stdout, &pos_out);
		fperr=dup(fileno(stderr));
		freopen(file_err, "a", stderr);
	}
	
	// Reassembles the command line that was split in the beginning
	for(i=0; i< MAXARGS; i++) {
		if(argv[i]!=NULL) {
			strcat(aux, argv[i]);
			strcat(aux, " ");
		}
	}
	
	strcpy(cmdline, aux);
	// printf("setpath - cmdline final: \"%s\"\n", cmdline);
}

// Clears the flags that were set before
void resetFlags() {
	// Reset stderr
	if(redir_err || redir_erra) {
		fflush(stderr);
		dup2(fperr, fileno(stderr));
		close(fperr);
		clearerr(stderr);
		fsetpos(stderr, &pos_in);
	}
	
	// Reset stdout
	if(redir_out || redir_app) {
		fflush(stdout);
		dup2(fpout, fileno(stdout));
		close(fpout);
		clearerr(stdout);
		fsetpos(stdout, &pos_out);
	}
	
	// Reset stdin
	if(redir_in) {
		fflush(stdin);
		dup2(fpin, fileno(stdin));
		close(fpin);
		clearerr(stdin);
		fsetpos(stdin, &pos_in);
	}
	
	// Reset all flags
	redir_erra=0;
	redir_app=0;
	redir_err=0;
	redir_out=0;
	redir_in=0;
}