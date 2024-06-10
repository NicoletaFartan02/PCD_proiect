#include "utils.h"
#include "pdf_utils.h"
#include "convert_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

// structura pentru argumentele thread-ului
struct ThreadArgs {
    int client_socket;
};

// functia care gestioneaza un client, rulata intr-un thread separat
void *handle_client(void *args) {
    struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
    int client_socket = thread_args->client_socket;
    free(args);

    process_client(client_socket);

    close(client_socket);

    pthread_exit(NULL);
}

// functia care proceseaza cererile de la un client
void process_client(int client_socket) {
    MessageHeader header;
    receive_header(client_socket, &header);

    printf("received header from client: operation=%s, file_size=%d\n", header.operation, header.file_size);

    char buffer[1024];
    int bytes_received, total_bytes = 0;
    const char *received_filename = "received_file";
    const char *pdf_filename = "received_file.pdf";
    const char *docx_filename = "received_file.docx";
    const char *rtf_filename = "received_file.rtf";
    const char *html_filename = "received_file.html";
    const char *image_to_add = "image.jpg";

    FILE *file = fopen(received_filename, "wb");
    if (!file) {
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }

    // primim fisierul de la client si il salvam pe disc
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes += bytes_received;
        if (total_bytes >= header.file_size) break;
    }

    fclose(file);
    printf("file received and saved as %s. total bytes: %d\n", received_filename, total_bytes);

    // verificam ce operatie trebuie sa realizam
    if (strncmp(header.operation, "SEND", sizeof(header.operation)) == 0) {
        char text_buffer[1024];
        bytes_received = recv(client_socket, text_buffer, sizeof(text_buffer), 0);
        text_buffer[bytes_received] = '\0'; // adaugare terminator null pentru a converti in sir de caractere.
        add_text_to_pdf(received_filename, "am procesat acest fisier");
        
        // trimitem un mesaj de confirmare catre client
        const char *confirmation_message = "text added successfully!";
        send(client_socket, confirmation_message, strlen(confirmation_message), 0);
    } else if (strncmp(header.operation, "PDF2DOCX", sizeof(header.operation)) == 0) {
        rename(received_filename, pdf_filename);
        pdf_to_docx(pdf_filename, docx_filename);
    } else if (strncmp(header.operation, "DOCX2PDF", sizeof(header.operation)) == 0) {
        rename(received_filename, docx_filename);
        docx_to_pdf(docx_filename, pdf_filename);
    } else if (strncmp(header.operation, "PDF2RTF", sizeof(header.operation)) == 0) {
        rename(received_filename, pdf_filename);
        pdf_to_rtf(pdf_filename, rtf_filename);
    } else if (strncmp(header.operation, "PDF2HTML", sizeof(header.operation)) == 0) {
        rename(received_filename, pdf_filename);
        pdf_to_html(pdf_filename, html_filename);
    } else if (strncmp(header.operation, "IMGINPDF", sizeof(header.operation)) == 0) {
        rename(received_filename, pdf_filename);
        add_image_to_pdf(pdf_filename, image_to_add);
    } else {
        printf("unknown operation\n");
    }

    close(client_socket);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // cream un socket tcp pentru server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // legam socket-ul la adresa specificata
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // punem serverul in modul de ascultare
    if (listen(server_fd, 5) < 0) {
        perror("failed to listen on socket");
        exit(EXIT_FAILURE);
    }

    printf("server listening on port 8080\n");

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("failed to accept connection");
            continue;
        }

        printf("new connection, socket fd is %d, ip is : %s, port : %d\n",
               client_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        struct ThreadArgs *thread_args = malloc(sizeof(struct ThreadArgs));
        if (!thread_args) {
            perror("failed to allocate memory");
            close(client_fd);
            continue;
        }
        thread_args->client_socket = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void *)thread_args) != 0) {
            perror("failed to create thread");
            free(thread_args);
            close(client_fd);
            continue;
        }
        
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}