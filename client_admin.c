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
#define BUFFER_SIZE 20000
#define BUF_SIZE 1024 // definim marime buffer


void connect_to_admin_server();

void connect_to_admin_server() {
    struct sockaddr_un server_addr;
    int sockfd, n;
    char buffer[BUF_SIZE];

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

        printf("1.System processes and resource usage \n");
        printf("2.Server uptime\n");
        printf("3.Disk space usage\n");
        printf("4.Active network connections and listening ports\n");
        printf("5.Network interfaces configured on the system\n");
        printf("6.Files and directories in the current directory\n");
        printf("7.Exit\n");
        printf("Choose option:");


        fgets(buffer, BUF_SIZE, stdin);
        //buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        int option = atoi(buffer);

        //printf("buffer int %d\n",atoi(buffer));
        // trimitem comanda la server
        if (send(sockfd, &buffer, sizeof(buffer), 0) < 0) {
            perror("send failed"); // eroare trimitere comanda
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if(option == 7)
        {
            printf("Exiting...\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            bzero(buffer, sizeof(buffer));
            // primim raspunsul de la server
            if ((n = recv(sockfd, buffer, BUFFER_SIZE, 0)) < 0) {
                perror("recv failed"); // eroare primire raspuns
                exit(EXIT_FAILURE);
            }
            buffer[n] = '\0';
            printf("%s\n", buffer); // printam raspunsul
        }
    }

    // inchidem socketul
    close(sockfd);
}

int main() {
 printf("Connect to admin server\n");   
 connect_to_admin_server();

    return 0;
}