// Danilo Marques Araujo dos Santos - 8598670

#include "syserror.h"
#include "jobs.h"

// Sleeps until the process that's running in foreground changes its state
void waitFg(pid_t pid) {
	struct job_t *job;
	
	while((job=getJobPid(jobs, pid))!=NULL && (job->state==FG)) {
		sleep(0);
	}
}

// Resets all fields of the struct of an job
void resetJob(struct job_t *job) {
	job->cmdline[0]='\0';
	job->state=UNDEF;
	job->jid=0;
	job->pid=0;
}

// Starts a new job
void startJobs(struct job_t *jobs) {
	int i;
	
	for(i=0; i<MAXJOBS; i++)
		resetJob(&jobs[i]);
}

// Returns the maximum JID
int maxJid(struct job_t *jobs) {
	int i, max=0;
	
	for(i=0; i<MAXJOBS; i++)
		if(jobs[i].jid>max)
			max=jobs[i].jid;
	
	return max;
}

// Adds a new job to a list of jobs
int addJob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
	int i;
	
	if(pid<1) {
		return 0;
	}
	for(i=0; i<MAXJOBS; i++) {
		if(jobs[i].pid==0) {
			jobs[i].jid=nextJid++;
			jobs[i].state=state;
			jobs[i].pid=pid;
			
			if(nextJid>MAXJOBS)
				nextJid=1;
			
			strcpy(jobs[i].cmdline, cmdline);
			return 1;
			
		}
	}
	
	printf("luna: there are too many jobs\n");
	
	return 0;
}

// Removes a job from a list of jobs
int removeJob(struct job_t *jobs, pid_t pid) {
	int i;
	
	if(pid<1) {
		return 0;
	}
	for(i=0; i<MAXJOBS; i++) {
		if(jobs[i].pid==pid) {
			resetJob(&jobs[i]);
			nextJid=maxJid(jobs)+1;
			
			return 1;
		}
	}
	
	return 0;
}

// Stays in a while loop until the process that's running in foreground changes its state
struct job_t *getJobPid(struct job_t *jobs, pid_t pid) {
	int i;
	
	if(pid<1)
		return NULL;
	
	for(i=0; i<MAXJOBS; i++)
		if(jobs[i].pid==pid)
			return &jobs[i];
		
	return NULL;
}

// Gets an JID through its PID
int pidToJid(pid_t pid) {
	int i;
	
	if(pid<1)
		return 0;
	
	for(i=0; i<MAXJOBS; i++)
		if(jobs[i].pid==pid)
			return jobs[i].jid;
	
	return 0;
}

// Prints all jobs from a collection
void listJobs(struct job_t *jobs) {
	int i;

	printf("[JOBID](PID)\n");
	for(i=0; i<MAXJOBS; i++) {
		if(jobs[i].pid!=0) {
			printf("[%d]	(%d) ", jobs[i].jid, jobs[i].pid);
			switch(jobs[i].state) {
				case BG:
					printf("Background ");
					break;
				case FG:
					printf("Foreground ");
					break;
				case ST:
					printf("Stopped	 ");
					break;
				default:
					printf("listJobs: Internal error: job[%d].state=%d ", i, jobs[i].state);
			}
			
			printf("%s", jobs[i].cmdline);
		}
	}
}

// Adds a signal to a handler
handler_t *Signal(int signum, handler_t *handler) {
	struct sigaction action, old_action;

	action.sa_handler=handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags=SA_RESTART;

	if(sigaction(signum, &action, &old_action)<0)
		unixError("Signal error");
	
	return(old_action.sa_handler);
}

// Handler to Ctrl+C
void ctrlC_hdlr(int sig) {
	int i;
	
	for(i=0; i<MAXJOBS; i++) {
		if(jobs[i].state==FG) {
			Kill(-jobs[i].pid, SIGINT);
		}
	}
}

// Handler to Ctrl+Z
void ctrlZ_hdlr(int sig) {
	int i;
	
	for(i=0; i<MAXJOBS; i++) {
		if(jobs[i].state==FG) {
			Kill(-jobs[i].pid, SIGTSTP);
		}
	}
}

// Checks the signal of the waiting process
void child_hdlr(int sig) {
	int status=-1;
	int pid=0;
	
	do {	
		pid=waitpid(-1, &status, WNOHANG|WUNTRACED);
		
		if(pid>0) {
			// Child normally finished
			if(WIFEXITED(status)) {
				removeJob(jobs, pid);
			}
			
			// Child incorrectly finished
			else if(WIFSIGNALED(status)) {
				if(WTERMSIG(status)==2) {
					printf("luna: job [%d](%d) killed\n", pidToJid(pid), pid);
					removeJob(jobs, pid);
				}
			}
			
			// Child stopped
			else if(WIFSTOPPED(status)) {
				getJobPid(jobs, pid)->state=ST;
				printf("luna: job [%d](%d) stopped\n", pidToJid(pid), pid);
			}
		}
	} while(pid>0);
}