#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <curl/curl.h>

#define SERVER_PORT 	14333
#define DEBUG_PRINT 	1
#define MAX_CONNECTIONS 1

// Function to download a single file using curl
size_t download_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

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

pid_t child_pid;

/*void handle_command(char* command) {
    char cmd[1024];
    char playlist[1024];
    char url[1024];
    if (strcmp(command, "test_connection") == 0) {
        printf("Received test_connection command\n");
    } 
	else if (strcmp(command, "play") == 0) {

        
        sscanf(command, "%[^:]:%s", cmd, playlist);
        
		child_pid = fork();
		if (child_pid == 0) {
			execl("/bin/sh", "sh", "-c", "mpv --fs --loop --playlist=<(find '/home/krinyo/playlist' -type f -name '*')", NULL);
			exit(0);
		}
        printf("Received play command\n");
    } 
	else if (strcmp(command, "restart") == 0) {
		if (child_pid > 0) {
			kill(child_pid, SIGTERM);
			child_pid = fork();
			if (child_pid == 0) {
				execl("/bin/sh", "sh", "-c", "mpv --fs --loop --playlist=<(find '/home/krinyo/playlist' -type f -name '*')", NULL);
				exit(0);
			}
		}
        printf("Received restart command\n");
    } 
	else if (strcmp(command, "stop") == 0) {
		if (child_pid > 0) {
			kill(child_pid, SIGTERM);
			child_pid = 0;
		}
        printf("Received stop command\n");
    } 
	else {
        sscanf(command, "%[^:]:%s", cmd, playlist);
        printf("I'm here %s cmd , %s playlist\n", cmd, playlist);
        //here we download files
        
        // Build the URL to retrieve playlist files
        //sprintf(url, "http://10.1.9.52:8000/get_playlist_files/%s", playlist);
        sprintf(url, "http://10.1.9.52:8000/get_playlist_files/playlist_/", playlist);

        CURL *curl;
        CURLcode res;
        FILE *fp;

        curl = curl_easy_init();
        if (curl) {
            //curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            
            
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);  // Skip SSL verification for self-signed certs (if needed)
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_file);  // Set callback for downloaded data
            
            printf("HELLO url %s \n", url);
            // Open a temporary file to store the playlist response
            fp = fopen("/tmp/playlist_files.txt", "wb");
            if (fp) {
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);  // Set file pointer for writing downloaded data

                // Send the request and handle errors
                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }

                fclose(fp);  // Close the temporary file
            } else {
                fprintf(stderr, "Error opening temporary file\n");
            }


        }

        // Now process the downloaded playlist file (text containing URLs)
        if (fp) {
        FILE *playlist_file = fopen("/tmp/playlist_files.txt", "r");
        if (playlist_file) {
            char file_url[1024];

            // Read each file URL from the playlist file
            while (fgets(file_url, sizeof(file_url), playlist_file) != NULL) {
                // Remove trailing newline from the URL
                strtok(file_url, "\n");
                // Download the file using curl
                sprintf(url, "%s", file_url);  // Update URL for each file
                curl_easy_setopt(curl, CURLOPT_URL, url);  // Set the URL for curl
                res = curl_easy_perform(curl);  // Reuse the initialized curl handle
                if (res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                    fprintf(stderr, "Failed to download file: %s\n", file_url);
                }
            }

            fclose(playlist_file);
            } else {
                fprintf(stderr, "Error opening playlist file\n");
            }
        }
        curl_easy_cleanup(curl);
        // Clean up temporary file (optional)
        remove("/tmp/playlist_files.txt");

        printf("Received start_playlist command\n");

            
        printf("Received start playlist command\n");
        //printf("Unknown command: %s\n", command);
    }
}
*/

void handle_command(char* command) {
    char cmd[1024];
    char playlist[1024];
    char url[1024];
    char file_path[1024];
    char filename[1024];
    if (strcmp(command, "test_connection") == 0) {
        printf("Received test_connection command\n");
    } 
    else if (strcmp(command, "play") == 0) {
        sscanf(command, "%[^:]:%s", cmd, playlist);
        child_pid = fork();
        if (child_pid == 0) {
            execl("/bin/sh", "sh", "-c", "mpv --fs --loop --playlist=<(find '/home/krinyo/playlist' -type f -name '*')", NULL);
            exit(0);
        }
        printf("Received play command\n");
    } 
    else if (strcmp(command, "restart") == 0) {
        if (child_pid > 0) {
            kill(child_pid, SIGTERM);
            child_pid = fork();
            if (child_pid == 0) {
                execl("/bin/sh", "sh", "-c", "mpv --fs --loop --playlist=<(find '/home/krinyo/playlist' -type f -name '*')", NULL);
                exit(0);
            }
        }
        printf("Received restart command\n");
    } 
    else if (strcmp(command, "stop") == 0) {
        if (child_pid > 0) {
            kill(child_pid, SIGTERM);
            child_pid = 0;
        }
        printf("Received stop command\n");
    } 
    else {
        sscanf(command, "%[^:]:%s", cmd, playlist);
        printf("I'm here %s cmd , %s playlist\n", cmd, playlist);
        
        sprintf(url, "http://10.1.9.52:8000/get_playlist_files/playlist_/", playlist);

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

        if (fp) {
            FILE *playlist_file = fopen("/tmp/playlist_files.txt", "r");
            if (playlist_file) {
                char file_url[1024];

                while (fgets(file_url, sizeof(file_url), playlist_file) != NULL) {
                    strtok(file_url, "\n");
                    // Extract filename from URL and replace all '/' and ':' with '_'
                    sscanf(file_url, "%*[^/]/%s", filename);
                    for (int i = 0; filename[i]; i++) {
                        if (filename[i] == '/' || filename[i] == ':') {
                            filename[i] = '_';
                        }
                    }
                    sprintf(file_path, "%s", filename);  // Create file path for each file

                    sprintf(file_path, "%s", filename);  // Create file path for each file
                    fp = fopen(file_path, "wb");  // Open a new file for each download
                    if (fp) {
                        curl_easy_setopt(curl, CURLOPT_URL, file_url);  
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);  

                        res = curl_easy_perform(curl);
                        if (res != CURLE_OK) {
                            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                            fprintf(stderr, "Failed to download file: %s\n", file_url);
                        }

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

            
        printf("Received start playlist command\n");
        //printf("Unknown command: %s\n", command);
    }
}

int main() {
    struct server *ms = init_server();
    char buff[256];

    while(1) { // Бесконечный цикл для принятия соединений
        if((ms->client_desc = accept(ms->server_socket, NULL, NULL)) != -1){
            printf("Accepted!\n");

			read(ms->client_desc, buff, sizeof(buff)-1);
			printf("%s\n", buff);
			//char* trimmed_buffer = strtok(buff, "\0 ");
			handle_command(buff);
			//memset(buff, '0', sizeof(buff));

            /*close*/
            close(ms->client_desc);
            printf("Connection closed\n");
        }
    }
}

/*
#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
    CURL *curl;
    FILE *fp;
    int result;
    char *url = "http://localhost:8000/download_playlist/playlist1/";
    char outfilename[FILENAME_MAX] = "downloaded_playlist.zip";
    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        result = curl_easy_perform(curl);
        //always cleanup 
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 0;
}

*/