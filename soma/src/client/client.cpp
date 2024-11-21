#include <cstdint>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctime>
#include <chrono>

#include "../shared/broadcast.h"
#include "../shared/types.h"

#define BROADCAST_IP "255.255.255.255"
#define IP_SIZE 17
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char serverIP[IP_SIZE];

    if (argc < 2) {
      std::cerr << "Please send the port your want to run on";
      return -1;
    }

    int port = atoi(argv[1]);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Set the broadcast IP address
    if (inet_pton(AF_INET, BROADCAST_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid broadcast IP address\n";
        close(sockfd);
        return -1;
    }
    
    if (allowBroadcast(&sockfd, &server_addr, serverIP) < 0) {
        std::cerr << "client broadcast failed.\n";
        return -1;
    }

    // set timeouts
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        std::cerr << "setsockopt failed\n";

    //if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    //    std::cerr << "setsockopt failed\n";
    //************

    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::cout << "server_addr " << serverIP << " " << std::ctime(&current_time); 

    requisicao req;
    uint16_t seqn = 0;
    uint16_t total_sum = 0;
    packet req_pack;
    packet res_pack;
    while (true) {
      std::cin >> req.value;
      seqn++;
      //if (req.value == 9) seqn = 10; // force sequence error for testing
      total_sum += req.value;

      req_pack.type = REQ;
      req_pack.seqn = seqn;
      req_pack.req = req;

      socklen_t addr_len = sizeof(server_addr);
      int response_size;
      do {
        // send sum request 
        if (sendto(sockfd, &req_pack, sizeof(req_pack), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
          std::cerr << "Failed to send message\n";
          close(sockfd);
          return -1;
        }
        // get response from server
        response_size = recvfrom(sockfd, &res_pack, sizeof(res_pack), 0, (struct sockaddr*)&server_addr, &addr_len);
        if (response_size > 0 && res_pack.type == REQ_ACK) {
            now = std::chrono::system_clock::now();
            current_time = std::chrono::system_clock::to_time_t(now);
            std::cout << std::ctime(&current_time) << "server " << inet_ntoa(server_addr.sin_addr) << " id_req " << seqn
                  << " value " << req.value << " num_reqs " << res_pack.ack.seqn << " total sum " << res_pack.ack.total_sum << std::endl;
        }
      } while (response_size < 0);
    }

    close(sockfd);
    return 0;
}
