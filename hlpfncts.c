#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "hlpfncts.h"

int validPath(char *targetPath){
    DIR     *targetDIR;
    char    *pipesPath = NULL, *outputPath = NULL, *realPath = NULL;
    int     success = 0;
    // Get the absolute path of pipes and output directories
    pipesPath = realpath(PIPESDIR, NULL);
    outputPath = realpath(OUTDIR, NULL);
    // Check if the target path is a valid path
    if((targetDIR = opendir(targetPath)) != NULL){
        closedir(targetDIR);
        // Get the target path's absolute path
        realPath = realpath(targetPath, NULL);
        // Check if target path is either the pipes or output directory
        if(pipesPath == NULL || outputPath == NULL || realPath == NULL ||
           !strcmp(realPath, pipesPath) || !strcmp(realPath, outputPath)){
            success = -1;
        }
    }else{
        perror("opendir");
        success = -1;
    }

    if(pipesPath != NULL) free(pipesPath);
    if(outputPath != NULL) free(outputPath);
    if(realPath != NULL) free(realPath);
    return success;
}

void cleanup(int *fd, char *mPath){
    DIR             *pipesDir;
    struct dirent   *curFile;
    char            temp[20];
    // Delete all files inside pipes directory
    pipesDir = opendir(PIPESDIR);
    if(pipesDir != NULL){
        while((curFile = readdir(pipesDir)) != NULL){
            sprintf(temp, "%s/%s", PIPESDIR, curFile->d_name);
            unlink(temp);
        }
    }

    closedir(pipesDir);
    rmdir(PIPESDIR);
    if(mPath != NULL) free(mPath);
    if(fd != NULL){
        close(fd[READ]);
        close(fd[WRITE]);
    }
}

int formatMsg(char *msg){
    char        *createptr, *pipesPath = NULL, *outputPath = NULL,
                *realPath = NULL, *targetPath = NULL;
    long int    index;
    int         success = -1;
    // First formats the message from inotifywait so it is a valid path to the
    // new file (ignores temporary fs files)
    if((createptr = strstr(msg, "CREATE")) != NULL && createptr[6] == ' ' &&
       strncmp(&(createptr[7]), ".goutputstream-", 15)){
        index = createptr - msg;
        memmove(&(msg[(int) index - 1]), &(createptr[7]), strlen(msg) - (index + 6));
        // And then checks if the given file is inside the pipes or output folders
        pipesPath = realpath(PIPESDIR, NULL);
        outputPath = realpath(OUTDIR, NULL);
        targetPath = malloc(index);
        if(targetPath != NULL){
            strncpy(targetPath, msg, index - 1);
            targetPath[index - 1] = '\0';
            realPath = realpath(targetPath, NULL);
            if(pipesPath != NULL && outputPath != NULL && targetPath != NULL &&
               strcmp(pipesPath, realPath) && strcmp(outputPath, realPath)){
                success = 0;
            }
        }
    }

    if(pipesPath != NULL) free(pipesPath);
    if(outputPath != NULL) free(outputPath);
    if(realPath != NULL) free(realPath);
    if(targetPath != NULL) free(targetPath);
    return success;
}

