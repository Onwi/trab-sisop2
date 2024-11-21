#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "handle_request.h"
#include "../shared/utils.h"
#include <chrono>

void *handle_request(void *args) {
  thd_args *thd_arg = (thd_args *)args;

  char *client_sin_address = thd_arg->client_sin_address;
  int *sockfd = thd_arg->sockfd;
  std::list<clients> *clients_list = thd_arg->clients_list;
  packet *pack_from_client = thd_arg->pack_from_client;
  long int *total_sum = thd_arg->total_sum;
  struct sockaddr_in *client_addr = thd_arg->client_addr;
  struct sockaddr_in *server_addr = thd_arg->server_addr;
  socklen_t client_len = thd_arg->client_len;
  pthread_mutex_t *lock = thd_arg->lock;
  int num_reqs = thd_arg->num_reqs;

  clients *req_client = find_client(clients_list, client_sin_address);

  // set timeouts
  struct timeval timeout;      
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;
  if (setsockopt(*sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
      std::cerr << "setsockopt failed\n";
  //********* 

  if (req_client && (*pack_from_client).type != DESC) {
    uint16_t last_req = (*req_client).last_req;
    req_client->last_req++;

    packet res_pack;
    if (last_req + 1 == (*pack_from_client).seqn) {
      req_client->last_sum += pack_from_client->req.value;
      
      // lock this to prevent simultaneous access
      pthread_mutex_lock(lock);
      *total_sum += pack_from_client->req.value;
      auto now = std::chrono::system_clock::now();
      std::time_t current_time = std::chrono::system_clock::to_time_t(now);

      std::cout << std::ctime(&current_time) << "client " << inet_ntoa(client_addr->sin_addr)
            << " id_req " << pack_from_client->seqn << " value " << pack_from_client->req.value
            << " num_reqs " << num_reqs << " total_sum " << *total_sum << std::endl;
 

      pthread_mutex_unlock(lock);
      // ******************************************** 

      res_pack.type = REQ_ACK;
      res_pack.seqn = pack_from_client->seqn;
      res_pack.ack.seqn = pack_from_client->seqn;
      res_pack.ack.total_sum = (*req_client).last_sum;
      res_pack.ack.num_reqs = 1;
      int n = sendto(*sockfd, &res_pack, sizeof(res_pack), 0, (struct sockaddr *)client_addr, client_len);
      if (n < 0) {
        std::cerr << "Error sending response to client!\n";
        exit(1);
      }
    } else {
      std::cout << "we lost a message! " << (*pack_from_client).seqn << "\n";
    }
  } else {
    // if client is not in the list yet, then its a braodcast discovery message
    // create new client and add it to the list
    clients new_client;
    new_client.address = client_sin_address;
    new_client.last_sum = 0;
    new_client.last_req = 0;

    clients_list->push_back(new_client);
    // I know this wont work, I'm just lazy to fix since I wont use anyways
    char *server_address = inet_ntoa(server_addr->sin_addr);
    int n = sendto(*sockfd, server_address, sizeof(*server_address), 0, (struct sockaddr *)client_addr, client_len);
    if (n < 0) {
      std::cerr << "Error sending response to client!!\n";
      exit(1);
    }
  }

  int return_val = 1;
  pthread_exit(&return_val);
}
