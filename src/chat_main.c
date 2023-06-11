#include <chat_main.h>

#define MAX_MSG_SZ 512
#define MAX_USERNAME_SZ 10

typedef enum {
    CONNECT,
    JOIN,
    NUMBER_OF_TYPES,
} packet_type_t;

typedef struct {
    tcp_server_t * tcp_server;
    eloop_t * eloop;
    circular_list_t * clients;
} chat_server_t;

typedef struct {
    chat_server_t * server;
    int client_fd;
} server_data_t;

static chat_server_t * chat_server_init(size_t allowed_clients, const char * port);
static void chat_server_destroy(chat_server_t * server);

static int connection_event(void * data);
static int connection_destroy(void * data);

static int join_event(void * data);
static int join_destroy(void * data);

static event_t * generate_event(packet_type_t type, void * data);
static void username_destroy(void * data);

int main(void)
{
    int rv = 0;
    server_data_t * server_data = NULL;
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

        server_data = calloc(1, sizeof(*server_data));

        if (!server_data)
        {
            close(client_fd);
            continue;
        }
        
        server_data->client_fd = client_fd;
        server_data->server = server;
        event = generate_event(CONNECT, server_data);

        if (!event)
        {
            close(client_fd);
            continue;
        }

        eloop_add(server->eloop, event);
        server_data = NULL;
        event = NULL;

        sleep(5);
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

    server->clients = circular_create(client_compare, client_destroy);

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

    circular_destroy(server->clients);
    server->clients = NULL;
    tcp_server_teardown(server->tcp_server);
    server->tcp_server = NULL;
    eloop_destroy(server->eloop);
    server->eloop = NULL;
    free(server);

destroy_return:
    return;
}

static int connection_event(void * data)
{
    int rv = -1;
    server_data_t * server_data = (server_data_t *)data;
    eloop_t * eloop = server_data->server->eloop;
    uint32_t packet_type = 0;
    size_t nread = tcp_read_all(server_data->client_fd, &packet_type, sizeof(packet_type));

    if (nread != sizeof(packet_type))
    {
        goto cleanup;
    }

    packet_type = ntohl(packet_type);

    if (packet_type < NUMBER_OF_TYPES)
    {
        event_t * event = generate_event(packet_type, server_data);

        if (!event)
        {
            goto cleanup;
        }

        eloop_add(eloop, event);
        rv = 0;
        goto event_return;
    }

cleanup:
    close(server_data->client_fd);
    server_data->client_fd = -1;
event_return:
    return rv;
}

static int connection_destroy(void * data)
{
    event_t * event = (event_t *)data;
    server_data_t * server_data = (server_data_t *)event->data;

    if (server_data->client_fd == -1)
    {
        server_data->server = NULL;
        free(server_data);
    }
    
    event->data = NULL;
    free(event);
    return 0;
}

static int join_event(void * data)
{
    int rv = -1;
    server_data_t * server_data = (server_data_t *)data;
    int client_fd = server_data->client_fd;
    circular_list_t * clients = server_data->server->clients;
    uint32_t username_sz = 0;

    int nread = tcp_read_all(client_fd, &username_sz, sizeof(username_sz));

    if (nread != sizeof(username_sz))
    {
        goto client_close;
    }

    username_sz = ntohl(username_sz);

    if (username_sz > MAX_USERNAME_SZ)
    {
        goto client_close;
    }

    char buffer[MAX_USERNAME_SZ + 1];
    memset(buffer, 0, MAX_USERNAME_SZ + 1);

    nread = tcp_read_all(client_fd, buffer, username_sz);

    if (nread != username_sz)
    {
        goto client_close;
    }

    client_t * client = client_create(buffer);

    if (!client)
    {
        goto client_close;
    }

    circular_insert(clients, client, FRONT);
    printf("%s has joined the chat\n", client->username);
    rv = 0;
    goto join_return;
    
client_close:
    close(client_fd);
join_return:
    return rv;
}

static int join_destroy(void * data)
{
    event_t * event = (event_t *)data;

    server_data_t * server_data = (server_data_t *)event->data;
    server_data->client_fd = -1;
    server_data->server = NULL;
    free(server_data);

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
        case JOIN:
            event->efunc = join_event;
            event->dfunc = join_destroy;
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