// Audio stream player (ESP_CMD_AUDIO_*): the host pushes a WAV / MP3 / FLAC /
// Ogg stream chunk by chunk, the ESP decodes it in its own task and adds it to
// the mix of player_task. See doc/esp32-spi-wifi.md, "Audio stream".
#pragma once

#include <stddef.h>
#include "main.h"

void audio_init();

// From the command ISR (command() in spi_slave.cpp)
void audio_command(u8 cmd);
bool audio_push_check(size_t size);   // PUSH: true = start the DMA; false: status set

// From the receiver task: the data of a PUSH is in the DMA buffer
void audio_rx(const u8 *data, size_t size);

// From player_task: add the stream to the mix (stereo float, int16 scale)
void audio_render(float *mix, int samples, int rate);
