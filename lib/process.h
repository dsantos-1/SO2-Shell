// Danilo Marques Araujo dos Santos - 8598670

#ifndef _PROCESS_H_
#define _PROCESS_H_

int parser(const char *, char **, int *);

void procBg(char **);

void procFg(char **);

void procDirPipe(char *, int , int ifd[2], int fdstd[2]);

void setPath(char *);

void resetFlags();

#endif