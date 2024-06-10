#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#pragma pack(push, 1) // push current alignment and set to 1 byte alignment
typedef struct {
    char operation[16];
    int32_t file_size;
} MessageHeader;
#pragma pack(pop) // restore previous alignment

// trimite antetul mesajului prin socket
void send_header(int socket, MessageHeader *header);

// primeste antetul mesajului prin socket
void receive_header(int socket, MessageHeader *header);

#endif // UTILS_H