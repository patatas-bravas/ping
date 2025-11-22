#include <float.h>
#include <stdio.h>
#include <string.h>

#include "ping.h"

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: usage error: Destination address required\n");
    return 0;
  }

  ping_stats stats;
  memset(&stats, 0, sizeof(stats));
  stats.hostname = argv[argc - 1];
  stats.rtt.min = FLT_MAX;
  stats.rtt.max = FLT_MIN;

  struct sockaddr_in addr_dest;
  if (dns_resolver(stats.hostname, stats.ipname, &addr_dest) == FATAL_ERR)
    return 1;

  socket_t fd = init_icmp_socket();
  if (fd == -1)
    return 2;

  ping(fd, &addr_dest, &stats);

  close(fd);
}
