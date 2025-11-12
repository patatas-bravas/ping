#pragma once

#include <stdbool.h>
#include <netinet/in.h>

#include "printer.h"
#include "types.h"
#include "utils.h"

int init_icmp_socket();
bool dns_resolver(char *hostname, char *ipname, struct sockaddr_in *dest_addr);
bool send_pkt(int socket_fd, struct sockaddr_in *dest_addr, ping_stats *data);
bool recv_pkt(int socket_fd, ping_stats *data);
void sigint_handler(int);
bool ping(int socket_fd, struct sockaddr_in *dest_addr, ping_stats *data);
