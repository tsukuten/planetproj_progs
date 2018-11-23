from __future__ import print_function, unicode_literals


CMD_STATUS = 0x10
STATUS_SUCCESS = 0x00
STATUS_WRONG_CHECKSUM = 0x01
STATUS_UNKNOWN_COMMAND = 0x02
STATUS_NOT_READY = 0x03

CMD_SET_BRIGHTNESS = 0x20

CMD_SET_ROTATE = 0x30
CMD_SET_POWER = 0x31
CMD_SET_MAX_IDX = 0x32
CMD_SET_IDX_STEP = 0x33

ADDR_LED_1 = 0x30
ADDR_LED_2 = 0x31
ADDR_MOTOR_1 = 0x40
ADDR_MOTOR_2 = 0x41


import os
import sys
import time
import fcntl
import binascii


class I2C(object):

    # Derived from linux/i2c-dev.h
    I2C_SLAVE = 0x0703

    def __init__(self, bus = 1):
        self.fd = os.open("/dev/i2c-%u" % bus, os.O_RDWR)

    def set_addr(self, addr):
        fcntl.ioctl(self.fd, self.I2C_SLAVE, addr)

    def write(self, data):
        return os.write(self.fd, bytes(data))

    def read(self, n):
        return list(os.read(self.fd, n))


class PlanetProj(object):

    i2c = None
    addrs = []
    dry_run = False

    def _to_le_array(self, v, n = None):
        assert(v >= 0)
        a = []
        if n is None:
            while v != 0:
                a.append(v & 0xff)
                v >>= 8
        else:
            for i in range(n):
                a.append(v & 0xff)
                v >>= 8
        return a

    def _crc16(self, data, init = 0):
        # crc_hqx is compatible with CRC-XMODEM:
        # - Polynomial is 0x1021.
        # - Initial value is 0.
        return binascii.crc_hqx(bytearray(data), init)

    def _read_and_check_status(self, data = []):
        # xxx: Wait for a full data?
        data[:] = self.i2c.read(2 + 2)
        print('Received data:', data)
        crc = self._crc16(data[:-2])
        crc_rcv = data[-2] | (data[-1] << 8)
        if crc != crc_rcv:
            print('Checksum error: 0x%04x vs. 0x%04x for' % (crc, crc_rcv),
                    data[:-2])
            return (False, None)
        if data[0] != CMD_STATUS:
            print('Slave returned cmd=0x%02x instead of STATUS(0x%02x)' %
                    (data[0], CMD_STATUS))
            return (False, None)
        if data[1] != STATUS_SUCCESS:
            print('Slave status=0x%02x is not SUCCESS(0x%02x)' %
                    (data[1], STATUS_SUCCESS))
        return (True, data[1])

    def _write_with_cs(self, n, register, data, wait = 0.1):
        d = [register]
        d.extend(data)
        d.extend(self._to_le_array(self._crc16(d), n = 2))
        print('Writing to dev %d:' % n, d)
        if self.dry_run:
            return
        self.i2c.set_addr(self.addrs[n])
        self.i2c.write(d)
        # xxx: Set timeout?
        while True:
            time.sleep(wait)
            (succ, stat) = self._read_and_check_status()
            if succ and stat == STATUS_SUCCESS:
                break
            if (not succ) or stat == STATUS_WRONG_CHECKSUM:
                print('Checksum is wrong; re-sending...')
                self.i2c.write(d)
            elif stat == STATUS_UNKNOWN_COMMAND:
                print('Command is not recognized by slave; aborting')
                os.abort()
            elif stat == STATUS_NOT_READY:
                print('Slave is just not ready; waiting...')
            else:
                os.abort()


def main():
    pass

if __name__ == '__main__':
    main()
