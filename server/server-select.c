#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8888
#define BUFFER_SIZE 4096

// Function to handle incoming client requests
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
		sleep(10);
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
		sleep(10);
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
    int server_socket, client_socket, max_sd, sd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set readfds;  // Set of socket descriptors
    int client_sockets[30] = {0};  // Track up to 30 client sockets
    int activity, i;

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

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        // Add child sockets to set
        for (i = 0; i < 30; i++) {
            // Socket descriptor
            sd = client_sockets[i];

            // If valid socket descriptor, add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Get the highest socket number
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for an activity on one of the sockets, with no timeout
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error\n");
        }

        // Check if something happened on the server socket (incoming connection)
        if (FD_ISSET(server_socket, &readfds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Failed to accept client");
                continue;
            }

            printf("New client connected...\n");

            // Add new socket to array of sockets
            for (i = 0; i < 30; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    printf("Adding client socket %d to list\n", i);
                    break;
                }
            }
        }

        // Check all clients for incoming data
        for (i = 0; i < 30; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Handle the client request
                handle_client(sd);

                // Close the socket and mark as 0 for reuse
                client_sockets[i] = 0;
            }
        }
    }

    close(server_socket);
    return 0;
}

