#ifndef CRC16_H_
#define CRC16_H_

    _Bool crc16_check(const uint8_t * const data, const size_t len);
    void crc16_append(uint8_t * const data, const size_t len);

#endif /* CRC16_H_ */
