#pragma once

#include "printer.h"
#include "types.h"
#include "utils.h"

socket_t init_icmp_socket();
int8_t dns_resolver(const char *hostname, char *ipname, struct sockaddr_in *dest_addr);
int8_t send_pkt(const socket_t fd, struct sockaddr_in *dest_addr, ping_stats *data);
int8_t recv_pkt(const socket_t fd, ping_stats *data);
int8_t handle_icmp_hdr(char *buffer,  ping_stats *stats);
void sigint_handler(int code);
int8_t ping(const socket_t fd, struct sockaddr_in *dest_addr, ping_stats *data);
