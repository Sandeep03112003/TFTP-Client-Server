#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tftp.h"


void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;


    // Create UDP socket
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd==-1)
    {
         perror("socket");
         return -1;
    }
    // Set socket timeout option
    struct timeval timeout;
    timeout.tv_sec=TIMEOUT_SEC;
    timeout.tv_usec=0;
    //TODO Use setsockopt() to set timeout option

    if(setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1)
    {
         perror("setsockopt");
         close(sockfd);
         return -1;
    }
    // Set up server address
   memset(&server_addr,0,sizeof(server_addr));
   server_addr.sin_family=AF_INET;
   server_addr.sin_port=htons(PORT);
   server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    // Bind the socket
    if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1)
    {
         perror("bind");
         close(sockfd);
         return -1;
    }
    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while (1) {
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
          //  perror("Receive failed or timeout occurred");
            continue;
        }
         printf("Client connected: %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
         handle_client(sockfd, client_addr, client_len, &packet);
    }

    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    // Extract the TFTP operation (read or write) from the received packet
     // Convert opcode from network byte order
    uint16_t opcode = ntohs(packet->opcode);
   
    if (opcode == RRQ)
    {
        send_file(sockfd, client_addr, client_len,
                  packet->body.request.filename);
    }
    else if (opcode == WRQ)
    {
        receive_file(sockfd, client_addr, client_len,
                     packet->body.request.filename);
    }
    else
    {
        printf("Invalid Request\n");
    }
    
}




