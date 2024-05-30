#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#define SERVER_PORT 	14333
#define DEBUG_PRINT 	1
#define MAX_CONNECTIONS 1

struct server{
	int server_socket;
	struct sockaddr_in socket_params;
	int desc_count;
	int client_desc;
};
void init_server_socket(struct server *main_server)
{
	main_server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	main_server->socket_params.sin_family = AF_INET;
	main_server->socket_params.sin_port = htons(SERVER_PORT);
	main_server->socket_params.sin_addr.s_addr = htonl(INADDR_ANY);
}
void try_to_bind_socket(struct server *main_server)
{
	int opt = 1;
	setsockopt(main_server->server_socket, SOL_SOCKET,
			SO_REUSEADDR, &opt, sizeof(opt));

	while(-1 == bind(main_server->server_socket,
				(struct sockaddr*)&(main_server->socket_params),
								sizeof(main_server->socket_params))){
			if(DEBUG_PRINT){printf("Trying to bind server socket\n");}
			sleep(2);
	}
	if(DEBUG_PRINT){printf("Successfully binded!\n");}
}
struct server *init_server()
{
	struct server *main_server = malloc(sizeof(struct server));

	init_server_socket(main_server);
	try_to_bind_socket(main_server);
	listen(main_server->server_socket, MAX_CONNECTIONS);
	if(DEBUG_PRINT){ printf("Server is up. Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
	return main_server;
}
void handle_command(char* command) {
    if (strcmp(command, "test_connection") == 0) {
        // Обработка команды test_connection
        printf("Received test_connection command\n");
    } else if (strcmp(command, "play") == 0) {
        // Обработка команды play
        printf("Received play command\n");
    } else if (strcmp(command, "restart") == 0) {
        // Обработка команды restart
        printf("Received restart command\n");
    } else if (strcmp(command, "stop") == 0) {
        // Обработка команды stop
        printf("Received stop command\n");
    } else {
        printf("Unknown command: %s\n", command);
    }
}

int main()
{
	struct server *ms = init_server();
	while(ms->desc_count < MAX_CONNECTIONS){
		if((ms->client_desc = accept(ms->server_socket, NULL, NULL)) != -1){
			printf("Accepted!\n");
			ms->desc_count ++;
		}
	}

    char buff[256];
    while(1){
        read(ms->client_desc, buff, sizeof(buff)-1);
        handle_command(buff);
        memset(buff, '1', sizeof(buff));
    }
    /*close*/
    close(ms->client_desc);
    ms->desc_count --;

}
