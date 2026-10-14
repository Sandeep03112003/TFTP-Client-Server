# TFTP Client-Server Implementation in C

A client-server file transfer application implemented in C using UDP sockets on Linux. This project demonstrates network programming, client-server communication, packet handling, and file operations through a simplified implementation of the Trivial File Transfer Protocol (TFTP).

## Overview

The project implements a TFTP client and server that communicate over UDP to transfer files between two endpoints.

The client can upload files to the server and download files from the server using TFTP request, data, and acknowledgment packets.

The implementation focuses on understanding how application-level protocols work, how UDP sockets facilitate communication, and how file transfers are handled in C.

## Features

- UDP socket-based client-server communication
- TFTP Read Request (RRQ) and Write Request (WRQ) handling
- File upload from client to server
- File download from server to client
- DATA and ACK packet handling
- Block-based file transfer
- Client command processing
- File handling using standard C file I/O
- Server-side request handling

## Technologies Used

| Technology | Purpose |
|---|---|
| C | Application development |
| Linux | Development and execution environment |
| UDP Sockets | Client-server communication |
| TFTP | File transfer protocol |
| GCC | Compilation |

## Project Structure

```text
TFTP-Client-Server/
│
├── tftp.h
├── tftp.c
├── tftp_client.h
├── tftp_client.c
├── tftp_server.c
└── README.md
```

### File Description

| File | Description |
|---|---|
| `tftp.h` | Defines TFTP opcodes, packet structures, constants, and shared declarations. |
| `tftp.c` | Implements server-side file transfer operations. |
| `tftp_client.h` | Defines the client structure and client function declarations. |
| `tftp_client.c` | Implements client initialization, connection configuration, upload, download, and command processing. |
| `tftp_server.c` | Initializes the UDP server socket and handles incoming client requests. |
| `README.md` | Documents the project, compilation, and execution steps. |

## How It Works

The application uses UDP sockets to exchange TFTP packets between the client and server.

### File Upload

1. The client sends a Write Request (WRQ) to the server.
2. The server prepares to receive the requested file and responds with an acknowledgment.
3. The client sends the file as DATA packets.
4. The server receives the data and writes it to a file.
5. The server acknowledges received data blocks.
6. The transfer completes when the final data block is received and acknowledged.

### File Download

1. The client sends a Read Request (RRQ) to the server.
2. The server opens the requested file.
3. The server sends file contents in DATA packets.
4. The client writes the received data to a local file.
5. The client acknowledges received data blocks.
6. The transfer completes when the final data block is received.

## Packet Types

The implementation uses the following TFTP packet types:

| Opcode | Packet | Purpose |
|---|---|---|
| 1 | RRQ | Requests a file from the server. |
| 2 | WRQ | Requests permission to upload a file. |
| 3 | DATA | Carries file data. |
| 4 | ACK | Acknowledges a request or data block. |
| 5 | ERROR | Represents a protocol error packet. |

## Build and Run

### Prerequisites

- A Linux environment
- GCC compiler
- UDP socket programming support

### 1. Clone the Repository

```bash
git clone https://github.com/YOUR-USERNAME/TFTP-Client-Server.git
cd TFTP-Client-Server
```

Replace `YOUR-USERNAME` with your GitHub username.

### 2. Compile the Server

```bash
gcc tftp_server.c tftp.c -o server
```

### 3. Compile the Client

```bash
gcc tftp_client.c -o client
```

### 4. Start the Server

Open a terminal and run:

```bash
./server
```

The server listens for incoming TFTP requests on UDP port `6969`.

### 5. Start the Client

Open another terminal in the project directory and run:

```bash
./client
```

### 6. Connect to the Server

For testing on the same machine, enter the following command in the client:

```text
connect 127.0.0.1 6969
```

Here, `127.0.0.1` is the loopback address, and `6969` is the server's UDP port.

## Usage Examples

### Upload a File

Create a sample file in the client working directory if one does not already exist:

```bash
echo "Hello from TFTP client" > client.txt
```

Enter the following command in the client application:

```text
put client.txt
```

The client sends the file to the server.

### Download a File

Ensure the requested file exists in the server's working directory. For example:

```bash
echo "Hello from TFTP server" > test.txt
```

Enter the following command in the client application:

```text
get test.txt
```

The client requests the file and saves the received contents locally.

**Note:** The implementation uses prefixed filenames for received files, if configured in the source code. Check the source files for the exact output filenames.

## Concepts Demonstrated

This project provides practical experience with:

- UDP socket creation and communication
- Client-server application design
- Network byte order using `htons()` and `ntohs()`
- TFTP request, data, and acknowledgment handling
- Block-based data transfer
- File operations using `fopen()`, `fread()`, and `fwrite()`
- Error and timeout handling
- Linux application development in C

## Future Improvements

Potential enhancements include:

- Packet retransmission and duplicate-packet handling
- More comprehensive protocol error handling
- Support for additional TFTP error conditions
- Improved handling of file transfers and network failures
- Testing between separate machines on a network

## Author

Developed as a C and Linux systems programming project to explore UDP socket communication and file transfer protocol implementation.
