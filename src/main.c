#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

#include "ping.h"

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: usage error: Destination address required\n");
    return 0;
  }

  char *hostname = argv[argc - 1];
  char ipname[INET_ADDRSTRLEN];
  struct sockaddr_in addr_dest;
  if (dns_resolver(hostname, ipname, &addr_dest) == false)
    return 1;

  int socket_fd = init_icmp_socket();
  if (socket_fd == -1)
    return 2;

  ping_stats data = {.hostname = hostname,
                     .ipname = ipname,
                     .bytes_read = 0,
                     .sequence = 0,
                     .ttl = 0,
                     .pkt_size = ICMP_HEADER_SIZE + ICMP_PAYLOAD_SIZE + IP_HEADER_SIZE};

  ping(socket_fd, &addr_dest, &data);

  close(socket_fd);
}
