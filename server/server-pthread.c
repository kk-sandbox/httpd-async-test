#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8888
#define BUFFER_SIZE 4096

// Function to get the current time as a formatted string
void get_current_time(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);                        // Get the current time
    timeinfo = localtime(&rawtime);        // Convert to local time

    strftime(buffer, size, "[%Y%m-%d %H:%M:%S]", timeinfo);  // Format as [YYYY-MM-DD]
}

// Function to handle incoming client requests
void *handle_client(void *client_sock) {
    int client_socket = *((int *)client_sock);
    free(client_sock);  // Free the malloc'ed socket pointer
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read the request from the client
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Failed to read from client");
        close(client_socket);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Received request\n");
    printf("\n%s\n", buffer);

    // CORS headers to be included in all responses
    char cors_headers[] =
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n";

    // Buffer to hold the formatted time
    char time_str[50];
    get_current_time(time_str, sizeof(time_str));  // Get the current time

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
        printf("Done.\n");
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
                 "%s Hello world!\n",
                 cors_headers, time_str);

        printf("Sending GET response...\n");
        sleep(10);  // Simulate workload
        write(client_socket, response, strlen(response));
        printf("Done.\n");
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
                 "%s Hello world!\n",
                 cors_headers, time_str);

        printf("Sending POST response...\n");
        sleep(10);  // Simulate workload
        write(client_socket, response, strlen(response));
        printf("Done.\n");
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

        printf("Sending cors header...\n");
        write(client_socket, response, strlen(response));
        printf("Done.\n");
    }

    close(client_socket);
    return NULL;
}

int main() {
    int opt = 1;
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
        perror("Failed to set socket option");
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

    while (1) {
        // Accept a new client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Failed to accept client");
            continue;
        }

        printf("New client connected...\n");

        // Create a thread to handle the client request
        pthread_t client_thread;
        int *client_sock = malloc(sizeof(int));  // Allocate memory for client socket
        *client_sock = client_socket;

        if (pthread_create(&client_thread, NULL, handle_client, (void*)client_sock) != 0) {
            perror("Failed to create thread");
            free(client_sock);
            close(client_socket);
        }

        // Detach the thread to allow it to clean up after finishing
        pthread_detach(client_thread);
    }

    close(server_socket);
    return 0;
}

