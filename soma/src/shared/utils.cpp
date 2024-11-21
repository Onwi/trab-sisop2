#include "utils.h"

clients *find_client(std::list<clients> *clients_list, char *client_ip) {
  for (auto &i : *clients_list) {
    if (i.address == client_ip) {
      std::cout << "found client with client address: " << i.address << std::endl;
      std::cout << "found client with last req: " << i.last_req << std::endl;

      return &i;
    }
  }
  return NULL;
}
