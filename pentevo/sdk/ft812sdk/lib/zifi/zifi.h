
#pragma once

bool zifi_set_ap(const char*, const char*);  // args:    AP name (ASCII)
                                             //          AP password (ASCII)
                                             // return:  command result (0 - errror, 1 - success)

u8 zifi_input_buffer_used();                 // return:  number of bytes available for the first burst, actual number in the input buffer can be larger

u16 zifi_receive(void*, u16);                // args:    pointer to store received data
                                             //          max number of bytes to receive
                                             // return:  number of bytes received

void zifi_send(void*, u16);                  // args:    pointer to store received data
                                             //          number of bytes to send

bool zifi_is_connected();

bool zifi_connect();

bool zifi_disconnect();

bool zifi_get_fw_version(char*);
