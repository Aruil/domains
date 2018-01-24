#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "hlpfncts.h"
#include "domainList.h"

domainList *dl_create(){
    domainList *newList;

    newList = malloc(sizeof(domainList));
    if(newList == NULL){
        perror("dl_create malloc");
    }else{
        newList->size = 0;
        newList->start = NULL;
    }

    return newList;
}

int dl_add(domainList *list, char *domain){
    listNode    *cur = list->start, *newNode;
    int         length;

    length = strlen(domain);

    if(cur == NULL){
        newNode = ln_create(domain, length);
        if(newNode == NULL) return -1;
        list->start = newNode;
        list->size++;
    }else{
        while(cur != NULL){
            if(!strcmp(domain, cur->domain)){
                cur->freq++;
                break;
            }else if(cur->next == NULL){
                newNode = ln_create(domain, length);
                if(newNode == NULL) return -1;
                cur->next = newNode;
                list->size++;
                break;
            }
            cur = cur->next;
        }
    }

    return 0;
}

listNode *ln_create(char *domain, int domain_len){
    listNode *newNode;

    newNode = malloc(sizeof(listNode));
    if(newNode == NULL){
        perror("ln_create node malloc");
    }else{
        newNode->domain = malloc(domain_len + 1);
        if(newNode->domain == NULL){
            perror("ln_create node->domain malloc");
            free(newNode);
            newNode = NULL;
        }else{
            strcpy(newNode->domain, domain);
            newNode->freq = 1;
            newNode->domain_len = domain_len;
            newNode->next = NULL;
        }
    }

    return newNode;
}

int dl_print(domainList *list, int fd){
    listNode    *cur = list->start, *toPrint;
    char        temp[30];
    int         length;

    while(cur != NULL){
        toPrint = cur;
        list->start = cur->next;
        list->size--;
        cur = cur->next;
        sprintf(temp, "\t %d\n", toPrint->freq);
        if(write(fd, toPrint->domain, toPrint->domain_len) < toPrint->domain_len){
            perror("dl_print write domain");
            free(toPrint->domain);
            free(toPrint);
            return -1;
        }
        free(toPrint->domain);
        free(toPrint);
        length = strlen(temp);
        if(write(fd, temp, length) < length){
            perror("dl_print write freq");
            return -1;
        }
    }

    return 0;
}

void dl_destroy(domainList *list){
    listNode *cur = list->start, *toDelete;

    while(cur != NULL){
        toDelete = cur;
        cur = cur->next;
        free(toDelete->domain);
        free(toDelete);
    }
    list->start = NULL;
    list->size = 0;
}
