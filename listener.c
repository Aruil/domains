#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "hlpfncts.h"

int main(int argc, char *argv[]){
    int     fd[2];
    static struct sigaction interrupt;
    // Set SIGINT to ignore so the listener exits when it receives SIGUSR1
    interrupt.sa_handler = SIG_IGN;
    sigaction(SIGINT, &interrupt, NULL);

    if(argc != 4){
        fprintf(stderr, "Incorrect amount of arguments.\n");
    }

    fd[READ] = atoi(argv[2]);
    fd[WRITE] = atoi(argv[3]);

    close(fd[READ]);

    if(dup2(fd[WRITE], 1) == -1){
        perror("dup2");
    }else{
        execlp("inotifywait", "inotifywait", "-m", "-e", "create", "-r", argv[1],
               NULL);
        perror("execlp inotifywait");
    }

    exit(1);
}
