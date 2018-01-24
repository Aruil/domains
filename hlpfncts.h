#ifndef _HLPFNCTS_H_
#define _HLPFNCTS_H_

#define PIPESDIR    "./pipes"
#define OUTDIR      "./output"
#define READ        0
#define WRITE       1
#define BUFFER_SIZE 1024

int validPath(char *targetPath);

void cleanup(int *fd, char *mPath);

int formatMsg(char *msg);

#endif
