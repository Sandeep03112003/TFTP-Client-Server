#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tftp.h"
#include "tftp_client.h"

int main()
{
    char command[256];
    tftp_client_t client;
    memset(&client, 0, sizeof(client)); // Initialize client structure

    // Main loop for command-line interface
    while (1)
    {
        printf("tftp> ");
        fgets(command, sizeof(command), stdin);

        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        // Process the command
        process_command(&client, command);
    }

    return 0;
}

// Function to process commands
void process_command(tftp_client_t *client, char *command)
{
    char buffer[20];
    if (sscanf(command, "%s", buffer) != 1)
    {
        perror("sscanf");
        return;
    }
    if (strcmp(buffer, "connect") == 0)
    {
        char ip[20];
        int port;
        if (sscanf(command, "connect %s %d", ip, &port) != 2)
        {
            perror("sscanf");
            return;
        }
        connect_to_server(client, ip, port);
    }
    else if (strcmp(buffer, "put") == 0)
    {
        char filename[30];
        if (sscanf(command, "put %s", filename) != 1)
        {
            perror("sscanf");
            return;
        }
        put_file(client, filename);
    }
    else if (strcmp(buffer, "get") == 0)
    {
        char filename[30];
        if (sscanf(command, "get %s", filename) != 1)
        {
            perror("sscanf");
            return;
        }
        get_file(client, filename);
    }
    else if (strcmp(buffer, "quit") == 0)
    {
        disconnect(client);
    }
}

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return;
    }

    // Set socket timeout option
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        perror("setsockopt");
        client->sockfd=-1;
        close(sockfd);
        return;
    }

    // Set up server address
    memset(&(client->server_addr), 0, sizeof(client->server_addr));
    client->sockfd = sockfd;
    client->server_addr.sin_family = AF_INET;
    client->server_len=sizeof(client->server_addr);
    client->server_addr.sin_port = htons(port);
    int ret;
    if((ret=inet_pton(AF_INET, ip, &(client->server_addr.sin_addr)))==1)
    {
          printf("Connected to server %s:%d successfully.\n", ip, port);
        return;
    }
    else if(ret==0)
    {
        printf("Invalid IP address\n");
        client->sockfd=-1;
        close(sockfd);
        return;
    }
    else
    {
        perror("inet_pton");
        client->sockfd=-1;
        close(sockfd);
        return;
    }

}

void put_file(tftp_client_t *client, char *filename)
{
    // Step 1: Validate client socket
    if (client->sockfd < 0)
    {
        printf("Connect to the server first.\n");
        return;
    }

    // Step 2: Open file
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL)
    {
        perror("fopen");
        return;
    }

    // Step 3: Construct WRQ request
    char request[2 + strlen(filename) + 1 + 6];

    uint16_t opcode = htons(WRQ);
    memcpy(request, &opcode, 2);

    int offset = 2;

    strcpy(request + offset, filename);
    offset += strlen(filename) + 1;

    strcpy(request + offset, "octet");
    offset += strlen("octet") + 1;

    // Step 4: Send WRQ
    int ret = sendto(client->sockfd, request, offset, 0,
                     (struct sockaddr *)&client->server_addr,
                     client->server_len);

    if (ret == -1 || ret != offset)
    {
        perror("sendto WRQ");
        fclose(fptr);
        return;
    }

    // Step 5: Receive ACK block 0
    tftp_packet response;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    ret = recvfrom(client->sockfd, &response, sizeof(response), 0,
                   (struct sockaddr *)&server_addr, &server_len);

    if (ret != 4)
    {
        printf("Invalid ACK 0 or timeout.\n");
        fclose(fptr);
        return;
    }

    uint16_t ack_opcode;
    uint16_t ack_block;

    memcpy(&ack_opcode, &response, 2);
    memcpy(&ack_block, (char *)&response + 2, 2);

    ack_opcode = ntohs(ack_opcode);
    ack_block = ntohs(ack_block);

    if (ack_opcode != ACK || ack_block != 0)
    {
        printf("Expected ACK block 0.\n");
        fclose(fptr);
        return;
    }

    // Step 6: Read and send DATA blocks
    uint16_t block = 1;
    tftp_packet packet;

    while (1)
    {
        memset(&packet, 0, sizeof(packet));

        size_t bytes = fread(packet.body.data_packet.data, 1, 512, fptr);

        if (ferror(fptr))
        {
            perror("fread");
            fclose(fptr);
            return;
        }

        packet.opcode = htons(DATA);
        packet.body.data_packet.block_number = htons(block);

        int packet_len = bytes + 4;
        int acknowledged = 0;

        // Step 7: Send DATA and wait for its ACK
        for (int attempt = 0; attempt < 3; attempt++)
        {
            ret = sendto(client->sockfd, &packet, packet_len, 0,
                         (struct sockaddr *)&client->server_addr,
                         client->server_len);

            if (ret == -1)
            {
                perror("sendto DATA");
                fclose(fptr);
                return;
            }

            ret = recvfrom(client->sockfd, &response, sizeof(response), 0,
                           (struct sockaddr *)&server_addr, &server_len);

            if (ret != 4)
                continue;

            memcpy(&ack_opcode, &response, 2);
            memcpy(&ack_block, (char *)&response + 2, 2);

            ack_opcode = ntohs(ack_opcode);
            ack_block = ntohs(ack_block);

            if (ack_opcode == ACK && ack_block == block)
            {
                acknowledged = 1;
                break;
            }
        }

        if (!acknowledged)
        {
            printf("Failed to receive ACK for block %u.\n", block);
            fclose(fptr);
            return;
        }

        // Step 8: Check final DATA block
        if (bytes < 512)
            break;

        block++;
    }

    // Step 9: Close file
    fclose(fptr);
    printf("File uploaded successfully: %s\n", filename);
}

void get_file(tftp_client_t *client, char *filename)
{
    if (client->sockfd < 0)
    {
        printf("Connect to the server first.\n");
        return;
    }

    // Step 1: Open local file to store received data
    char buffer[30];
    sprintf(buffer,"client_local_%s",filename);
    FILE *fptr = fopen(buffer, "wb");
    if (fptr == NULL)
    {
        perror("fopen");
        return;
    }

    // Step 2: Construct RRQ request
    char request[2 + strlen(filename) + 1 + 6];

    uint16_t opcode = htons(RRQ);
    memcpy(request, &opcode, 2);

    int offset = 2;

    strcpy(request + offset, filename);
    offset += strlen(filename) + 1;

    strcpy(request + offset, "octet");
    offset += strlen("octet") + 1;

    // Step 3: Send RRQ request to server
    int ret = sendto(client->sockfd, request, offset, 0,
                     (struct sockaddr *)&client->server_addr,
                     client->server_len);

    if (ret == -1)
    {
        perror("sendto RRQ");
        fclose(fptr);
        remove(filename);
        return;
    }

    // Step 4: Receive DATA packets and acknowledge them
    uint16_t expected_block = 1;
    tftp_packet packet;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    while (1)
    {
        ret = recvfrom(client->sockfd, &packet, sizeof(packet), 0,
                       (struct sockaddr *)&server_addr, &server_len);

        if (ret < 0)
        {
            perror("recvfrom DATA");
            fclose(fptr);
            remove(filename);
            return;
        }

        // Validate minimum DATA packet size
        if (ret < 4)
        {
            printf("Invalid DATA packet.\n");
            fclose(fptr);
            remove(filename);
            return;
        }

        uint16_t received_opcode = ntohs(packet.opcode);
        uint16_t received_block =
            ntohs(packet.body.data_packet.block_number);

        // Check whether this is a DATA packet
        if (received_opcode != DATA)
        {
            printf("Expected DATA packet.\n");
            fclose(fptr);
            remove(filename);
            return;
        }

        // Check the expected block number
        if (received_block != expected_block)
        {
            printf("Unexpected block number: %u\n", received_block);
            fclose(fptr);
            remove(filename);
            return;
        }

        // Step 5: Write received data to local file
        size_t data_size = ret - 4;

        if (fwrite(packet.body.data_packet.data, 1,
                   data_size, fptr) != data_size)
        {
            perror("fwrite");
            fclose(fptr);
            remove(filename);
            return;
        }

        // Step 6: Construct ACK packet
        tftp_packet ack_packet;
        memset(&ack_packet, 0, sizeof(ack_packet));

        ack_packet.opcode = htons(ACK);
        ack_packet.body.ack_packet.block_number = htons(received_block);

        // Send only 4 bytes for ACK
        ret = sendto(client->sockfd, &ack_packet, 4, 0,
                     (struct sockaddr *)&client->server_addr,
                     client->server_len);

        if (ret != 4)
        {
            perror("sendto ACK");
            fclose(fptr);
            remove(filename);
            return;
        }

        // Step 7: A DATA packet smaller than 512 bytes is the last packet
        if (data_size < 512)
            break;

        expected_block++;
    }

    // Step 8: Close file
    fclose(fptr);

    printf("File downloaded successfully: %s\n", filename);
}

void disconnect(tftp_client_t *client)
{
    if (client->sockfd >= 0)
    {
        close(client->sockfd);
        client->sockfd = -1;

        printf("Disconnected from server.\n");
    }
    else
    {
        printf("Not connected to server.\n");
    }
}
