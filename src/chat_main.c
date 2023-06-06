#include <chat_main.h>

#define MAX_MSG_SZ 512
#define MAX_USERNAME_SZ 10

typedef enum {
    CONNECT,
    MESSAGE,
    NUMBER_OF_TYPES,
} packet_type_t;

typedef struct {
    int * clients;
    tcp_server_t * tcp_server;
    eloop_t * eloop;
} chat_server_t;

typedef struct {
    eloop_t * eloop;
    int * client_fd;
} connection_data_t;

static chat_server_t * chat_server_init(size_t allowed_clients, const char * port);
static void chat_server_destroy(chat_server_t * server);
static int connection_event(void * data);
static int connection_destroy(void * data);
static int message_event(void * data);
static int message_destroy(void * data);
static event_t * generate_event(packet_type_t type, void * data);

int main(void)
{
    int rv = 0;
    connection_data_t * connection_data = NULL;
    event_t * event = NULL;
    chat_server_t * server = chat_server_init(10, "44567");

    if (!server)
    {
        rv = -1;
        goto main_return;
    }
    
    for (;;)
    {

        int client_fd = tcp_server_accept(server->tcp_server);

        if (client_fd <= 0)
        {
            continue;
        }

        connection_data = calloc(1, sizeof(*connection_data));

        if (!connection_data)
        {
            close(client_fd);
            continue;
        }

        connection_data->client_fd = calloc(1, sizeof(*(connection_data->client_fd)));

        if (!(connection_data->client_fd))
        {
            free(connection_data);
            close(client_fd);
            continue;
        }
        
        *(connection_data->client_fd) = client_fd;
        connection_data->eloop = server->eloop;
        event = generate_event(CONNECT, connection_data);

        if (!event)
        {
            *(connection_data->client_fd) = -1;
            connection_data->eloop = NULL;
            free(connection_data->client_fd);
            free(connection_data);
            close(client_fd);
            continue;
        }

        eloop_add(server->eloop, event);
        connection_data = NULL;
        event = NULL;

        sleep(10);
        break;
    }

    chat_server_destroy(server);

main_return:
    return rv;
}

static chat_server_t * chat_server_init(size_t allowed_clients, const char * port)
{
    if (allowed_clients == 0)
    {
        return NULL;
    }

    chat_server_t * server = calloc(1, sizeof(*server));

    if (!server)
    {
        goto init_return;
    }

    server->clients = calloc(allowed_clients, sizeof(*(server->clients)));

    if (!(server->clients))
    {
        goto cleanup;
    }

    server->eloop = eloop_create();

    if (!(server->eloop))
    {
        goto cleanup;
    }

    server->tcp_server = tcp_server_setup(port);

    if (!(server->tcp_server))
    {
        goto cleanup;
    }

    goto init_return;

cleanup:
    chat_server_destroy(server);
    server = NULL;
init_return:
    return server;
}

static void chat_server_destroy(chat_server_t * server)
{
    if (!server)
    {
        goto destroy_return;
    }

    tcp_server_teardown(server->tcp_server);
    eloop_destroy(server->eloop);
    free(server->clients);
    free(server);

destroy_return:
    return;
}

static int connection_event(void * data)
{
    int rv = 0;
    connection_data_t * event_data = (connection_data_t *)data;
    int * client_fd = event_data->client_fd;
    uint32_t packet_type = 0;
    size_t nread = tcp_read_all(*client_fd, &packet_type, sizeof(packet_type));

    if (nread != sizeof(packet_type))
    {
        rv = -1;
        goto cleanup;
    }

    packet_type = ntohl(packet_type);

    if (packet_type >= NUMBER_OF_TYPES)
    {
        rv = -1;
        goto cleanup;
    }
    else
    {
        event_t * event = generate_event(packet_type, client_fd);

        if (!event)
        {
            goto cleanup;
        }
        eloop_add(event_data->eloop, event);
        goto event_return;
    }

cleanup:
    close(*client_fd);
    *client_fd = -1;
event_return:
    return rv;
}

static int connection_destroy(void * data)
{
    event_t * event = (event_t *)data;
    connection_data_t * event_data = (connection_data_t *)event->data;

    if (*(event_data->client_fd) == -1)
    {
        free(event_data->client_fd);
    }

    event_data->client_fd = NULL;
    event_data->eloop = NULL;
    free(event_data);
    event->data = NULL;
    free(event);
    return 0;
}

static int message_event(void * data)
{
    int rv = 0;
    int * client_fd = (int *)data;
    uint32_t username_sz = 0;
    uint32_t message_sz = 0;
    int nread = tcp_read_all(*client_fd, &username_sz, sizeof(username_sz));
    
    if (nread != sizeof(username_sz))
    {
        fputs("error: username size\n", stderr);
        rv = -1;
        goto message_event_return;
    }

    nread = tcp_read_all(*client_fd, &message_sz, sizeof(message_sz));

    if (nread != sizeof(message_sz))
    {
        fputs("error: message size\n", stderr);
        rv = -1;
        goto message_event_return;
    }

    username_sz = ntohl(username_sz);
    message_sz = ntohl(message_sz);

    if (username_sz > MAX_USERNAME_SZ || message_sz > MAX_MSG_SZ)
    {
        fputs("error: username/message max size\n", stderr);
        rv = -1;
        goto message_event_return;
    }

    size_t buffer_sz = username_sz + message_sz + 2;
    char * buffer = calloc(buffer_sz, sizeof(*buffer));

    if (!buffer)
    {
        fputs("error: buffer allocation\n", stderr);
        rv = -1;
        goto message_event_return;
    }

    nread = tcp_read_all(*client_fd, buffer, username_sz);

    if (nread != username_sz)
    {
        fputs("error: username read\n", stderr);
        rv = -1;
        goto cleanup;
    }

    char * buffer_ptr = buffer + username_sz + 1;
    nread = tcp_read_all(*client_fd, buffer_ptr, message_sz);

    if (nread != message_sz)
    {
        fputs("error: message read\n", stderr);
        rv = -1;
        goto cleanup;
    }

    printf("%s: %s\n", buffer, (buffer + username_sz + 1));

cleanup:
    free(buffer);
    buffer = NULL;
message_event_return:
    return rv;
}

static int message_destroy(void * data)
{
    event_t * event = (event_t *)data;
    int * client_fd = event->data;
    close(*client_fd);
    free(client_fd);
    event->data = NULL;
    event->dfunc = NULL;
    event->efunc = NULL;
    free(event);
    return 0;
}

static event_t * generate_event(packet_type_t type, void * data)
{
    event_t * event = NULL;
    
    if (type >= NUMBER_OF_TYPES)
    {
        goto generate_return;
    }

    event = calloc(1, sizeof(*event));

    if (!event)
    {
        goto generate_return;
    }

    event->data = data;

    switch(type)
    {
        case CONNECT:
            event->efunc = connection_event;
            event->dfunc = connection_destroy;
            break;
        case MESSAGE:
            event->efunc = message_event;
            event->dfunc = message_destroy;
            break;
        default:
            free(event);
            event = NULL;
            break;
    }

generate_return:
    return event;
}
// END OF SOURCE