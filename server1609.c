#include <stdio.h>
#include <netinet/in.h>     //sockets
#include <stdlib.h>         //malloc
#include <unistd.h>         //sleep
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>       //mkdir
#include <signal.h>
#include <dirent.h>  // For directory handling
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

// Function to download a single file using curl
size_t download_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

FILE *save_filenames(char url[BUF_SIZE], char filepath[BUF_SIZE]){
    /*SAVING FILES STRINGS TO FILE*/
    CURL *curl;
    CURLcode res;
    FILE *fp;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);  
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_file);  

        fp = fopen(filepath, "wb");
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
    return fp;
    /**/
}
char* get_substring(const char* str, int substring_number, const char* delimiter) {
    static char result[256]; // Используем static для возвращения локальной строки
    char str_copy[256]; // Копия строки, так как strtok модифицирует строку
    strncpy(str_copy, str, sizeof(str_copy));
    str_copy[sizeof(str_copy) - 1] = '\0'; // Обеспечиваем нуль-терминированность

    char* token;
    int count = 0;

    // Получаем первую подстроку
    token = strtok(str_copy, delimiter);
    // Перебираем остальные подстроки
    while (token != NULL) {
        count++;
        if (count == substring_number) {
            strncpy(result, token, sizeof(result));
            result[sizeof(result) - 1] = '\0'; // Обеспечиваем нуль-терминированность
            return result;
        }
        token = strtok(NULL, delimiter);
    }
    return NULL;
}

// Struct to hold command data after parsing
struct command {
    char cmd[BUF_SIZE];
    char playlist[BUF_SIZE];
    char type[BUF_SIZE];  // For future use, to handle different playlist types (e.g., video or pictures)
};

void parse_command(char *message, struct command *parsed_cmd) {
    // Split the input message into command and playlist
    sscanf(message, "%[^:]:%[^:]:%s", parsed_cmd->cmd, parsed_cmd->playlist, parsed_cmd->type);

    // Print parsed values for debugging
    if (DEBUG_PRINT) {
        printf("Parsed Command: %s, Playlist: %s, Type: %s\n", parsed_cmd->cmd, parsed_cmd->playlist, parsed_cmd->type);
    }
}

// Function to remove a directory and its contents
int remove_directory(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    char full_path[BUF_SIZE];
    struct stat statbuf;

    if (!dir) {
        perror("Error opening directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                remove_directory(full_path);  // Recursively remove subdirectories
            } else {
                remove(full_path);  // Remove files
            }
        }
    }

    closedir(dir);
    rmdir(path);  // Remove the empty directory

    return 0;
}

// Clear all playlists by removing the entire playlists directory
void clear_all_playlists() {
    char playlists_dir[BUF_SIZE];
    snprintf(playlists_dir, sizeof(playlists_dir), "%s/playlists", getcwd(NULL, 0));
    
    printf("Clearing all playlists in directory: %s\n", playlists_dir);

    // Remove playlists directory and its contents
    remove_directory(playlists_dir);
    
    // Optionally recreate the directory if needed
    mkdir(playlists_dir, 0777);
}

// Clear one specific playlist
void clear_playlist(const char *playlist_name) {
    char playlist_dir[BUF_SIZE];
    snprintf(playlist_dir, sizeof(playlist_dir), "%s/playlists/%s", getcwd(NULL, 0), playlist_name);

    printf("Clearing playlist: %s\n", playlist_dir);

    // Remove the specific playlist directory
    remove_directory(playlist_dir);
}

void save_current_playlist(const char *playlist_name, const char *type) {
    FILE *file = fopen("current_playlist.txt", "w");
    if (file) {
        fprintf(file, "%s\n%s\n", playlist_name, type);
        fclose(file);
    } else {
        printf("Error opening current_playlist.txt for writing.\n");
    }
}

int load_current_playlist(char *playlist_name, char *type) {
    FILE *file = fopen("current_playlist.txt", "r");
    if (file) {
        if (fgets(playlist_name, BUF_SIZE, file) != NULL && fgets(type, BUF_SIZE, file) != NULL) {
            // Remove newline characters from playlist_name and type
            playlist_name[strcspn(playlist_name, "\n")] = 0;
            type[strcspn(type, "\n")] = 0;
            fclose(file);
            return 1;  // Success
        }
        fclose(file);
    }
    return 0;  // No playlist to load
}

void execute_command(struct command *cmd_data, char server_hostname[BUF_SIZE], pid_t *pid) {
    char url[BUF_SIZE], file_path[BUF_SIZE], filename[BUF_SIZE];
    char cwd[BUF_SIZE], playlist_dir[BUF_SIZE], file_url[BUF_SIZE];
    char download_command[BUF_SIZE], mpv_command[BUF_SIZE], feh_command[BUF_SIZE];
    FILE *fp;
    
    // Handle the 'play' command
    if (strcmp(cmd_data->cmd, "play") == 0) {
        sprintf(url, "http://%s:8000/get_playlist_filenames/%s/", server_hostname, cmd_data->playlist);
        if (DEBUG_PRINT) {
            printf("Fetching playlist from URL: %s\n", url);
        }

        // Download playlist files
        fp = save_filenames(url, "/tmp/playlist_files.txt");
        if (fp) {
            FILE *playlist_filenames = fopen("/tmp/playlist_files.txt", "r");
            if (playlist_filenames) {
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("Current working dir: %s\n", cwd);
                }
                mkdir("playlists", 0777);
                snprintf(playlist_dir, sizeof(playlist_dir), "%s/playlists/%s", cwd, cmd_data->playlist);
                mkdir(playlist_dir, 0777);

                while (fgets(file_url, sizeof(file_url), playlist_filenames) != NULL) {
                        // Убираем перенос строки в конце, если он есть
                    size_t len = strlen(file_url);
                    if (len > 0 && file_url[len - 1] == '\n') {
                        file_url[len - 1] = '\0';
                    }

                    // Чистим URL файла и получаем имя файла
                    sprintf(filename, "%s", get_substring(file_url, 6, "/"));
                    int i = 0;
                    while (filename[i] != '\0') {
                        if (filename[i] == '\n') {
                            filename[i] = '\0';
                        }
                        i++;
                    }
                    printf("File url:%s", file_url);
                    printf("Filename:%s", filename);
                    // Формирование пути для сохранения файла
                    sprintf(file_path, "%s/%s", playlist_dir, filename);
                    printf("\nDownloading file to path: %s\n", file_path);

                    // Формирование команды для загрузки файла
                    sprintf(download_command, "curl -o '%s' -O '%s'", file_path, file_url);
                    printf("\nExecuting download command: %s\n", download_command);

                    // Выполнение команды
                    system(download_command);
                }


                fclose(playlist_filenames);
            }
            remove("/tmp/playlist_files.txt");

            // Depending on the playlist type, we handle video or pictures
            if (strcmp(cmd_data->type, "video") == 0) {
                snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
            } else if (strcmp(cmd_data->type, "image") == 0) {
                printf("PLAYLIST DIR BEFORE FEHING:%s;\n", playlist_dir);
                snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 4 '%s'", playlist_dir);
		        // Placeholder for handling pictures (could be something like feh for a slideshow)
                printf("Handle pictures playlist here. one\n");
            }

            // Fork and execute the player
            *pid = fork();
            if (*pid == 0) {
                execl("/bin/bash", "bash", "-c", mpv_command, NULL);
                exit(0);
            }

            save_current_playlist(cmd_data->playlist, cmd_data->type);
        }
    }
    // Stop the current process
    else if (strcmp(cmd_data->cmd, "stop") == 0) {
        if (*pid > 0) {
            printf("Stopping process with PID: %d\n", *pid);
            kill(*pid, SIGTERM); // Send termination signal to the process
            *pid = 0; // Reset pid after stopping
        } else {
            printf("No process running to stop.\n");
        }
    }
    // Stop the current process
    else if (strcmp(cmd_data->cmd, "stop_all_video") == 0) {
        if (*pid > 0) {
            system("killall mpv");
        } else {
            printf("No process running to stop.\n");
        }
    }
    // Restart the current process
    else if (strcmp(cmd_data->cmd, "restart") == 0) {
        if (*pid > 0) {
            printf("Restarting process with PID: %d\n", *pid);
            kill(*pid, SIGTERM); // Stop the current process
            *pid = 0; // Reset the pid
        }

        // Reuse the logic from 'play' command to start a new process
        snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/", getcwd(cwd, sizeof(cwd)), cmd_data->playlist);

        if (strcmp(cmd_data->type, "video") == 0) {
            snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
        } else if (strcmp(cmd_data->type, "image") == 0) {
	        snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 1.5 --cycle-once '%s'", playlist_dir);
            printf("Handle pictures playlist here.\n");
        }

        // Fork and execute the player again
        *pid = fork();
        if (*pid == 0) {
            execl("/bin/bash", "bash", "-c", mpv_command, NULL);
            exit(0);
        }
        printf("Restarted process with new PID: %d\n", *pid);
    }

    else if (strcmp(cmd_data->cmd, "clear_all") == 0) {
        clear_all_playlists();
        printf("All playlists cleared.\n");
    }
    // Clear a specific playlist
    else if (strcmp(cmd_data->cmd, "clear_playlist") == 0) {
        if (cmd_data->playlist[0] != '\0') {  // Ensure the playlist name is provided
            clear_playlist(cmd_data->playlist);
            printf("Playlist '%s' cleared.\n", cmd_data->playlist);
        } else {
            printf("No playlist name provided for clear_playlist command.\n");
        }
    }
}


void handle_command(char *message, char server_hostname[BUF_SIZE], pid_t *pid) {
    struct command cmd_data = {0};
    parse_command(message, &cmd_data);
    execute_command(&cmd_data, server_hostname, pid);
}


int main(int argc, char **args) {
    char server_hostname[BUF_SIZE];
    pid_t child_pid = 0;
    char last_playlist[BUF_SIZE] = {0};
    char last_type[BUF_SIZE] = {0};

    snprintf(server_hostname, sizeof(server_hostname), "%s", args[1]);
    if (DEBUG_PRINT) {
        printf("Python Server hostname is: %s;\n", server_hostname);
    }

    struct server *ms = init_server();

    // Check if there's a previously played playlist
    if (load_current_playlist(last_playlist, last_type)) {
        printf("Found previous playlist: %s of type: %s. Resuming playback...\n", last_playlist, last_type);

        // Prepare the command struct and execute the 'play' command to restart the last playlist
        struct command last_cmd = {0};
        snprintf(last_cmd.cmd, sizeof(last_cmd.cmd), "play");
        snprintf(last_cmd.playlist, sizeof(last_cmd.playlist), "%s", last_playlist);
        snprintf(last_cmd.type, sizeof(last_cmd.type), "%s", last_type);

        execute_command(&last_cmd, server_hostname, &child_pid);
    } else {
        printf("No previous playlist found.\n");
    }

    char buff[BUF_SIZE];
    while (1) { // Infinite loop for accepting connections
        if ((ms->client_desc = accept(ms->server_socket, NULL, NULL)) != -1) {
            printf("Accepted!\n");

            read(ms->client_desc, buff, sizeof(buff) - 1);
            printf("%s\n", buff);

            handle_command(buff, server_hostname, &child_pid);
            fflush(stdout);
            close(ms->client_desc);
            printf("Connection closed\n");
        }
    }
}

