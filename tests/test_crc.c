#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint16_t crc_xmodem_update(uint16_t crc, uint8_t data)
{
    int i;

    crc = crc ^ ((uint16_t) data << 8);

    for (i = 0; i < 8; i ++) {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }

    return crc;
}

int main(int argc, char *argv[])
{
    int i;
    uint16_t crc = 0;

    if (argc <= 1) {
        fprintf(stderr, "Usage: %s values...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < argc; i ++) {
        uint8_t v = strtoul(argv[i], NULL, 0);
        crc = crc_xmodem_update(crc, v);
    }

    printf("CRC is\n");
    printf("  in hex:               0x%04x\n", crc);
    printf("  in little-endian hex: 0x%02x 0x%02x\n", crc & 0xff, crc >> 8);
    printf("  in dec:               %u\n", crc);
    printf("  in little-endian dec: %u %u\n", crc & 0xff, crc >> 8);

    return 0;
}
