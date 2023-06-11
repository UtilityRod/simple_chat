#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdint.h>

typedef struct {
    int fd;
    uint32_t id;
    char * username;
} client_t;

client_t * client_create(const char * username);
int client_compare(const void * data1, const void * data2);
void client_destroy(void * data);

#endif