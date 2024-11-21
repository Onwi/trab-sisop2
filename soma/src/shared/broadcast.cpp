#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "broadcast.h"
#include "types.h"

#define BUFFER_SIZE 17

int allowBroadcast(int *sockfd, struct sockaddr_in *server_addr, char buffer[]) {
  packet pack;
  pack.type = DESC;
  
    int broadcast = 1;
    if (setsockopt((*sockfd), SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "Failed to set socket options\n";
        close((*sockfd));
        return -1;
    }

    // Send a broadcast message to the server
    if (sendto((*sockfd), &pack, sizeof(pack), 0, (struct sockaddr*)server_addr, sizeof((*server_addr))) < 0) {
        std::cerr << "Failed to send message\n";
        close((*sockfd));
        return -1;
    }

    std::cout << "Broadcast message sent\n";

    // wait for response from server, giving the ip address
    socklen_t addr_len = sizeof((*server_addr));
    int recv_len = recvfrom((*sockfd), buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        std::cout << "Received response from server " << buffer << "\n";
    } else {
      std::cerr << "Error getting server ip address" << std::endl;
      exit(1);
    }
    
    return 1;
}
