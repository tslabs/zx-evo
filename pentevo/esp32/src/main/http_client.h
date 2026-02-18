#pragma once

#include <stdint.h>
#include <stdbool.h>

void http_init(void);
void http_do_get(void);

// Streaming API
void http_stream_read_task(void);  // Вызывается из helper task
void http_stream_delete(uint8_t handle);
bool http_is_stream_handle(uint8_t handle);
void http_stream_read(uint8_t handle, uint32_t offset, uint32_t size);  // legacy
