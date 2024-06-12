#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8080
#define ADMIN_SOCKET_PATH "/tmp/admin_socket"
#define BUFFER_SIZE 4095


void send_file(int socket_fd, const char *file_path);
void receive_file(int socket_fd, const char *input_path);
void communicate_with_server(int socket_fd);
void connect_to_admin_server();
void connect_to_simple_server();

void send_file(int socket_fd, const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        perror("Failed to get file size");
        close(fd);
        return;
    }
    size_t file_size = file_stat.st_size;
    write(socket_fd, &file_size, sizeof(file_size));

    printf("Size of the file being sent: %zu bytes\n", file_size);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        if (write(socket_fd, buffer, bytes_read) != bytes_read) {
            perror("Failed to send file");
            close(fd);
            return;
        }
        printf("Sent to server %zd bytes:\n", bytes_read);
    }

    close(fd);
}

void receive_file(int socket_fd, const char *input_path) {
    char buffer[BUFFER_SIZE];

    char new_extension[BUFFER_SIZE];
    char file_path_codex[BUFFER_SIZE];

     // Read the file path
    ssize_t path_length = recv(socket_fd, file_path_codex, sizeof(file_path_codex) - 1, 0);
    if (path_length <= 0) {
        perror("Failed to read file path");
        return;
    }
    file_path_codex[path_length] = '\0'; 

    // Read the new file extension
    ssize_t extension_length = recv(socket_fd, new_extension, sizeof(new_extension) - 1, 0);
    if (extension_length <= 0) {
        perror("Failed to read new file extension");
        return;
    }
    new_extension[extension_length] = '\0'; 

    // Generate the full output path with the new extension
    char output_file_path[BUFFER_SIZE];
    snprintf(output_file_path, BUFFER_SIZE, "%s_%s.%s", "Converted",file_path_codex, new_extension);
    printf(" output file path %s\n", output_file_path);

    int fd = open(output_file_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open file for writing");
        return;
    }

   // Read the file size
    size_t file_size;
    if (recv(socket_fd, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Failed to read file size");
        close(fd);
        return;
    }

    printf("Size of the received file: %zu bytes\n", file_size);

    // Read the file content
    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    while (total_bytes_received < file_size && (bytes_received = recv(socket_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (write(fd, buffer, bytes_received) != bytes_received) {
            perror("Failed to write to file");
            close(fd);
            return;
        }
        total_bytes_received += bytes_received;
    }

    if (total_bytes_received == file_size) {
        printf("File received successfully\n");
    } else {
        printf("File reception incomplete: received %zu bytes out of %zu bytes\n", total_bytes_received, file_size);
    }

    printf("Converted file saved to: %s\n", output_file_path);
    close(fd);

}

void communicate_with_server(int socket_fd) {
    char buffer[BUFFER_SIZE];
    int n;
    int option;
   

    while (1) {
        char file_name[BUFFER_SIZE];
        printf("Enter input file path: ");
        scanf("%s", file_name);

        // Determine file extension
        const char *dot = strrchr(file_name, '.');
        if (!dot || dot == file_name) {
            printf("Invalid file extension.\n");
            continue;
        }
        const char *extension = dot + 1;

        // Send file extension to the server
        if (write(socket_fd, extension, strlen(extension) + 1) < 0) {
            perror("send failed"); // error sending file extension to server
            exit(EXIT_FAILURE);
        }
        bzero(buffer, sizeof(BUFFER_SIZE));
        // Receive conversion options from the server
        if ((n = recv(socket_fd, buffer, BUFFER_SIZE, 0)) < 0) {
            perror("recv failed"); // error receiving conversion options from server
            exit(EXIT_FAILURE);
        }
        buffer[n] = '\0';
        printf("Conversion options:\n%s", buffer);

        // Get user's choice for conversion
        printf("Choose an option:\n");
        scanf("%d", &option);
        bzero(buffer, sizeof(BUFFER_SIZE));

        // Send user's option choice to the server
        sprintf(buffer, "%d", option);
        if (write(socket_fd, buffer, strlen(buffer) + 1) < 0) {
            perror("send failed"); // error sending user's option choice to server
            exit(EXIT_FAILURE);
        }

        // Send the input file to the server
        send_file(socket_fd, file_name);

        // Receive the converted file from the server
        receive_file(socket_fd, file_name);
    }
    close(socket_fd);

}


void connect_to_simple_server() {
    int socket_fd;
    struct sockaddr_in address;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Connection Failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    communicate_with_server(socket_fd);
}

int main() {
    printf("Connect to simple server");
    connect_to_simple_server();
    return 0;
}