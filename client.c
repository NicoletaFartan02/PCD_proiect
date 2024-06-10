#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

// functie care trimite un fisier prin socket la server, impreuna cu o operatie specificata
void send_file(const char *filename, int socket, const char *operation) {
    // deschidem fisierul in modul citire binara
    FILE *file = fopen(filename, "rb");
    if (!file) {
        // daca fisierul nu poate fi deschis, afisam un mesaj de eroare si iesim
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }

    // determinam dimensiunea fisierului
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // construim si trimitem antetul mesajului
    MessageHeader header;
    strncpy(header.operation, operation, sizeof(header.operation) - 1);
    header.operation[sizeof(header.operation) - 1] = '\0';
    header.file_size = file_size;

    send_header(socket, &header);

    // citim si trimitem fisierul in bucati de cate 1024 de octeti
    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(socket, buffer, bytes_read, 0) != bytes_read) {
            // daca nu reusim sa trimitem datele, afisam un mesaj de eroare si iesim
            perror("failed to send file data");
            exit(EXIT_FAILURE);
        }
    }

    // inchidem fisierul si afisam un mesaj de succes
    fclose(file);
    printf("file %s sent to server successfully. operation: %s, file size: %d\n", filename, operation, file_size);
}

// functie care verifica credentialele dintr-un fisier text pentru a realiza login-ul
void login(char * const username, char * const password) {
    char file_username[50];
    char file_password[50];
    // deschidem fisierul cu credentiale in modul citire text
    FILE *file = fopen("credentiale.txt", "r");
    if (file == NULL) {
        // daca fisierul nu poate fi deschis, afisam un mesaj de eroare si iesim
        fprintf(stderr, "error opening credentials file: %s\n", strerror(errno));
        return;
    }
    // citim username-ul si parola din fisier
    fscanf(file, "%s %s", file_username, file_password);
    fclose(file);

    // comparam credentialele introduse de utilizator cu cele din fisier
    if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0) {
        printf("login successful!\n");
    } else {
        printf("invalid username or password!\n");
        exit(1);
    }
}

int main() {
    char username[50];
    char password[50];

    // solicitam utilizatorului sa introduca username-ul si parola
    printf("username: ");
    scanf("%s", username);
    printf("password: ");
    scanf("%s", password);

    // verificam credentialele utilizatorului
    login(username, password);

    int socket_fd;
    struct sockaddr_in server_addr;

    // cream un socket tcp
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        // daca nu reusim sa cream socket-ul, afisam un mesaj de eroare si iesim
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // ne conectam la server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        // daca nu reusim sa ne conectam la server, afisam un mesaj de eroare si iesim
        perror("failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("connected to the server.\n");

    int choice;
    char filename[256];
    char text_to_add[256];

    // solicitam utilizatorului sa selecteze o operatie
    printf("select an operation:\n");
    printf("1. transform pdf to docx\n");
    printf("2. transform docx to pdf\n");
    printf("3. add message to pdf header\n");
    printf("4. transform pdf to rtf\n");
    printf("5. transform pdf to html\n");
    printf("6. add image in pdf\n");
    printf("enter your choice: ");
    scanf("%d", &choice);

    // solicitam utilizatorului sa introduca numele fisierului
    printf("enter the filename: ");
    scanf("%s", filename);

    // executam operatia selectata
    switch (choice) {
        case 1:
            send_file(filename, socket_fd, "PDF2DOCX");
            break;
        case 2:
            send_file(filename, socket_fd, "DOCX2PDF");
            break;
        case 3:
            printf("enter the text to add: ");
            scanf("%s", text_to_add);
            send_file(filename, socket_fd, "SEND");
            break;
        case 4:
            send_file(filename, socket_fd, "PDF2RTF");
            break;
        case 5:
            send_file(filename, socket_fd, "PDF2HTML");
            break;
        case 6:
            send_file(filename, socket_fd, "IMGINPDF");
            break;
        default:
            printf("invalid choice\n");
            break;
    }

    // inchidem socket-ul
    close(socket_fd);
    return 0;
}