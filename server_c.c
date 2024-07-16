#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void handle_client(int client_socket);
void send_response(int client_socket, int status_code, const char *status_text, const char *content_type, const char *content);
void send_file(int client_socket, const char *filepath);
void handle_not_found(int client_socket);
void handle_bad_request(int client_socket);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        handle_client(client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv");
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Request:\n%s\n", buffer);

    char method[8], path[512], protocol[16];
    if (sscanf(buffer, "%7s %511s %15s", method, path, protocol) != 3) {
        handle_bad_request(client_socket);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        handle_bad_request(client_socket);
        return;
    }

    if (strstr(path, "..")) {
        handle_bad_request(client_socket);
        return;
    }

    char filepath[BUFFER_SIZE];
    snprintf(filepath, sizeof(filepath), ".%s", path);

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        handle_not_found(client_socket);
        return;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        strcat(filepath, "./index.html");
        if (stat(filepath, &file_stat) == -1) {
            handle_not_found(client_socket);
            return;
        }
    }

    send_file(client_socket, filepath);
}

void send_response(int client_socket, int status_code, const char *status_text, const char *content_type, const char *content) {
    char buffer[BUFFER_SIZE];
    int content_length = strlen(content);
    int length = snprintf(buffer, sizeof(buffer),
                          "HTTP/1.1 %d %s\r\n"
                          "Content-Type: %s\r\n"
                          "Content-Length: %d\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "%s",
                          status_code, status_text, content_type, content_length, content);
    send(client_socket, buffer, length, 0);
}

void send_file(int client_socket, const char *filepath) {
    int file = open(filepath, O_RDONLY);
    if (file == -1) {
        handle_not_found(client_socket);
        return;
    }

    struct stat file_stat;
    fstat(file, &file_stat);

    char buffer[BUFFER_SIZE];
    
    int length = snprintf(buffer, sizeof(buffer),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: %ld\r\n"
                          "Connection: close\r\n"
                          "\r\n",
                          file_stat.st_size);
    send(client_socket, buffer, length, 0);

    int bytes_read;
    while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    close(file);
}

void handle_not_found(int client_socket) {
    const char *not_found_response = "<html><body><h1>404 Not Found</h1></body></html>";
    send_response(client_socket, 404, "Not Found", "text/html", not_found_response);
}

void handle_bad_request(int client_socket) {
    const char *bad_request_response = "<html><body><h1>400 Bad Request</h1></body></html>";
    send_response(client_socket, 400, "Bad Request", "text/html", bad_request_response);
}
