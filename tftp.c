/* Common file for server & client */

#include "tftp.h"

void send_file(int sockfd, struct sockaddr_in client_addr,
               socklen_t client_len, char *filename)
{
    // Open file
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL)
    {
        perror("fopen");
        return;
    }

    int curr_block = 1;
    tftp_packet packet;
    int bytes;

    while (1)
    {
        int flag = 0;
        memset(&packet, 0, sizeof(packet));

        packet.opcode = htons(DATA);
        packet.body.data_packet.block_number = htons(curr_block);

        bytes = fread(packet.body.data_packet.data, 1, 512, fptr);

        if (ferror(fptr))
        {
            perror("fread");
            fclose(fptr);
            return;
        }

        if (bytes < 512)
            flag = 1;

        int length = bytes + 4;

        // Send DATA packet
        sendto(sockfd, &packet, length, 0,
               (struct sockaddr *)&client_addr, client_len);

        tftp_packet recv_packet;

        while (1)
        {
            int ret = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0,
                               (struct sockaddr *)&client_addr, &client_len);

            if (ret != 4)
            {
                // Resend DATA packet
                sendto(sockfd, &packet, length, 0,
                       (struct sockaddr *)&client_addr, client_len);
                continue;
            }

            uint16_t ack_opcode = ntohs(recv_packet.opcode);
            uint16_t ack_block =
                ntohs(recv_packet.body.ack_packet.block_number);

            if (ack_opcode == ACK && ack_block == curr_block)
                break;

            // Resend DATA packet if ACK is incorrect
            sendto(sockfd, &packet, length, 0,
                   (struct sockaddr *)&client_addr, client_len);
        }

        if (flag == 1)
        {
            fclose(fptr);
            printf("File sent successfully: %s\n", filename);
            return;
        }

        curr_block++;
    }
}

void receive_file(int sockfd, struct sockaddr_in client_addr,
                  socklen_t client_len, char *filename)
{
    // Open file
    char buffer[30];
    sprintf(buffer,"server_local_%s",filename);
    FILE *fptr = fopen(buffer, "wb");
    if (fptr == NULL)
    {
        perror("fopen");
        return;
    }

    tftp_packet packet;
    int curr_block = 0;

    // Send ACK for block 0
    memset(&packet, 0, sizeof(packet));
    packet.opcode = htons(ACK);
    packet.body.ack_packet.block_number = htons(0);

    sendto(sockfd, &packet, 4, 0,
           (struct sockaddr *)&client_addr, client_len);

    while (1)
    {
        tftp_packet recv_packet;
        int ret;

        while (1)
        {
            ret = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0,
                           (struct sockaddr *)&client_addr, &client_len);

            if (ret < 4)
                continue;

            uint16_t opcode = ntohs(recv_packet.opcode);
            uint16_t block =
                ntohs(recv_packet.body.data_packet.block_number);

            if (opcode == DATA && block == curr_block + 1)
                break;
        }

        // Write received data to file
        int data_size = ret - 4;

        if (fwrite(recv_packet.body.data_packet.data, 1,
                   data_size, fptr) != (size_t)data_size)
        {
            perror("fwrite");
            fclose(fptr);
            return;
        }

        curr_block++;

        // Send ACK for received block
        memset(&packet, 0, sizeof(packet));
        packet.opcode = htons(ACK);
        packet.body.ack_packet.block_number = htons(curr_block);

        sendto(sockfd, &packet, 4, 0,
               (struct sockaddr *)&client_addr, client_len);

        // Check final DATA packet
        if (ret < 516)
        {
            fclose(fptr);
            printf("File received successfully: %s\n", filename);
            return;
        }
    }
}
