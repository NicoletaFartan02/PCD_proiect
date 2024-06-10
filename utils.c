#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// functia trimite antetul mesajului prin socket
void send_header(int socket, MessageHeader *header) {
    // ensure correct buffer size by using sizeof(MessageHeader)
    char buffer[sizeof(MessageHeader)];
    memset(buffer, 0, sizeof(buffer));

    // copy operation
    strncpy(buffer, header->operation, sizeof(header->operation) - 1);
    buffer[sizeof(header->operation) - 1] = '\0'; // ensure null-termination

    // convert and copy file_size to network byte order
    int32_t file_size_n = htonl(header->file_size);
    memcpy(buffer + sizeof(header->operation), &file_size_n, sizeof(file_size_n));

    // trimitem antetul prin socket
    if (send(socket, buffer, sizeof(buffer), 0) != sizeof(buffer)) {
        perror("failed to send header");
        exit(EXIT_FAILURE);
    }
}

// functia primeste antetul mesajului prin socket
void receive_header(int socket, MessageHeader *header) {
    char buffer[sizeof(MessageHeader)];
    memset(buffer, 0, sizeof(buffer));

    // primim antetul prin socket
    if (recv(socket, buffer, sizeof(buffer), 0) != sizeof(buffer)) {
        perror("failed to receive header");
        exit(EXIT_FAILURE);
    }

    // copy operation
    strncpy(header->operation, buffer, sizeof(header->operation) - 1);
    header->operation[sizeof(header->operation) - 1] = '\0';

    // copy and convert file_size to host byte order
    int32_t file_size_n;
    memcpy(&file_size_n, buffer + sizeof(header->operation), sizeof(file_size_n));
    header->file_size = ntohl(file_size_n);
}