#pragma once

#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "types.h"

double diff_ms(struct timespec start, struct timespec end);
void update_rtt(rtt *rtt, double latency);
