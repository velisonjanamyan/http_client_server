#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUFFER_SIZE 1024

void openInBrowser(char *response) {
    if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
        printf("adsfa");
        char *content = strstr(response, "\r\n\r\n");
        if (content) {
            content += 4;
            FILE *dest = fopen("example.html", "w+");
            if (!dest) {
                perror("Unable to create file.");
                return;
            }
            fprintf(dest, "%s", content);
            fclose(dest);

            char command[256] = {0};
            if (snprintf(command, sizeof(command), "open %s", "example.html") >= sizeof(command)) {
                fprintf(stderr, "Command buffer overflow.\n");
                return;
            }
            if (system(command) == -1) {
                perror("Unable to execute command.");
            }
        } else {
            printf("HTML content not found in the response.\n");
        }
    } else {
        printf("%s\n", response);
    }
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    char buffer[BUFFER_SIZE];
    int port = atoi(argv[2]);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
   

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        exit(EXIT_FAILURE);
    }


    snprintf(buffer, sizeof(buffer), "GET /index.html HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", argv[1]);
    send(client_socket, buffer, strlen(buffer), 0);

    int bytes_received;
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        // printf("%s", buffer);
    }

    openInBrowser(buffer);

    close(client_socket);
    return 0;
}
