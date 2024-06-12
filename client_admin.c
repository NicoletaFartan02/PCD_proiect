#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ADMIN_SOCKET_PATH "/tmp/admin_socket"
#define BUFFER_SIZE 4096
#define BUF_SIZE 1024 // definim marime buffer

void send_file(int socket_fd, const char *file_path);
void receive_file(int socket_fd, const char *input_path);
void generate_output_path(const char *input_path, const char *new_extension, char *output_path);
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

void connect_to_admin_server() {
    struct sockaddr_un server_addr;
    int sockfd, n;
    char buffer[BUF_SIZE];
    // if (access(ADMIN_SOCKET_PATH, F_OK) == 0) {
    //     unlink(ADMIN_SOCKET_PATH);
    // }
    // cream socketul UNIX
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed"); // eroare creare socket
        exit(EXIT_FAILURE);
    }

    // initializam atributele serverului
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, ADMIN_SOCKET_PATH);

    // ne conectam la server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {

        printf("1.Convert Files\n");
        printf("2.Server uptime\n");
        printf("3.Clients online\n");
        printf("4.Quit\n");
        printf("Choose option:");


        fgets(buffer, BUF_SIZE, stdin);
        //buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        int option = atoi(buffer);

        printf("buffuer int %d\n",atoi(buffer));
        // trimitem comanda la server
        if (send(sockfd, &buffer, sizeof(buffer), 0) < 0) {
            perror("send failed"); // eroare trimitere comanda
            close(sockfd);
            exit(EXIT_FAILURE);
        }
         // Process server response based on the option
        switch (option) {
            case 1:
               { int n;
                int option;
                while(1){
                char file_name[BUFFER_SIZE];
                printf("Enter input file path: ");
                scanf("%s", file_name);
                // if(strcmp(file_name,"exit")==0)
                // {
                //     break;
                // }
                // Determine file extension
                const char *dot = strrchr(file_name, '.');
                if (!dot || dot == file_name) {
                    printf("Invalid file extension.\n");
                    continue;
                }
                const char *extension = dot + 1;

                // Send file extension to the server
                if (write(sockfd, extension, strlen(extension) + 1) < 0) {
                    perror("send failed"); // error sending file extension to server
                    exit(EXIT_FAILURE);
                }
                bzero(buffer, sizeof(BUFFER_SIZE));
                // Receive conversion options from the server
                if ((n = recv(sockfd, buffer, BUFFER_SIZE, 0)) < 0) {
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
                if (write(sockfd, buffer, strlen(buffer) + 1) < 0) {
                    perror("send failed"); // error sending user's option choice to server
                    exit(EXIT_FAILURE);
                }

                // Send the input file to the server
                send_file(sockfd, file_name);

                // Receive the converted file from the server
                receive_file(sockfd, file_name);
                }
                break;
               }
            case 2:
                // Receive and display server response
                if (recv(sockfd, buffer, BUF_SIZE, 0) < 0) {
                    perror("recv failed");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                printf("%s\n", buffer);
                break;
            case 3:
                // Receive and display server response
                if (recv(sockfd, buffer, BUF_SIZE, 0) < 0) {
                    perror("recv failed");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                printf("%s\n", buffer);
                break;
            case 4:
                printf("Exiting...\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid option\n");
        }

    }

    // inchidem socketul
    close(sockfd);
}

int main() {
 printf("Connect to admin server");   
 connect_to_admin_server();

    return 0;
}