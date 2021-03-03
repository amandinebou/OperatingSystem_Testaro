#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

void treatLine(char *array, char *buff, char *accumulateurEntree, char *accumulateurSortie);
void testFunction(char* accumulateurEntree, char* accumulateurSortie, char* cmd);
void handler();
void signalreceived_handler(int signo);