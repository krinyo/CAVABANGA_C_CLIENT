#include <stdio.h>
#include <netinet/in.h>     //sockets
#include <stdlib.h>         //malloc
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <dirent.h>
#include <signal.h>         //sigterm
#define BUFF_SIZE       1024
#define SERVER_PORT     14333
#define MAX_CONNECTIONS 1

/*DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT     1
#else
#define DEBUG_PRINT     0
#endif

static const char playlist_tmp_path[BUFF_SIZE] = "/tmp/playlist_files.txt";
static const char playlist_directory_path[BUFF_SIZE] = "playlists";
static const char current_playlist_filename[BUFF_SIZE] = "current_playlist.txt";
static const char delete_all_playlists_command[BUFF_SIZE] = "rm -r playlist/*";
size_t download_file(void *ptr, size_t size, size_t nmemb, FILE *stream);

static FILE *save_filenames(char url[BUFF_SIZE], const char filepath[BUFF_SIZE]){
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
void sync_playlist_directory(const char *playlist_dir, const char *playlist_file_url, const char *server_hostname) {
    char file_url[BUFF_SIZE], filename[BUFF_SIZE], file_path[BUFF_SIZE];
    FILE *playlist_filenames;
    size_t len;
    // Открываем файл, который содержит имена файлов с сервера
    playlist_filenames = fopen(playlist_file_url, "r");
    if (playlist_filenames) {
        DIR *dir;
        struct dirent *entry;
        char existing_files[BUFF_SIZE][BUFF_SIZE];
        int existing_count = 0;

        // Читаем все файлы из текущей директории плейлиста
        if ((dir = opendir(playlist_dir)) != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {  // Если это файл
                    snprintf(existing_files[existing_count], BUFF_SIZE, "%s", entry->d_name);
                    existing_count++;
                }
            }
            closedir(dir);
        }

        // Проходим по каждому файлу, полученному с сервера
        while (fgets(file_url, sizeof(file_url), playlist_filenames) != NULL) {
            // Обрезаем символ новой строки
            len = strlen(file_url);
            if (len > 0 && file_url[len - 1] == '\n') {
                file_url[len - 1] = '\0';
            }

            // Получаем имя файла из URL
            sprintf(filename, "%s", get_substring(file_url, 6, "/"));

            // Формируем путь для сохранения файла
            snprintf(file_path, sizeof(file_path), "%s/%s", playlist_dir, filename);

            // Если файл отсутствует в директории, скачиваем его
            int found = 0;
            for (int i = 0; i < existing_count; i++) {
                if (strcmp(existing_files[i], filename) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                printf("Файл %s отсутствует, скачиваем...\n", filename);
                char download_command[BUFF_SIZE];
                sprintf(download_command, "curl -o '%s' -O '%s'", file_path, file_url);
                system(download_command);
            } else {
                printf("Файл %s уже существует, пропускаем...\n", filename);
            }
        }

        // Удаляем файлы, которых больше нет на сервере
        for (int i = 0; i < existing_count; i++) {
            int found = 0;
            fseek(playlist_filenames, 0, SEEK_SET);  // Возвращаемся в начало файла
            while (fgets(file_url, sizeof(file_url), playlist_filenames) != NULL) {
                // Обрезаем символ новой строки
                len = strlen(file_url);
                if (len > 0 && file_url[len - 1] == '\n') {
                    file_url[len - 1] = '\0';
                }

                sprintf(filename, "%s", get_substring(file_url, 6, "/"));
                if (strcmp(existing_files[i], filename) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                snprintf(file_path, sizeof(file_path), "%s/%s", playlist_dir, existing_files[i]);
                printf("Файл %s отсутствует на сервере, удаляем...\n", existing_files[i]);
                remove(file_path);
            }
        }

        fclose(playlist_filenames);
    } else {
        printf("Ошибка открытия файла списка плейлиста.\n");
    }
}
void save_current_playlist(const char *playlist_name, const char *type) {
    FILE *file = fopen(current_playlist_filename, "w");
    if (file) {
        fprintf(file, "%s\n%s\n", playlist_name, type);
        fclose(file);
    } else {
        printf("Error opening current_playlist.txt for writing.\n");
    }
}

int load_current_playlist(char *playlist_name, char *type) {
    FILE *file = fopen(current_playlist_filename, "r");
    if (file) {
        if (fgets(playlist_name, BUFF_SIZE, file) != NULL && fgets(type, BUFF_SIZE, file) != NULL) {
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

// Struct to hold command data after parsing
struct command {
    char cmd[BUFF_SIZE];
    char playlist[BUFF_SIZE];
    char type[BUFF_SIZE];  // For future use, to handle different playlist types (e.g., video or pictures)
    char delay[BUFF_SIZE];
};
struct server{
	int server_socket;
	struct sockaddr_in socket_params;
	int client_desc;
};

/*adding handlers 1*/
char handlers[256][BUFF_SIZE] = {
                                        "play",/*play:playlist_name:video*//*playing and downloading given playlist by type*/
                                        "test",/*test:hostname*//*testing connection*/
                                        "stop",/*stop*//*stopping process by global var with pid*/
                                        //"sync",/*sync:playlist_name*/
                                        "restart",/*restart*//*stop process, generate command to start, start process*/
                                        //"clear_playlist",
                                        "rotate",/*rotate:right*/
                                        "get_playlist",/*get_playlist*/
                                        "delete_all_playlists"
};


size_t download_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}
/*adding handler function 2*/
void play_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    if(*pid > 0){
        printf("Process has already been started. PID IS:%i;\n", *pid);
        printf("Restarting process with PID: %d\n", *pid);
        kill(*pid, SIGTERM); // Stop the current process
        *pid = 0; // Reset the pid
        //return;
    }
    if (DEBUG_PRINT) {
        printf("Playing:%s; Python server hostname:%s; pid=%i; output:%s;type:%s;delay:%s;\n", sc.playlist, server_hostname, *pid, output, sc.type, sc.delay);
    }
    /*play:playlist_name:video*/
    char playlist_url[BUFF_SIZE], playlist_dir[BUFF_SIZE], cwd[BUFF_SIZE], mpv_command[BUFF_SIZE];
    FILE *fp;

    sprintf(playlist_url, "http://%s:8000/get_playlist_filenames/%s/", server_hostname, sc.playlist);
    if (DEBUG_PRINT) {
        printf("Fetching playlist from URL: %s\n", playlist_url);
    }

    fp = save_filenames(playlist_url, playlist_tmp_path);
    if(fp){
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current working dir: %s\n", cwd);
        }
        //mkdir(playlist_directory_path, 0777);
        snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/%s", cwd, playlist_directory_path, sc.playlist);
        mkdir(playlist_dir, 0777);

        sync_playlist_directory(playlist_dir, playlist_tmp_path, server_hostname);
        remove(playlist_tmp_path);

        // Воспроизводим плейлист в зависимости от типа (видео или изображение)
        if (strcmp(sc.type, "video") == 0) {
            
            snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
        } else if (strcmp(sc.type, "image") == 0) {
            if(sc.delay)
            {
                snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay '%s' '%s'", sc.delay, playlist_dir);
            }
            else{
                snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 4 '%s'", playlist_dir);
            }
            printf("MPV COMMAND IS:%s;\n", mpv_command);
        }
        
        // Форкаем и выполняем команду для воспроизведения
        
        *pid = fork();
        printf("\nFORKING PID %i\n", *pid);
        if (*pid == 0) {
            execl("/bin/bash", "bash", "-c", mpv_command, NULL);
            exit(0);
        }
        save_current_playlist(sc.playlist, sc.type);
    }

}
void test_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    if (DEBUG_PRINT) {
        printf("Testing:%s; Python server hostname:%s; pid=%i; output:%s;\n", sc.playlist, server_hostname, *pid, output);
    }
}
void stop_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    if (DEBUG_PRINT) {
        printf("Stopping:%s; Python server hostname:%s; pid=%i; output:%s;\n", sc.playlist, server_hostname, *pid, output);
    }
    if (*pid > 0) {
        printf("Stopping process with PID: %d\n", *pid);
        kill(*pid, SIGTERM); // Send termination signal to the process
        *pid = 0; // Reset pid after stopping
    } else {
        printf("No process running to stop.\n");
    }
}
void restart_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    if (DEBUG_PRINT) {
        printf("Restarting:%s; Python server hostname:%s; pid=%i; output:%s;\n", sc.playlist, server_hostname, *pid, output);
    }
    char playlist_dir[BUFF_SIZE], mpv_command[BUFF_SIZE], cwd[BUFF_SIZE], last_playlist[BUFF_SIZE], last_type[BUFF_SIZE] = {0};
    //struct command *last_cmd = malloc(sizeof(struct command));
    if (*pid > 0) {
        printf("Restarting process with PID: %d\n", *pid);
        kill(*pid, SIGTERM); // Stop the current process
        *pid = 0; // Reset the pid
    }
    // Reuse the logic from 'play' command to start a new process
    /*snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/%s/", getcwd(cwd, sizeof(cwd)), playlist_directory_path, sc.playlist);
    sync_playlist_directory(playlist_dir, playlist_tmp_path, server_hostname);
    if (strcmp(sc.type, "video") == 0) {
        snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
    } else if (strcmp(sc.type, "image") == 0) {
        snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 1.5 '%s'", playlist_dir);
        printf("Handle pictures playlist here.\n");
    }*/
        // Check if there's a previously played playlist
    if (load_current_playlist(last_playlist, last_type)) {
        printf("Found previous playlist: %s of type: %s. Resuming playback...\n", last_playlist, last_type);

        // Prepare the command struct and execute the 'play' command to restart the last playlist
        struct command last_cmd = {0};
        snprintf(last_cmd.cmd, sizeof(last_cmd.cmd), "play");
        snprintf(last_cmd.playlist, sizeof(last_cmd.playlist), "%s", last_playlist);
        snprintf(last_cmd.type, sizeof(last_cmd.type), "%s", last_type);
        snprintf(last_cmd.delay, sizeof(last_cmd.delay), "%s", sc.delay);

        printf("last_cmd:cmd:%s;playlist:%s;type:%s;delay%s", last_cmd.cmd, last_cmd.playlist, last_cmd.type, last_cmd.delay);
        play_handler(last_cmd, serv, server_hostname, pid, output);
        //handle_command(&last_cmd, server_hostname, &child_pid, output, ms);
    } else {
        printf("No previous playlist found.\n");
    }

    // Fork and execute the player again
    /*pid = fork();
    if (*pid == 0) {
        execl("/bin/bash", "bash", "-c", mpv_command, NULL);
        exit(0);
    }*/
    printf("Restarted process with new PID: %d\n", *pid);
}
void rotate_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    char rotate_command[BUFF_SIZE], cwd[BUFF_SIZE], playlist_dir[BUFF_SIZE], mpv_command[BUFF_SIZE] = {0};
    snprintf(rotate_command, sizeof(rotate_command), "xrandr --output %s --rotate %s", output, sc.playlist);
    printf("Rotate command is:%s;\n", rotate_command);
    system(rotate_command);

    load_current_playlist(sc.playlist, sc.type);
    if (*pid > 0) {
        printf("Restarting process with PID: %d\n", *pid);
        kill(*pid, SIGTERM); // Stop the current process
        *pid = 0; // Reset the pid
    }
    snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/%s/", getcwd(cwd, sizeof(cwd)), playlist_directory_path, sc.playlist);
    if (strcmp(sc.type, "video") == 0) {
        snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
    } else if (strcmp(sc.type, "image") == 0) {
        if(sc.delay)
        {
            snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay '%s' '%s'", sc.delay, playlist_dir);
        }
        else{
            snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 4 '%s'", playlist_dir);
        }
        printf("Handle pictures playlist here. MPV COMMAND IS:%s;\n", mpv_command);
    }
    
        // Fork and execute the player again
    *pid = fork();
    if (*pid == 0) {
        execl("/bin/bash", "bash", "-c", mpv_command, NULL);
        exit(0);
    }
    printf("Restarted process with new PID: %d\n", *pid);
    
}
void get_playlist_handler(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    //get_playlist
    char playlist_name[BUFF_SIZE]={0};
    FILE *file = fopen(current_playlist_filename, "r");
    if (file) {
        printf("getting playlist...");
        fgets(playlist_name, sizeof(playlist_name), file);
        //write(main_server->client_desc, playlist_name, sizeof(playlist_name));
        send(serv.client_desc, playlist_name, strlen(playlist_name), 0);
        fclose(file);
    } else {
        printf("Error opening current_playlist.txt for reading.\n");
    }
}
void delete_all_playlists(struct command sc, struct server serv,
                    char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE]){
    ///
    system(delete_all_playlists_command);
}
/*adding function pointer 3*/
void (*handler_ptrs[])(struct command, struct server, char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE])={
    play_handler,
    test_handler,
    stop_handler,
    restart_handler,
    rotate_handler,
    get_playlist_handler,
    delete_all_playlists
};


/*server part*/
void init_server_socket(struct server *main_server){
	main_server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	main_server->socket_params.sin_family = AF_INET;
	main_server->socket_params.sin_port = htons(SERVER_PORT);
	main_server->socket_params.sin_addr.s_addr = htonl(INADDR_ANY);
}
void try_to_bind_socket(struct server *main_server){
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
struct server *init_server(){
	struct server *main_server = malloc(sizeof(struct server));

	init_server_socket(main_server);
	try_to_bind_socket(main_server);
	listen(main_server->server_socket, MAX_CONNECTIONS);
	
    if(DEBUG_PRINT){ printf("Server is up. Listening on: %d:%d\n", INADDR_ANY, SERVER_PORT); }
	
    return main_server;
}
/*server part ends */


static void parse_command(char c[BUFF_SIZE], struct command *sc) {
    // Tokenize the string using ':' as the delimiter
    char *token = strtok(c, ":");
    
    // First token is the command (e.g., "playlist")
    if (token != NULL) {
        strncpy(sc->cmd, token, BUFF_SIZE - 1);
        sc->cmd[BUFF_SIZE - 1] = '\0'; // Ensure null-termination
    }
    
    // Second token is the playlist (e.g., "test")
    token = strtok(NULL, ":");
    if (token != NULL) {
        strncpy(sc->playlist, token, BUFF_SIZE - 1);
        sc->playlist[BUFF_SIZE - 1] = '\0'; // Ensure null-termination
    }
    
    // Third token is the type (e.g., "video")
    token = strtok(NULL, ":");
    if (token != NULL) {
        strncpy(sc->type, token, BUFF_SIZE - 1);
        sc->type[BUFF_SIZE - 1] = '\0'; // Ensure null-termination
    }

    // Third token is the type (e.g., "1.5")
    token = strtok(NULL, ":");
    if (token != NULL) {
        strncpy(sc->delay, token, BUFF_SIZE - 1);
        sc->delay[BUFF_SIZE - 1] = '\0'; // Ensure null-termination
    }
}
void handle_command(char c[BUFF_SIZE], struct command *sc, char server_hostname[BUFF_SIZE], pid_t *pid, char output[BUFF_SIZE], struct server ms){
    parse_command(c, sc);
    
    printf("Cmd:%s;Playlist:%s;Type:%s;Delay:%s;\n", sc->cmd, sc->playlist, sc->type, sc->delay);

    int found = 0;
    int i;
    for(i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++){
        if(strcmp(sc->cmd, handlers[i]) == 0){
            handler_ptrs[i](*sc, ms, server_hostname, pid, output);
            found = 1;
            break;
        }
    }
    if (!found) {
        if (DEBUG_PRINT) {
            printf("Unknown command: %s\n", sc->cmd);
        }
    }
}
int main(int agrc, char **args){
    char comm_buff[BUFF_SIZE];
    struct command *parsed_command = malloc(sizeof(struct command));
    struct server *srv = init_server();
    pid_t main_process_pid = 0;
    char server_hostname[BUFF_SIZE] = {0};
    char output_name[BUFF_SIZE] = {0};
    char last_playlist[BUFF_SIZE] = {0};
    char last_type[BUFF_SIZE] = {0};
    snprintf(server_hostname, sizeof(server_hostname), args[1]);
    snprintf(output_name, sizeof(output_name), args[2]);
    
    if(DEBUG_PRINT){
        printf("Checking and creating playlist directory:\n");
    }
    if(!opendir(playlist_directory_path)){
        mkdir(playlist_directory_path, 0777);
    }
    sleep(10);
    // Check if there's a previously played playlist
    if (load_current_playlist(last_playlist, last_type)) {
        printf("Found previous playlist: %s of type: %s. Resuming playback...\n", last_playlist, last_type);

        // Prepare the command struct and execute the 'play' command to restart the last playlist
        struct command last_cmd = {0};
        char mpv_command[BUFF_SIZE] = {0};
        char playlist_dir[BUFF_SIZE] = {0};
        char cwd[BUFF_SIZE] = {0};
        getcwd(cwd, sizeof(cwd));
        snprintf(last_cmd.cmd, sizeof(last_cmd.cmd), "play");
        snprintf(last_cmd.playlist, sizeof(last_cmd.playlist), "%s", last_playlist);
        snprintf(last_cmd.type, sizeof(last_cmd.type), "%s", last_type);

        //play_handler(last_cmd, *srv, server_hostname, &main_process_pid, output_name);
        snprintf(playlist_dir, sizeof(playlist_dir), "%s/%s/%s", cwd, playlist_directory_path, last_cmd.playlist);
        if (strcmp(last_cmd.type, "video") == 0) {
            snprintf(mpv_command, sizeof(mpv_command), "mpv --fs --playlist=<(find '%s' -type f -name '*') --loop-playlist", playlist_dir);
        } else if (strcmp(last_cmd.type, "image") == 0) {
            if(last_cmd.delay)
            {
                snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay '%s' '%s'", last_cmd.delay, playlist_dir);
            }
            else{
                snprintf(mpv_command, sizeof(mpv_command), "feh --fullscreen --slideshow-delay 4 '%s'", playlist_dir);
            }
            printf("MPV COMMAND IS:%s;\n", mpv_command);
        }
        main_process_pid = fork();
        printf("\nFORKING PID %i\n", main_process_pid);
        if (main_process_pid == 0) {
            execl("/bin/bash", "bash", "-c", mpv_command, NULL);
            exit(0);
        }
        //handle_command(&last_cmd, server_hostname, &child_pid, output, ms);
    } else {
        printf("No previous playlist found.\n");
    }
    
    while(1){
        if((srv->client_desc = accept(srv->server_socket, NULL, NULL)) != -1){
            if (DEBUG_PRINT) {
                printf("Accepted connection\n");
            }
            memset(comm_buff, 0, BUFF_SIZE);
            memset(parsed_command, 0, BUFF_SIZE);
            
            read(srv->client_desc, comm_buff, sizeof(comm_buff)-1);
            if (DEBUG_PRINT) {
                printf("Command:%s\n", comm_buff);
            }
            handle_command(comm_buff, parsed_command, server_hostname, &main_process_pid, output_name, *srv);

            fflush(stdout);
            close(srv->client_desc);
        }
    }
}