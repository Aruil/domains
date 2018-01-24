#ifndef _DOMAINLIST_H_
#define _DOMAINLIST_H_

typedef struct listNode listNode;
typedef struct domainList domainList;

struct domainList{
    int         size;
    listNode    *start;
};

struct listNode{
    char        *domain;
    int         freq;
    int         domain_len;
    listNode    *next;
};

domainList *dl_create();

int dl_add(domainList *list, char *domain);

listNode *ln_create(char *domain, int domain_len);

int dl_print(domainList *list, int fd);

void dl_destroy(domainList *list);

#endif
