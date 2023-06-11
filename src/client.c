#include <client.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

client_t * client_create(const char * username)
{
    client_t * client = NULL;

    if (!username)
    {
        goto client_create_return;
    }

    client = calloc(1, sizeof(*client));

    if (!client)
    {
        goto client_create_return;
    }

    client->username = strdup(username);

    if (!client->username)
    {
        goto cleanup;
    }

    goto client_create_return;

cleanup:
    free(client);
    client = NULL;
client_create_return:
    return client;
}

int client_compare(const void * data1, const void * data2)
{
    if (!data1 || !data2)
    {
        return 0;
    }

    client_t * client1 = (client_t *)data1;
    client_t * client2 = (client_t *)data2;
    int rv = 0;

    if (client1->id > client2->id)
    {
        rv = -1;
    }
    else if (client1->id < client2->id)
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

void client_destroy(void * data)
{
    if (!data)
    {
        return;
    }

    client_t * client = (client_t *)data;
    free(client->username);
    client->username = NULL;
    client->id = 0;
    close(client->fd);
    client->fd = -1;
    free(client);
}
// END OF SOURCE