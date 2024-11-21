#include <iostream>
#include <cstring>
#include <ostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <list>
#include <pthread.h>
#include <chrono>

#include "../shared/broadcast.h"
#include "../shared/utils.h"
#include "handle_request.h"

#define BUFFER_SIZE 1024

long int total_sum = 0;
pthread_mutex_t lock; 

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    int num_reqs = 0;

    if (argc < 2) {
      std::cerr << "Please send the port your want to run on";
      return -1;
    }

    int port = atoi(argv[1]);
    
    if (pthread_mutex_init(&lock, NULL) != 0) { 
      std::cerr << "Mutex init has failed!\n"; 
      return -1; 
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      std::cerr << "Socket creation failed\n";
      return -1;
    }

    // allow socket to receive broadcast messages
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
      std::cerr << "Failed to set socket options\n";
      close(sockfd);
      return -1;
    }
    
    // set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket to address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed\n";
        close(sockfd);
        return -1;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);

    std::list<clients> clients_list;
    uint16_t total_reqs = 0;
    packet pack_from_client;
    while (true) {
        socklen_t client_len = sizeof(client_addr);
        memset(buffer, 0, BUFFER_SIZE);

        // receive message from client
        int recv_len = recvfrom(sockfd, &pack_from_client, sizeof(pack_from_client), 0, (struct sockaddr*)&client_addr, &client_len);
        if (recv_len < 0) {
            std::cerr << "Failed to receive message\n";
            break;
        } else {
          
          pthread_t thd;
          total_reqs++;
          if (pack_from_client.type != DESC) {
            num_reqs++;
          }

          thd_args args;
          args.server_addr = &server_addr;
          args.client_addr = &client_addr;
          args.client_len = client_len;
          args.client_sin_address = inet_ntoa(client_addr.sin_addr);
          args.clients_list = &clients_list;
          args.pack_from_client = &pack_from_client;
          args.total_sum = &total_sum;
          args.sockfd = &sockfd;
          args.lock = &lock;
          args.num_reqs = num_reqs;

          // trigger a thread for each client request
          if (pthread_create(&thd, NULL, handle_request, &args) != 0) {
            std::cerr << "failed to trigger thread!!";
          }
          pthread_join(thd, NULL);
        }
    }

    close(sockfd);
    return 0;
}

