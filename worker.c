#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "hlpfncts.h"
#include "domainList.h"

int running;

void manager_exit(int signo){
    // Received SIGUSR1 so do some cleanup and exit
    running = 0;
}

int main(int argc, char *argv[]){
    char        filename[BUFFER_SIZE], buffer[BUFFER_SIZE], *outfile = NULL,
                http[8], domain[BUFFER_SIZE / 2], *temp = NULL, *token, *oldToken;
    int         fd, outfd, i, j, foundDomain, foundStop, err = 0, pipefd, foundDash,
                foundHTTP, len, validDomain;
    domainList  *list;
    ssize_t     bytes_read;
    pid_t       pid;
    static struct sigaction interrupt, user1;
    sigset_t                set1;
    // Set SIGINT to ignore and SIGUSR1 to stop the processes after cleaning up
    interrupt.sa_handler = SIG_IGN;
    user1.sa_handler = manager_exit;
    sigaction(SIGINT, &interrupt, NULL);
    sigaction(SIGUSR1, &user1, NULL);
    sigfillset(&set1);
    running = 1;
    // Get the process ID and open the correspoding pipe
    pid = getpid();
    if((pipefd = open(argv[1], O_RDONLY)) == -1){
        fprintf(stderr, "worker %d ", pid);
        perror("open pipe");
        kill(getppid(), SIGUSR2);
        exit(1);
    }

    sigprocmask(SIG_UNBLOCK, &set1, NULL);
    while(running){
        // Read the filename from the pipe
        sigprocmask(SIG_BLOCK, &set1, NULL);
        while(running && (bytes_read = read(pipefd, filename, BUFFER_SIZE)) <= 0){
            fprintf(stderr, "worker %d ", pid);
            perror("read pipe");
        }
        sigprocmask(SIG_UNBLOCK, &set1, NULL);
        if(running){
            filename[bytes_read] = '\0';
            // Open the necessary files
            if((fd = open(filename, O_RDONLY)) == -1){
                fprintf(stderr, "worker %d %s", pid, filename);
                perror("open");
                err = -1;
            }else{
                // Create a list to keep the found domains
                if((list = dl_create()) == NULL){
                    err = -1;
                }else{
                    outfile = malloc(strlen(filename) + 14);
                    if(outfile == NULL){
                        perror("outfile malloc");
                        fprintf(stderr, "worker %d ", pid);
                        err= -1;
                    }else{
                        // Save the filename and generate the .out filename
                        len = strlen(filename);
                        temp = malloc(len + 1);
                        temp[len] = '\0';
                        strcpy(temp, filename);
                        token = strtok(filename, "/");
                        while(token != NULL){
                            oldToken = token;
                            token = strtok(NULL, "/");
                        }
                        sprintf(outfile, "%s/%s.out", OUTDIR, oldToken);
                    }
                }
            }
        }
        // Read from the file to a buffer and parse the buffer to find domains
        if(!err){
            foundDomain = -1;
            foundHTTP = -1;
            while(running && (bytes_read = read(fd, buffer, BUFFER_SIZE)) != 0){
                if(bytes_read == -1){
                    continue;
                }else if(bytes_read != -1 && bytes_read < BUFFER_SIZE){
                    buffer[bytes_read] = '\0';
                }
                for(i = 0; i < bytes_read; i++){
                    if(!running){
                        break;
                    }else if(buffer[i] == 'h' && foundDomain){
                        // If an 'h' is found and we are not checking for domains
                        // start checking for "http://"
                        foundHTTP = 0;
                        http[0] = 'h';
                        j = 1;
                    }else if(!foundHTTP){
                        // If we are checking for "http://" store the string in
                        // http[8] and when we have read 7 characters check for
                        // equality
                        if(j < 7){
                            http[j] = buffer[i];
                            j++;
                            if(j == 7){
                                http[7] = '\0';
                                if(!strcmp(http, "http://")){
                                    foundDomain = 0;
                                    validDomain = 0;
                                    foundStop = 0;
                                    foundDash = -1;
                                }
                                j = 0;
                                foundHTTP = -1;
                            }
                        }
                    }else if(buffer[i] == '.' && !foundDomain && !validDomain){
                        if(j == 0 || (!foundStop && j == foundStop + 1) || !foundDash){
                            // If a fullstop is found at the beginning of the domain
                            // or is following a dash or another fullstop the domain
                            // is not valid
                            validDomain = -1;
                        }else{
                            if(foundStop){
                                // If a second fullstop is found "delete" the first
                                // part of the domain name and continue
                                j -= (foundStop + 1);
                                memmove(&(domain[0]), &(domain[foundStop + 1]), j);
                            }
                            foundStop = j;
                            domain[j] = '.';
                            j++;
                        }
                    }else if(buffer[i] == '/' && !foundDomain){
                        if(!validDomain && foundDash && foundStop){
                            // If the backslash is found after a fullstop or a dash
                            // it's not a valid domain. Also if there are no fullstops
                            // in the domain.
                            domain[j] = '\0';
                            dl_add(list, domain);
                        }
                        foundDomain = -1;
                    }else if(!foundDomain && !validDomain){
                        if(isalnum(buffer[i]) || (buffer[i] == '-' && j != 0 &&
                           (isalnum(domain[j - 1]) || domain[j - 1] == '-'))){
                            // The only valid domain characters are a-z A-Z 0-9,
                            // dashes and fullstops
                            if(buffer[i] == '-'){
                                foundDash = 0;
                            }else{
                                foundDash = -1;
                            }
                            domain[j] = buffer[i];
                            j++;
                        }else{
                            validDomain = -1;
                        }
                    }
                }
            }
        }

        if(err){
            err = 0;
        }else if(running){
            if(list->size > 0){
                if((outfd = open(outfile, O_WRONLY | O_CREAT, 0666)) == -1){
                    perror("open out_file");
                    fprintf(stderr, "worker %d ", pid);
                    err = -1;
                }
                if(dl_print(list, outfd)){
                    dl_destroy(list);
                }
                close(outfd);
            }else{
                printf("%s contained no valid urls.\n", temp);
            }
            free(outfile);
            outfile = NULL;
        }
        free(temp);
        temp = NULL;
        if(running) kill(pid, SIGSTOP);
    }

    dl_destroy(list);
    free(list);
    if(outfile != NULL) free(outfile);
    if(temp != NULL) free(temp);
    close(pipefd);
    exit(0);
}
