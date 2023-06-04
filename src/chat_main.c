#include <chat_main.h>

#define BUFFSZ 512

static int connection_event(void * data);
static int connection_destroy(void * data);

int main(void)
{
    tcp_server_t * server = tcp_server_setup("44567");
    eloop_t * eloop = eloop_create();

    if (!server || !eloop)
    {
        goto main_exit;
    }

    int * client = calloc(1, sizeof(*client));
    *client = tcp_server_accept(server);

    if (*client > 0)
    {
        event_t * event = calloc(1, sizeof(*event));
        event->data = client;
        event->efunc = connection_event;
        event->dfunc = connection_destroy;

        eloop_add(eloop, event);
    }

main_exit:
    eloop_destroy(eloop);
    tcp_server_teardown(server);
}

static int connection_event(void * data)
{
    int client = *(int *)data;
    char buffer[BUFFSZ];
    memset(buffer, 0, BUFFSZ);
    uint32_t msg_sz = 0;
    size_t nread = tcp_read_all(client, &msg_sz, sizeof(msg_sz));

    
    if (nread != sizeof(msg_sz))
    {
        close(client);
        return -1;
    }

    msg_sz = ntohl(msg_sz);
    printf("Message length: %u\n", msg_sz);
    nread = tcp_read_all(client, buffer, msg_sz);

    if(nread == msg_sz)
    {
        printf("Message: %s\n", buffer);
    }
    else
    {
        printf("Error on read\n");
    }

    close(client);
    return 0;
}

static int connection_destroy(void * data)
{
    event_t * event = (event_t *)data;
    free(event->data);
    event->data = NULL;
    event->efunc = NULL;
    event->dfunc = NULL;
    free(event);
    return 0;
}
// END OF SOURCE