// Danilo Marques Araujo dos Santos - 8598670

#include "syserror.h"
#include "process.h"
#include "jobs.h"

int nextJid=1;

// Signal block mask
sigset_t maskchld;

// Path to shell and current directory
int shell_terminal;
pid_t shell_pgid;
char *ptr, *ag;

// Job list
struct job_t jobs[MAXJOBS];

void run(char *);

int checkDirPipe(char *);

int builtIn(char **, int);

void cd(char **, int);

void bgFgKill(char **, int);

int main(int argc, char **argv) {
	char cmdline[MAXLINE];
	char *bu;
	
	// Clears the screen
	system("clear");
	
	// Prints welcome message
	
	printf("\033[1m\033[93m    _.._                                        \n");
    printf("  .' .-'`                                       \n");
    printf(" /  /                                           \n");
    printf(" |  |    Welcome to Luna Shell!                 \n");
    printf(" \\  \\    Developed by: Danilo Santos (8598670)\n");
    printf("  '._'-._                                       \n");
    printf("     ```                                        \033[00m\n");

	// Create a block mask
	Sigemptyset(&maskchld);
	Sigaddset(&maskchld, SIGCHLD);
	Sigaddset(&maskchld, SIGTSTP);

	shell_terminal=STDIN_FILENO;

	// Get the shell execution directory
	long size=pathconf(".", _PC_PATH_MAX);
	if((bu=(char *)malloc((size_t)size))!=NULL)
		ptr=getcwd(bu,(size_t)size);

	// Redirect stderr to stdout, for pipe usage
	dup2(1, 2);

	// Ignore job control signals
	Signal(SIGINT, SIG_IGN);
	Signal(SIGTSTP, SIG_IGN);
	Signal(SIGCHLD, SIG_IGN);

	// Connect the signals to handler
	Signal(SIGINT, ctrlC_hdlr);
	Signal(SIGTSTP, ctrlZ_hdlr);
	Signal(SIGCHLD, child_hdlr);

	while(tcgetpgrp(shell_terminal)!=(shell_pgid=getpgrp()))
		Kill(-shell_pgid, SIGTTIN);

	// Placing the shell in its own group
	shell_pgid=getpid();
	Setpgid(shell_pgid, shell_pgid);
	tcsetpgrp(shell_terminal, shell_pgid);

	// Start job collection
	startJobs(jobs);

	while(1) {
		// Get current directory and prints shell name
		long size=pathconf(".", _PC_PATH_MAX);
		char *buf;
	
		if((buf=(char *)malloc((size_t)size))!=NULL) {
			ag=getcwd(buf,(size_t)size);
		}
	
		if(!strcmp(ag, ptr)) {
			fprintf(stdout, "\033[1m\033[92muser@luna\033[37m\033[00m:\033[1m\033[94m~\033[00m$ ");
		}
		else {
			fprintf(stdout, "\033[1m\033[92muser@luna\033[37m\033[00m:\033[1m\033[94m%s\033[00m$ ", ag);
		}
	
		// Read the typed line and checks for errors
		if((fgets(cmdline, MAXLINE, stdin)==NULL) && ferror(stdin)) {
			printf("FGETS ERROR!\n");
			exit(EXIT_FAILURE);
		}
	
		// Check if Ctrl+D was pressed
		if(feof(stdin)) {
			fflush(stdout);
			exit(EXIT_SUCCESS);
		}
	
		// Check if something was typed
		if(strlen(cmdline)>1) {
			// Run the line
			run(cmdline);
		
			// Reconnect the signals to handlers
			Signal(SIGINT, ctrlC_hdlr);
			Signal(SIGTSTP, ctrlZ_hdlr);
			Signal(SIGCHLD, child_hdlr);
		}
	}

	exit(EXIT_SUCCESS);
}

// Direct the process to the correct function
void run(char *cmdline) {
	char *argv[MAXARGS];
	int argc=0;

	// Check if the program will run in background
	int bg=parser(cmdline, argv, &argc);

	// Check if a command is built-in
	if(builtIn(argv, argc))
		return;

	// Check if the command has pipe/forwarding
	if(!checkDirPipe(cmdline)) {
		// Run in background
		if(bg) {
			procBg(argv);
		}
	
		// Run in foreground
		else {
			procFg(argv);
		}
	}

	else {
		int fdstd[2];
		
		setPath(cmdline);
		procDirPipe(cmdline, 0, NULL, fdstd);
		resetFlags();
	}
	
	return;
}

// Check if a command has pipe/redirection
int checkDirPipe(char *cmdline) {
	char c, arg[MAXLINE];
	int k, size, i, ct;
	char **argv;

	argv=(char **)calloc(MAXARGS, MAXLINE);
	size=strlen(cmdline);
	cmdline[size]='\0';
	ct=0;
	k=0;
	
	do {
		argv[ct]=(char *)malloc(sizeof(arg));
		i=0;
		
		while((c=cmdline[k])!=' ') {
			if(c=='\0')
				break;
			*(argv[ct]+i)=c;
			i++;
			k++;
		}
		
		*(argv[ct]+i)='\0';
		ct++;
		k++;
	} while(c!='\0');

	for(i=0; i<ct; i++) {
		if(!strcmp(argv[i], ">") || !strcmp(argv[i], "<<") || !strcmp(argv[i], "<") ||
		   !strcmp(argv[i], ">>") || !strcmp(argv[i], "|") || !strcmp(argv[i], "1>") ||
		   !strcmp(argv[i], "2>") || !strcmp(argv[i], "1>>") || !strcmp(argv[i], "2>>"))
			return 1;
	}
	
	return 0;
}

// Check if a command is built-in
int builtIn(char **argv, int numberofargs) {
	if(!strcmp(argv[0], "exit")) {
		exit(EXIT_SUCCESS);
	}
	else if(!strcmp(argv[0], "fg") || !strcmp(argv[0], "bg") || !strcmp(argv[0], "kill")) {
		bgFgKill(argv, numberofargs);
		return 1;
	}
	else if(!strcmp(argv[0], "cd")) {
		cd(argv, numberofargs);
		return 1;
	}
	else if(!strcmp(argv[0], "jobs")) {
		listJobs(jobs);
		return 1;
	}
	
	return 0;
}

// Change the shell working directory
void cd(char **argv, int tam) {
	char *pt;
	int i;

	// Path contains spaces
	if(tam>2) {
		if((pt=(char *)malloc(200))!=NULL) {
			for(i=1; i<tam; i++) {
				if(i!=1)
					strcat(pt, " ");
				strcat(pt, argv[i]);
			}
		}
		if(chdir(pt)<0)
			printf("luna: cd: %s: No such file or directory\n", pt);
	}
	
	// Path doesn't contain spaces
	else if(tam==2) {
		if(chdir(argv[1])<0)
			printf("luna: cd: %s: No such file or directory\n", argv[1]);
	}
	
	// No path. Returns to the directory where the program is run
	else if(tam==1) {
		if(chdir(ptr)<0)
			printf("luna: cd: %s: No such file or directory\n", ptr);
	}
}

// Put a process in fg/bg or kills it
void bgFgKill(char **argv, int numberofargs) {
	int pid, jid;
	int i=0;

	if(numberofargs>1) {
		// The JID has been typed
		if(argv[1][0]=='%') {
			jid=atoi(argv[1]+1);
			
			while((i<MAXJOBS) && (jobs[i].jid!=jid))
				i++;
		}
		
		// The PID has been typed
		else {
			pid=atoi(argv[1]);
			
			if(pid<=0) {
				printf("luna: %s: PID or JID is necessary\n", argv[0]);
				
				return;
			}
			
			else {
				while((i<MAXJOBS) && (jobs[i].pid!=pid))
					i++;
			}
		}

		if((i>=0) && (i<MAXJOBS)) {
			int actualstate=jobs[i].state;
			
			// Stopped process and bg was typed
			if((actualstate==ST) && !strcmp(argv[0], "bg")) {
				// Sends signal to continue
				Kill(-jobs[i].pid, SIGCONT);
				
				// Changes state to bg
				jobs[i].state=BG;
				
				printf("[%d](%d) %s", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
			}
			
			// fg was typed
			else if(!strcmp(argv[0], "fg")) {
				// Sends signal to continue
				Kill(-jobs[i].pid, SIGCONT);
				
				// Changes state to fg
				jobs[i].state=FG;
				
				// Waits till the process ends or be stopped
				waitFg(jobs[i].pid);
			}
			
			// If the current state is bg and kill was typed
			else if(actualstate==BG && !strcmp(argv[0], "kill")) {
				// Terminal signal successfully sent
				if(!kill(-jobs[i].pid, SIGTERM)) {
					printf("luna: job [%d](%d) successfully killed\n", jobs[i].jid, jobs[i].pid);
					removeJob(jobs, jobs[i].pid);
					nextJid=maxJid(jobs)+1;
				}
				
				// The signal was not successfully sent
				else {
					fprintf(stderr, "luna: job [%d](%d) could not be killed\n", jobs[i].jid, jobs[i].pid);
				}
			}
		}
		
		// Job wasn't found
		else {
			if(argv[1][0]=='%') {
				printf("luna: %s: job not found\n", argv[1]);
			}
			else {
				printf("luna:(%s): process not found\n", argv[1]);
			}
		}
	}
	
	// JID wasn't typed
	else {
		printf("luna: %s: PID or JID is necessary\n", argv[0]);
	}
}