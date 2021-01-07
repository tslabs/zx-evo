
#pragma once

u8 tw_send_start();
void tw_send_stop();
u8 tw_send_addr(u8 addr);
u8 tw_send_data(u8 data);
u8 tw_read_data(u8 *data);
