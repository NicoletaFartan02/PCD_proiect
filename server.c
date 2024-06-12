#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <time.h> 
#include "convert_utils.h"
int nr_total=0;
#define PORT 8080
#define ADMIN_SOCKET_PATH "/tmp/admin_socket"
#define BUFFER_SIZE 4096
#define BUF_SIZE 1024 // definim marime buffer
#define MAX_CLIENTS 10 // Define the maximum number of clients the server can handle

time_t server_start_time;
int connected_clients = 0; // Counter for connected clients
int admin_connected = 0; // Flag to track admin connection status
pthread_mutex_t admin_lock = PTHREAD_MUTEX_INITIALIZER;
void *handle_client(void *arg);
void process_conversion(int client_fd, const char *input_file, int conversion_option);
void send_conversion_options(int client_fd, const char *extension);
void *conversion_worker(void *arg);
void *handle_admin_client(void *arg);
void *handle_simple_clients(void *arg);
void extract_filename_segment(const char *file_path, char *result) {
    const char *dot_position = strchr(file_path, '.');
    if (dot_position == NULL) {
        printf("No '.' found in the file path.\n");
        return;
    }

    // Calculate the start position for the 6 characters before the dot
    int index_before_dot = dot_position - file_path - 6;
    if (index_before_dot < 0) {
        index_before_dot = 0; // Handle case where there are less than 6 characters before the dot
    }

    // Copy the 6 characters before the dot to the result buffer
    strncpy(result, file_path + index_before_dot, 6);
    result[6] = '\0'; // Ensure null-termination
}
// Function to get server uptime
char *get_uptime() {
    time_t current_time = time(NULL);
    time_t uptime = current_time - server_start_time;
    char *uptime_msg = malloc(BUFFER_SIZE * sizeof(char));
    if (uptime_msg == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    snprintf(uptime_msg, BUFFER_SIZE, "%ld", uptime);
    return uptime_msg;
}

void send_conversion_options(int client_fd, const char *extension) {
    char options[BUFFER_SIZE] = {0};

    if (strcmp(extension, "txt") == 0) {
        strcpy(options, "1. TXT to PDF\n2. TXT to ODT\n");
    } else if (strcmp(extension, "docx") == 0) {
        strcpy(options, "3. DOCX to PDF\n4. DOCX to RTF\n");
    } else if (strcmp(extension, "odt") == 0) {
        strcpy(options, "5. ODT to TXT\n6. ODT to PDF\n");
    } else if (strcmp(extension, "rtf") == 0) {
        strcpy(options, "7. RTF to DOCX\n8. RTF to PDF\n");
    } else if (strcmp(extension, "pdf") == 0) {
        strcpy(options, "9. PDF to DOCX\n10. PDF to ODT\n11. PDF to RTF\n12. PDF to HTML\n");
    } else {
        strcpy(options, "Unsupported file extension.\n");
    }
    printf("Sent\n");
    printf("%s\n", options);
    write(client_fd, options, strlen(options));
    fflush(stdout);

}


void send_file_to_client(int client_fd, const char *dir_path, const char *extension) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    char file_path[BUFFER_SIZE];
    int found_file = 0;

    // Find the first regular file in the directory
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
            found_file = 1;
            break;
        }
    }

    closedir(dir);

    if (!found_file) {
        fprintf(stderr, "No regular file found in directory\n");
        return;
    }

    int fd = open(file_path, O_RDONLY);
    printf("Output file path: %s\n", file_path);
    if (fd == -1) {
        perror("Failed to open file");
        return;
    }
    char result[7]; // 6 characters + null terminator

    extract_filename_segment(file_path, result);

    printf("Segment before the first dot: %s\n", result);
    // Send the file path first
    if (send(client_fd, result, strlen(result) + 1, 0) < 0) {
        perror("Failed to send file path");
        close(fd);
        return;
    }
    printf("File path sent: %s\n", file_path);
    sleep(0.2); // Optional: Ensure the file path is sent before the extension

    // Send the file extension first
    if (send(client_fd, extension, strlen(extension) + 1, 0) < 0) {
        perror("Failed to send file extension");
        close(fd);
        return;
    }
     printf("File size received: %s\n", extension);
    sleep(0.2); // Optional: Ensure the extension is sent before the file size

    // Send the file size next
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        perror("Failed to get file size");
        close(fd);
        return;
    }
    size_t file_size = file_stat.st_size;
    if (send(client_fd, &file_size, sizeof(file_size), 0) < 0) {
        perror("Failed to send file size");
        close(fd);
        return;
    }
    printf("File size received: %zu\n", file_size);
    // Send the file content
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) != bytes_read) {
            perror("Failed to send file");
            close(fd);
            return;
        }
        printf("Sent to client %zd bytes\n", bytes_read);
    }

    close(fd);
}

void process_conversion(int client_fd, const char *input_file, int conversion_option) {
    char output_file_template[BUFFER_SIZE] = "output_file_XXXXXX";
    int output_fd = mkstemp(output_file_template);
    if (output_fd == -1) {
        perror("Failed to create temporary output file");
        return;
    }
    close(output_fd); // Close the file descriptor, we'll use the filename

    char output_file[BUFFER_SIZE];
    const char *extension;

    // Choose the right conversion based on the option
    switch (conversion_option) {
        case 1:
            extension = "pdf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_txt_to_pdf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);

            break;
        case 2:
            extension = "odt";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_txt_to_odt(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);

            break;
        case 3:
            extension = "pdf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_docx_to_pdf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);

            break;
        case 4:
            extension = "rtf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_docx_to_rtf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 5:
            extension = "txt";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_odt_to_txt(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 6:
            extension = "pdf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_odt_to_pdf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 7:
            extension = "docx";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_rtf_to_docx(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 8:
            extension = "pdf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_rtf_to_pdf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 9:
            extension = "docx";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_pdf_to_docx(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);

            break;
        case 10:
            extension = "odt";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_pdf_to_odt(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 11:
            extension = "rtf";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_pdf_to_rtf(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        case 12:
            extension = "html";
            snprintf(output_file, sizeof(output_file), "%s%s", output_file_template, extension);
            convert_pdf_to_html(input_file, output_file);
            send_file_to_client(client_fd, output_file, extension);
            break;
        default:
            write(client_fd, "Invalid conversion option.\n", 27);
            return;
    }
   
    // Send the converted file back to the client

    // Delete the temporary output file after sending
    unlink(output_file);
}
char *execute_command(const char *cmd) {
    static char cmd_output[3*BUFFER_SIZE]; // Static to retain value after function returns
    FILE *fp = popen(cmd, "r"); // Open a pipe to execute the command
    if (fp == NULL) { // Check for failure to open pipe
        strcpy(cmd_output, "Error executing command\n");
        return cmd_output;
    }

    memset(cmd_output, 0, BUFFER_SIZE); // Clear the output buffer
    char line[BUFFER_SIZE]={0};
    while (fgets(line, sizeof(line), fp) != NULL) { // Read command output line by line
        strncat(cmd_output, line, BUFFER_SIZE - strlen(cmd_output) - 1); // Append to output buffer
    }
    pclose(fp); // Close the pipe
    return cmd_output;
}

void *handle_client_admin(void *arg) {
    int client_sockfd = *((int *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        printf("::: Waiting for commands from client...\n");

        if ((n = recv(client_sockfd, buffer, BUFFER_SIZE, 0)) < 0) {
            perror("recv failed"); // Error receiving command from client
            exit(EXIT_FAILURE);
        }
        buffer[n] = '\0';
        printf("Received option: %s\n", buffer);
        int command_choice = atoi(buffer);

        char *stats;
        switch (command_choice) {
            case 1:
                stats = execute_command("top -bn1 -i");
                printf("::: Sending current top stats to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 2:
                stats = get_uptime();
                printf("::: Sending server uptime to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 3:
                stats = execute_command("df -h");
                printf("::: Sending disk usage stats to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 4:
                stats = execute_command("netstat -tuln");
                printf("::: Sending network connection stats to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 5:
                stats = execute_command("ifconfig");
                printf("::: Sending network interface stats to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 6:
                stats = execute_command("ls -l");
                printf("::: Sending directory listing to client...\n");
                send(client_sockfd, stats, strlen(stats), 0);
                break;
            case 7:
                printf("Client admin disconnected\n");
                close(client_sockfd);
                pthread_exit(NULL);
                break;
            default:
                send(client_sockfd, "Invalid command", 15, 0);
                break;
        }
    }

    printf("Admin disconnected.\n");
    close(client_sockfd);

    // Update the admin connection status
    pthread_mutex_lock(&admin_lock);
    admin_connected = 0;
    pthread_mutex_unlock(&admin_lock);
    // Close the client's socket
    close(client_sockfd);
    pthread_exit(NULL);
}

void *handle_admin(void *arg) {
    nr_total++;
    unlink(ADMIN_SOCKET_PATH);

    struct sockaddr_un server_addr, client_addr;
    int server_sockfd, client_sockfd, *new_sock;
    socklen_t client_len;
    pthread_t tid;

    // Create UNIX socket for the server
    if ((server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server attributes
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, ADMIN_SOCKET_PATH);

    // Bind socket to the specified path
    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for client connections, allow only one connection
    if (listen(server_sockfd, 1) < 0) {
        perror("Listen failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("::: Waiting for admin connection...\n");

        // Accept client connection
        client_len = sizeof(client_addr);
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Accept failed");
            close(server_sockfd);
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&admin_lock);
        if (admin_connected) {
            printf("::: Admin already connected, rejecting new connection...\n");
            close(client_sockfd);
        } else {
            admin_connected = 1;
            pthread_mutex_unlock(&admin_lock);

            printf("::: Admin connected...\n");

            // Create a new thread for the admin client
            new_sock = malloc(sizeof(int));
            *new_sock = client_sockfd;
            if (pthread_create(&tid, NULL, handle_client_admin, (void *)new_sock) < 0) {
                perror("pthread_create failed");
                close(client_sockfd);
                exit(EXIT_FAILURE);
            }

            // Detach the thread so that resources are freed when the thread finishes
            pthread_detach(tid);
        }
        pthread_mutex_unlock(&admin_lock);
    }

    // Close sockets
    close(server_sockfd);
    nr_total--;
    unlink(ADMIN_SOCKET_PATH);
    return NULL;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE] = {0};
    char extension[BUFFER_SIZE] = {0};
    int dim;
    int conversion_option;
    while (1) {
        // Read the file extension from the client
        bzero(extension, sizeof(extension));
        ssize_t recv_bytes = recv(client_fd, extension, sizeof(extension) - 1, 0);
        if (recv_bytes <= 0) {
            perror("Failed to receive file extension");
            close(client_fd);
            return NULL;
        }
        extension[recv_bytes] = '\0'; // Ensure the extension is null-terminated
        printf("Extension received: %s\n", extension);

        // Send the conversion options based on the extension
        send_conversion_options(client_fd, extension);

        bzero(buffer, sizeof(buffer));
        // Read the conversion option from the client
        recv_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (recv_bytes <= 0) {
            perror("Failed to receive conversion option");
            close(client_fd);
            return NULL;
        }
        buffer[recv_bytes] = '\0'; // Ensure the buffer is null-terminated
        conversion_option = atoi(buffer);
        printf("Conversion option chosen: %d\n", conversion_option);
    
        // Read the file size from the client
        size_t file_size;
        recv_bytes = recv(client_fd, &file_size, sizeof(file_size), 0);
        if (recv_bytes <= 0) {
            perror("Failed to receive file size");
            close(client_fd);
            return NULL;
        }
        printf("File size received: %zu\n", file_size);
        // Read file from client
        char input_file_template[BUFFER_SIZE] = "input_file_XXXXXX";
        int input_fd = mkstemp(input_file_template);
        if (input_fd == -1) {
            perror("Failed to create temporary input file");
            close(client_fd);
            return NULL;
        }

        ssize_t bytes_received;
        size_t total_bytes_received = 0;

        while (total_bytes_received < file_size) {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received < 0) {
                perror("Error receiving file data");
                close(input_fd);
                close(client_fd);
                return NULL;
            } else if (bytes_received == 0) {
                // End of file
                break;
            }
            printf("Received %zd bytes:\n", bytes_received);

            write(input_fd, buffer, bytes_received);
            total_bytes_received += bytes_received;
        }
        close(input_fd);

        // Rename the temporary file to include the original extension
        char input_file_with_extension[BUFFER_SIZE];
        snprintf(input_file_with_extension, sizeof(input_file_with_extension), "%s.%s", input_file_template, extension);
        rename(input_file_template, input_file_with_extension);

        process_conversion(client_fd, input_file_with_extension, conversion_option);

        // Delete the temporary input file
        unlink(input_file_with_extension);
        // nr_total--;
    }
    close(client_fd);
    return NULL;
}

void *handle_simple_clients(void *arg) {
    int server_fd, new_socket,*n_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    socklen_t client_len;
    pthread_t tid;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
         printf("::: Client connected...\n");

        // cream un nou thread pentru fiecare client
        n_sock = malloc(sizeof(int));
        *n_sock = new_socket;
        if (pthread_create(&tid, NULL, handle_client, (void *)n_sock) < 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }

        // detasam thread-ul astfel incat resursele sa fie eliberate cand thread-ul termina
        pthread_detach(tid);
        // nr_total++;
        // handle_client(new_socket);
        // close(new_socket);
    }

    close(server_fd);

    return NULL;
}

int main() {
    server_start_time = time(NULL);
    pthread_t admin_thread, clients_thread, worker_thread;

    pthread_create(&admin_thread, NULL, handle_admin, NULL);
    pthread_create(&clients_thread, NULL, handle_simple_clients, NULL);

    pthread_join(admin_thread, NULL);
    pthread_join(clients_thread, NULL);
    return 0;
}