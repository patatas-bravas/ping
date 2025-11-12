#pragma once
#include <stdio.h>

#include "types.h"

void print_header_ping(ping_stats *data);
void print_body_ping(ping_stats *data, float time);
void print_footer_ping(ping_stats *data, double time);
