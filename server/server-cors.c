#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 4096

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read the request from the client
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Failed to read from client");
        close(client_socket);
        return;
    }

    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n", buffer);

    // CORS headers to be included in all responses
    char cors_headers[] =
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n";

    // Check if it's a preflight OPTIONS request (for POST requests)
    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "HTTP/1.1 204 No Content\r\n"
                 "%s"
                 "Connection: close\r\n"
                 "\r\n",
                 cors_headers);

        printf("Sending CORS preflight OPTIONS response...\n");
        write(client_socket, response, strlen(response));
    }
    // Check if it's a GET request
    else if (strncmp(buffer, "GET /", 5) == 0) {
        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "%s"
                 "Connection: close\r\n"
                 "\r\n"
                 "Helloworld!\n",
                 cors_headers);

        printf("Sending GET response...\n");
        sleep(5);
        write(client_socket, response, strlen(response));
    }
    // Check if it's a POST request
    else if (strncmp(buffer, "POST /", 6) == 0) {
        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "%s"
                 "Connection: close\r\n"
                 "\r\n"
                 "Helloworld'\n",
                 cors_headers);

        printf("Sending POST response...\n");
        write(client_socket, response, strlen(response));
    }
    // Handle any other requests as 404 Not Found
    else {
        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Length: 13\r\n"
                 "%s"
                 "Connection: close\r\n"
                 "\r\n"
                 "404 Not Found\n",
                 cors_headers);

        write(client_socket, response, strlen(response));
    }

    close(client_socket);
}

int main() {
    int opt =1;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socketopt");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0) {
        perror("Failed to listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept and handle incoming connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Failed to accept client");
            continue;
        }

        printf("Client connected...\n");
        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}

