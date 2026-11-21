#pragma once

#include <stdint.h>

// TCP connect with a timeout (the plain connect() of lwIP blocks for minutes
// on a dead host). Returns a socket with SO_RCVTIMEO / SO_SNDTIMEO of io_ms
// set, or -1; the address must be in network byte order.
int net_connect_timeout(uint32_t addr, uint16_t port, uint32_t connect_ms, uint32_t io_ms);
