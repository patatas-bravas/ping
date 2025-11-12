#include "utils.h"

double diff_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

uint16_t checksum(void *addr, size_t size) {
  uint16_t *buffer = addr;
  uint32_t sum = 0;

  while (size > 1) {
    sum += *buffer++;
    size -= 2;
  }
  if (size == 1)
    sum += *(uint8_t *)buffer;

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  return (uint16_t)~sum;
}
