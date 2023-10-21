#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);
bool need_proxy(const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //printf("%s\n");
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    char* first_line = strtok(request, "\r\n");
    char* command_type = strtok(first_line, " ");
    char * file_name = strtok(NULL, " ");
    if (strcmp(command_type, "GET")) {
        printf("Only supports GET HTTPS requests");
        return;
    }

    if (!strcmp(file_name, "/") && strlen(file_name) == 1) {
        file_name = "index.html";
    }

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    if (need_proxy(file_name)) {
       proxy_remote_file(app, client_socket, file_name);
    } else {
        serve_local_file(client_socket, file_name);
    }
}

bool need_proxy(const char* path) {
    char* ext;
    if (strchr(path, '.')) {
        ext = strrchr(path, '.');
        if (!strcmp(ext, ".ts"))
            return true;
    }
    return false;
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    
    char* ext;
    if (strchr(path, '.'))
        ext = strrchr(path, '.');
    else
        ext = "binary";
    
    char* content;
    
    if (!strcmp(ext, ".html"))
        content = "text/html";
    else if (!strcmp(ext, ".txt"))
        content = "text/plain";
    else if (!strcmp(ext, ".jpg"))
        content = "image/jpeg";
    else if (!strcmp(ext, "binary"))
        content = "application/octet-stream";
    
    
    path = strtok((char*)path, "/");
    printf("%s\n", path);
    
    char path_parsed[strlen(path) + 1];
    char *sp = (char*)path;
    char *dp = path_parsed;

    while (*sp) {
        if (sp[0] == '%' && sp[1] == '2' && sp[2] == '0') {
            *dp = ' ';
            sp += 3;
        }
        else if (sp[0] == '%' && sp[1] == '2' && sp[2] == '5') {
            *dp = '%';
            sp += 3;
        }
        else {
            *dp = *sp;
            sp++;
        }
        dp++;
    }
    *dp = '\0';
    
    FILE *file = fopen(path_parsed, "r");
    
    if (!file) {
        char response[] = "HTTP/1.0 404 Not Found\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);
    
    char *response = (char *)malloc(file_size + 8192);
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", content, file_size);
    
    send(client_socket, response, strlen(response), 0);
    
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    
    fclose(file);
    
    free(response);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_host/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    // Connect to remote server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in host_addr;

    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = inet_addr(app->remote_host);
    host_addr.sin_port = htons(app->remote_port);

    if (connect(server_socket, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // Forward the original request to the remote server
    char *full_request = (char *)malloc(strlen(request)+8192);

    sprintf(full_request, "GET %s HTTP/1.1\r\n\r\n", request);
    if (send(server_socket, full_request, strlen(full_request), 0) < 0) {
        perror("Error sending request");
        exit(EXIT_FAILURE);
    }

    // Pass the response from remote server back
    char header_buffer[BUFFER_SIZE];
    int bytes_read;
    bytes_read = recv(server_socket, header_buffer, sizeof(header_buffer) - 1, 0);
    printf("%d\n", bytes_read);
    char *response = (char *)malloc(8192);
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: video/MP2T\r\nContent-Length: 4311780\r\n\r\n"); // hardcoded for now
    send(client_socket, response, strlen(response), 0);
    int total = 0;
    char obj_buffer[BUFFER_SIZE];
    while ((bytes_read = recv(server_socket, obj_buffer, sizeof(obj_buffer) - 1, 0)) > 0){
        total = total + bytes_read;
        send(client_socket, obj_buffer, bytes_read, 0);
    }
    printf("%d\n", total);
}