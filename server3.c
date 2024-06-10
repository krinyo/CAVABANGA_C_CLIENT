#include <stdio.h>
#include <netinet/in.h>     //sockets
#include <stdlib.h>         //malloc
#include <unistd.h>         //sleep
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>       //mkdir
#include <signal.h>

#define SERVER_PORT     14333
#define BUF_SIZE        1024

/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif


#define MAX_CONNECTIONS 1

struct server{
	int server_socket;
	struct sockaddr_in socket_params;
	int desc_count;                 //not actually need
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
/*void handle_command(char *message)
{
    char cmd[BUF_SIZE]; //first part of the command
    char playlist[BUF_SIZE]; //playlist name
    char url[BUF_SIZE]; //url for curl
    char file_path[BUF_SIZE];   //file path for saving files
    char filename[BUF_SIZE];
    char playlist_dir[BUF_SIZE];

    sscanf(message, "%[^: ]:%*[ ]%s", cmd, playlist);

    if(DEBUG_PRINT){printf("CMD:%s; and PLAYLIST:%s;", cmd, playlist);}

    if(strcmp(cmd, "test") == 0){
        if(DEBUG_PRINT){printf("TEST COMMAND;PLAYLIST:%s;", playlist);}


    }
    else if(strcmp(cmd, "play") == 0){
        if(DEBUG_PRINT){printf("PLAY COMMAND;PLAYLIST:%s;", playlist);}

    }

    memset(cmd, '0', BUF_SIZE);
    memset(playlist, '0', BUF_SIZE);

}*/

// Function to download a single file using curl
size_t download_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

void handle_command(char *message, char server_hostname[BUF_SIZE], pid_t *pid) {
    char cmd[BUF_SIZE] = {0}; // Initialize with null terminator
    char playlist[BUF_SIZE] = {0};
    char url[BUF_SIZE]; //url for curl
    char file_path[BUF_SIZE];   //file path for saving files
    char filename[BUF_SIZE];
    char playlist_dir[BUF_SIZE];
    char get_playlist_files[BUF_SIZE];
    char cwd[BUF_SIZE];
    char file_url[BUF_SIZE];
    char mpv_command[BUF_SIZE];
    char download_command[BUF_SIZE];
    sprintf(get_playlist_files, "http://%s:8000/get_playlist_files/", server_hostname);
    // Consider using a custom parsing function or tokenizing with a delimiter
    int num_tokens = sscanf(message, "%[^:]:%s", cmd, playlist);

    if (DEBUG_PRINT) {
    printf("CMD:%s; and PLAYLIST:%s;\n", cmd, playlist);
    }

    if (num_tokens == 2 && strcmp(cmd, "test") == 0) {
        if (DEBUG_PRINT) {
            printf("TEST COMMAND;PLAYLIST:%s;\n", playlist);
        }
    } else if (num_tokens == 2 && strcmp(cmd, "play") == 0) {
        if (DEBUG_PRINT) {
            printf("PLAY COMMAND;PLAYLIST:%s;\n", playlist);
        }

        sprintf(url, "%s%s/", get_playlist_files, playlist);
        if (DEBUG_PRINT) {
            printf("URLIS:%s;\n", url);
        }

        /*SAVING FILES STRINGS TO FILE*/
        CURL *curl;
        CURLcode res;
        FILE *fp;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);  
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_file);  

            fp = fopen("/tmp/playlist_files.txt", "wb");
            if (fp) {
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);  
                printf("Formatted URL: %s\n", url);
                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }

                fclose(fp);  
            } else {
                fprintf(stderr, "Error opening temporary file\n");
            }

            curl_easy_cleanup(curl);
        }
        /**/
        if (fp) {
            FILE *playlist_file = fopen("/tmp/playlist_files.txt", "r");
            if (playlist_file) {
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("Current working dir: %s\n", cwd);
                }
                snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/", cwd, playlist);
                printf("Playlist dir: %s\n", playlist_dir);
                mkdir(playlist_dir, 0777);

                while (fgets(file_url, sizeof(file_url), playlist_file) != NULL) {
                    strtok(file_url, "\n");
                    // Extract filename from URL and replace all '/' and ':' with '_'
                    sscanf(file_url, "%*[^/]/%s", filename);
                    for (int i = 0; filename[i]; i++) {
                        if (filename[i] == '/' || filename[i] == ':') {
                            filename[i] = '_';
                        }
                    }
                    
                    sprintf(file_path, "%s%s", playlist_dir, filename);  // Create file path for each file
                    printf("FILE PATH TO SAVE:%s;\n", file_path);
                    fp = fopen(file_path, "wb");  // Open a new file for each download
                    if (fp) {
                        printf("fileURLtoDOWNLOAD:%s;\n", file_url);
                        printf("DOWNLOAD COMMAND:%s;\n", download_command);
                        sprintf(download_command, "curl -o '%s' -O '%s'", file_path,file_url);
                        system(download_command);
                        /*curl_easy_setopt(curl, CURLOPT_URL, file_url);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);  

                        res = curl_easy_perform(curl);
                        if (res != CURLE_OK) {
                            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                            fprintf(stderr, "Failed to download file: %s\n", file_url);
                        }*/

                        fclose(fp);  
                    } else {
                        fprintf(stderr, "Error opening file: %s\n", file_path);
                    }
                }

                fclose(playlist_file);
            } else {
                fprintf(stderr, "Error opening playlist file\n");
            }
        }
        remove("/tmp/playlist_files.txt");
        printf("Received start_playlist command\n");
        printf("PLAYLIST DIR:%s;", playlist_dir);
        snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
        printf("COMMAND:%s;", mpv_command);
        *pid = fork();
        if (*pid == 0) {
            execl("/bin/bash", "bash", "-c", mpv_command, NULL);
            exit(0);
        }

    }
    else if (num_tokens == 2 && strcmp(cmd, "stop") == 0) {
        if (DEBUG_PRINT) {
            printf("STOP COMMAND;PLAYLIST:%s;\n", playlist);
        }
        if (*pid > 0) {
            kill(*pid, SIGTERM);
            *pid = 0;
        }
        printf("Received stop command\n");
    }
    else if (num_tokens == 2 && strcmp(cmd, "restart") == 0) {
        if (DEBUG_PRINT) {
            printf("RESTART COMMAND;PLAYLIST:%s;\n", playlist);
        }
    }
    else {
        // Handle invalid messages or unexpected commands (optional)
        if (DEBUG_PRINT) {
            printf("Invalid command or missing arguments.\n");
        }
    }
}


int main(int argc, char **args)
{
    char server_hostname[BUF_SIZE];
    pid_t child_pid = 0;

    sprintf(server_hostname, args[1], sizeof(server_hostname));
    if(DEBUG_PRINT){printf("Server hostname is:%s;\n", server_hostname);}

    struct server *ms = init_server();
    char buff[BUF_SIZE];

    while(1) { // Бесконечный цикл для принятия соединений
        if((ms->client_desc = accept(ms->server_socket, NULL, NULL)) != -1){
            printf("Accepted!\n");

			read(ms->client_desc, buff, sizeof(buff)-1);
			printf("%s\n", buff);

			handle_command(buff, server_hostname, &child_pid);
            fflush(stdout);
            close(ms->client_desc);
            printf("Connection closed\n");
        }
    }
}
