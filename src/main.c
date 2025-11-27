#include <float.h>
#include <stdio.h>
#include <string.h>

#include "ping.h"

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: missing host operand\n");
    return 0;
  }

  hostname = argv[argc - 1];
  memset(&rtt, 0, sizeof(rtt));
  memset(&opt, 0, sizeof(ping_opt));
  rtt.min = FLT_MAX;
  rtt.max = FLT_MIN;
  run = 1;
  err = 1;
  send_packet = 0;
  recv_packet = 0;
  bytes_read = 0;

  if (handle_opt(argc, argv) == FATAL_ERR)
    return 1;

  struct sockaddr_in addr_dest;
  if (dns_resolver(&addr_dest) == FATAL_ERR)
    return 2;

  socket_t fd = init_icmp_socket();
  if (fd == FATAL_ERR)
    return 3;

  if (ft_ping(fd, &addr_dest) == FATAL_ERR) {

    close(fd);
    return 4;
  }

  close(fd);
  return 0;
}
