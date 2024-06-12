import socket
import os
import struct

SERVER_IP = '127.0.0.1'
PORT = 8080
BUFFER_SIZE = 4096  # Adjusted buffer size to 4096

def send_file(socket, file_path):
    try:
        with open(file_path, 'rb') as file:
            file_size = os.path.getsize(file_path)
            socket.sendall(struct.pack('<Q', file_size))

            print(f'Size of the file being sent: {file_size} bytes')

            while True:
                bytes_read = file.read(BUFFER_SIZE)
                if not bytes_read:
                    break
                socket.sendall(bytes_read)
                print(f'Sent to server {len(bytes_read)} bytes')
    except Exception as e:
        print(f'Failed to send file: {e}')

def receive_file(socket, input_path):
    try:
        # Receive file path codex
        file_path_codex = socket.recv(BUFFER_SIZE).decode('utf-8').rstrip('\x00')
        # Receive new extension
        new_extension = socket.recv(BUFFER_SIZE).decode('utf-8').rstrip('\x00')

        output_file_path = f'Converted_{file_path_codex}.{new_extension}'
        print(f'Output file path: {output_file_path}')

        # Receive file size
        file_size_data = socket.recv(struct.calcsize('<Q'))
        file_size = struct.unpack('<Q', file_size_data)[0]
        print(f'Size of the received file: {file_size} bytes')

        # Receive file content
        with open(output_file_path, 'wb') as file:
            total_bytes_received = 0
            while total_bytes_received < file_size:
                bytes_read = socket.recv(min(BUFFER_SIZE, file_size - total_bytes_received))
                if not bytes_read:
                    break
                file.write(bytes_read)
                total_bytes_received += len(bytes_read)
                print(f'Received {len(bytes_read)} bytes')

            if total_bytes_received == file_size:
                print('File received successfully')
            else:
                print(f'File reception incomplete: received {total_bytes_received} bytes out of {file_size} bytes')

        print(f'Converted file saved to: {output_file_path}')
    except Exception as e:
        print(f'Failed to receive file: {e}')

def communicate_with_server(socket):
    try:
        while True:
            file_name = input('Enter input file path: ')

            if not os.path.isfile(file_name):
                print('Invalid file path.')
                continue

            extension = os.path.splitext(file_name)[1][1:]
            if not extension:
                print('Invalid file extension.')
                continue

            socket.sendall(extension.encode())

            conversion_options = socket.recv(BUFFER_SIZE)
            print(f'Conversion options:\n{conversion_options.decode()}')

            option = input('Choose an option: ')
            socket.sendall(option.encode())

            send_file(socket, file_name)
            receive_file(socket, file_name)
    except KeyboardInterrupt:
        print("\nClient terminated.")

def connect_to_simple_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((SERVER_IP, PORT))
        communicate_with_server(sock)
            
if __name__ == "__main__":
    print("Connecting to simple server")
    connect_to_simple_server()
