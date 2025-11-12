#pragma once

#include <stdint.h>
#include <unistd.h>
#include <time.h>

double diff_ms(struct timespec start, struct timespec end);
uint16_t checksum(void *addr, size_t size);
