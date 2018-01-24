#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "hlpfncts.h"
#include "workersQueue.h"

wQueue  *working, *sleeping;
int     workersNo, running, waitWorker;
pid_t   listenerPID;

void worker_finished(int signo){
    // Signal handler for SIGCHLD while the proccess is in running state
    pid_t   pid;
    int     status;
    worker  *wrk;

    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
        if(WIFSTOPPED(status)){
            wrk = remove_worker(working, pid);
            if(wrk != NULL){
                add_existWorker(sleeping, wrk);
            }
        }
    }
}

void worker_exit(int signo){
    // Signal handler for SIGCHLD while process is exiting
    pid_t pid;

    while((pid = waitpid(-1, NULL, WNOHANG)) > 0){
        if(pid != listenerPID) workersNo--;
    }
}

void exiting(int signo){
    // Received SIGINT
    running = 0;
}

void worker_ready(int signo){
    // Signal handler for SIGUSR2 for when the new worker failed to open pipe
    waitWorker = -1;
}

int main(int argc, char *argv[]){
    int     fd[2], i, j, splitMsg = -1, fifoFD, success;
    char    pipefd[2][5], buffer[BUFFER_SIZE], msg[BUFFER_SIZE],
            *mPath = NULL, fifo[30];
    worker  *wrk;
    pid_t   pid;
    ssize_t bytes_read;
    static struct sigaction child, interrupt, wait_child, wait_worker;
    sigset_t                set1;
    // Set handlers for SIGINT, SIGCHLD and SIGUSR2
    child.sa_handler = worker_finished;
    wait_child.sa_handler = worker_exit;
    interrupt.sa_handler = exiting;
    wait_worker.sa_handler = worker_ready;
    sigfillset(&(wait_worker.sa_mask));
    sigfillset(&(interrupt.sa_mask));
    sigfillset(&(child.sa_mask));
    sigfillset(&(wait_child.sa_mask));
    sigfillset(&set1);
    sigdelset(&set1, SIGUSR2);
    running = 1;
    // Read path from arguments and check if the path is valid or points to the
    // pipes or output directories
    if(argc != 1){
        if(!strcmp(argv[1], "-p")){
            mPath = malloc(strlen(argv[2]) + 1);
            strcpy(mPath, argv[2]);
        }else{
            fprintf(stderr, "Invalid argument.\n");
        }
    }else{
        mPath = malloc(2);
        strcpy(mPath, ".");
    }
    // Create the queues for working and waiting workers
    if((working = create_queue()) == NULL ||
       (sleeping = create_queue()) == NULL){
        free(mPath);
        exit(1);
    }
    workersNo = 0;
    // Create the directories for output and the named pipes for communication
    // with the workers. Also create the pipe for communication with the listener
    if(mkdir(PIPESDIR, 0777) == -1){
        perror("mkdir pipes");
        free(mPath);
        destroy_queue(working);
        destroy_queue(sleeping);
        exit(1);
    }
    if(mkdir(OUTDIR, 0777) == -1){
        if(errno != EEXIST){
            perror("mkdir output");
            cleanup(NULL, mPath);
            destroy_queue(working);
            destroy_queue(sleeping);
            exit(1);
        }
    }
    if(validPath(mPath)){
        fprintf(stderr, "Please enter a valid path (excluding ./pipes"
                " and ./output).\n");
        cleanup(NULL, mPath);
        destroy_queue(working);
        destroy_queue(sleeping);
        exit(1);
    }
    if(pipe(fd) == -1){
        perror("pipe M->L");
        cleanup(fd, mPath);
        destroy_queue(working);
        destroy_queue(sleeping);
        exit(2);
    }

    sprintf(pipefd[READ], "%d", fd[READ]);
    sprintf(pipefd[WRITE], "%d", fd[WRITE]);
    // Create the listener
    if((listenerPID = fork()) == -1){
        perror("fork");
        cleanup(fd, mPath);
        destroy_queue(working);
        destroy_queue(sleeping);
        exit(3);
    }

    if(listenerPID == 0){
        execlp("./listener", "./listener", mPath, pipefd[READ], pipefd[WRITE],
               NULL);
        perror("execlp listener");
    }else{
        close(fd[WRITE]);
        sigaction(SIGCHLD, &child, NULL);
        sigaction(SIGINT, &interrupt, NULL);
        sigaction(SIGUSR2, &wait_worker, NULL);
        while(running){
            bytes_read = 0;
            while(running && bytes_read <= 0){
                bytes_read = read(fd[READ], buffer, BUFFER_SIZE);
            }
            usleep(1000);
            i = 0;
            if(splitMsg) j = 0;
            while(running && i < bytes_read){
                if(buffer[i] == '\n'){
                    msg[j] = '\0';
                    j = 0;
                    if(!splitMsg) splitMsg = -1;
                    if(running && !formatMsg(msg)){
                        waitWorker = 0;
                        // This whole block is critical code so we don't want
                        // interruptions from signals
                        sigprocmask(SIG_BLOCK, &set1, NULL);
                        if(sleeping->size == 0){
                            // If there is no available worker
                            sprintf(fifo, "%s/pipe%d", PIPESDIR, workersNo);
                            success = -1;
                            while(success){
                                if(mkfifo(fifo, 0666) == -1){
                                    fprintf(stderr, "%s : failed to create pipe\n",
                                            msg);
                                    perror("mkfifo");
                                }else{
                                    if((pid = fork()) == -1){
                                        fprintf(stderr, "%s : failed to fork\n", msg);
                                        perror("fork");
                                        unlink(fifo);
                                    }else if(pid == 0){
                                        execlp("./worker", "./worker", fifo, NULL);
                                        fprintf(stderr, "%s : failed to exec\n", msg);
                                        perror("exec");
                                        unlink(fifo);
                                    }else{
                                        if(waitWorker ||
                                           (fifoFD = open(fifo, O_WRONLY)) < 0){
                                            if(!waitWorker){
                                                fprintf(stderr, "%s : failed to open\n",
                                                        msg);
                                                perror("fifo open");
                                                kill(pid, SIGKILL);
                                            }
                                            unlink(fifo);
                                        }else{
                                            add_newWorker(working, pid, fifoFD);
                                            while(write(fifoFD, msg, strlen(msg)) == -1){
                                                fprintf(stderr, "%s : failed to write.\n",
                                                        msg);
                                                perror("fifo write");
                                            }
                                            workersNo++;
                                            success = 0;
                                        }
                                    }
                                }
                            }
                        }else{
                            // If there is an available worker wake him
                            pid = sleeping->start->pid;
                            fifoFD = sleeping->start->pipeID;
                            kill(pid, SIGCONT);
                            if(write(fifoFD, msg, strlen(msg)) == -1){
                                fprintf(stderr, "%s : failed to write.\n", msg);
                                perror("fifo write");
                            }else{
                                wrk = remove_worker(sleeping, pid);
                                add_existWorker(working, wrk);
                            }
                        }
                        // We can accept signals now
                        sigprocmask(SIG_UNBLOCK, &set1, NULL);
                    }
                }else{
                    msg[j] = buffer[i];
                    j++;
                }
                i++;
            }
            if(j) splitMsg = 0;
        }
    }

    // Set SIGCHLD to the second handler and wait for all workers to exit
    sigaction(SIGCHLD, &wait_child, NULL);
    kill(listenerPID, SIGUSR1);
    wrk = sleeping->start;
    while(wrk != NULL){
        kill(wrk->pid, SIGUSR1);
        kill(wrk->pid, SIGCONT);
        wrk = wrk->next;
    }
    wrk = working->start;
    while(wrk != NULL){
        kill(wrk->pid, SIGUSR1);
        wrk = wrk->next;
    }

    while(workersNo);

    destroy_queue(working);
    destroy_queue(sleeping);
    close(fd[READ]);
    cleanup(NULL, mPath);
    exit(0);
}
