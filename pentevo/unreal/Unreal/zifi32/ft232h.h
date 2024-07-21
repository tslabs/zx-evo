
#include <stdint.h>

namespace spi
{
  int open();
  void close();
  int xfer(uint8_t *wr_buf, uint8_t *rd_buf, int len);
  int xfer_byte(uint8_t &wr, uint8_t &rd);
  int set_ss(bool is_ss);
};
