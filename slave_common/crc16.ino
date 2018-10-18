#include <stdint.h>
#include <util/crc16.h>

_Bool crc16_check(const uint8_t * const data, const size_t len)
{
  size_t i;
  uint16_t crc = 0;
  for (i = 0; i < len; i ++)
    crc = _crc_xmodem_update(crc, data[i]);
  if (data[len + 0] == (uint8_t) (crc & 0xff)
      && data[len + 1] == (uint8_t) (crc >> 8))
    return !0;
  return 0;
}

void crc16_append(uint8_t * const data, const size_t len)
{
  size_t i;
  uint16_t crc = 0;
  for (i = 0; i < len; i ++)
    crc = _crc_xmodem_update(crc, data[i]);
  data[len + 0] = (uint8_t) (crc & 0xff);
  data[len + 1] = (uint8_t) (crc >> 8);
}
