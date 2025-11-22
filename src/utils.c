#include "utils.h"
#include <math.h>
#include <stddef.h>

double diff_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

void update_rtt(rtt *rtt, double latency) {
  static size_t count;
  static double sum_of_square_delta;
  count++;
  if (rtt->min > latency)
    rtt->min = latency;
  if (rtt->max < latency)
    rtt->max = latency;

  double delta = latency - rtt->average;
  rtt->average += delta / count;
  double delta2 = latency - rtt->average;
  sum_of_square_delta += delta * delta2;
  rtt->stddev = sqrt(sum_of_square_delta / count);
}
