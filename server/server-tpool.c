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
#define THREAD_POOL_SIZE 5
#define TASK_QUEUE_SIZE 10

// Task struct to hold client socket
typedef struct {
    int client_socket;
} task_t;

// Circular buffer for task queue
typedef struct {
    task_t tasks[TASK_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} task_queue_t;

// Initialize the task queue
void init_task_queue(task_queue_t *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

// Add a task to the queue
void add_task_to_queue(task_queue_t *queue, int client_socket) {
    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is full
    while (queue->count == TASK_QUEUE_SIZE) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    queue->tasks[queue->tail].client_socket = client_socket;
    queue->tail = (queue->tail + 1) % TASK_QUEUE_SIZE;
    queue->count++;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// Get a task from the queue
int get_task_from_queue(task_queue_t *queue) {
    int client_socket;

    pthread_mutex_lock(&queue->mutex);

    // Wait if the queue is empty
    while (queue->count == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    client_socket = queue->tasks[queue->head].client_socket;
    queue->head = (queue->head + 1) % TASK_QUEUE_SIZE;
    queue->count--;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return client_socket;
}

// Function to handle incoming client requests
void *handle_client(void *arg) {
    task_queue_t *queue = (task_queue_t *)arg;

    while (1) {
        int client_socket = get_task_from_queue(queue);

        char buffer[BUFFER_SIZE];
        int bytes_read;

        // Read the request from the client
        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("Failed to read from client");
            close(client_socket);
            continue;
        }

        buffer[bytes_read] = '\0';
        //printf("Received request:\n%s\n", buffer);
        printf("Received request:\n");

        // CORS headers to be included in all responses
        char cors_headers[] =
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n";

        // Get the system time
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", tm_info);

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
                     "%s Acknowledged\n",
                     cors_headers, time_str);

            printf("Sending GET response...\n");
			sleep(5);
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
                     "%s Acknowledged\n",
                     cors_headers, time_str);

            printf("Sending POST response...\n");
			sleep(5);
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

            write(client_socket, response, strlen(response));
        }

        close(client_socket);
    }

    return NULL;
}

int main() {
    int opt = 1;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Initialize the task queue
    task_queue_t queue;
    init_task_queue(&queue);

    // Create worker threads
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, handle_client, (void *)&queue) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

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
		printf("\nWaiting for new connection...\n");
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Failed to accept client");
            continue;
        }

        printf("New client connected...\n");

        // Add the new client to the task queue
        add_task_to_queue(&queue, client_socket);
    }

    close(server_socket);
    return 0;
}

