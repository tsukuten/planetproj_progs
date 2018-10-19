from __future__ import print_function, unicode_literals


CMD_STATUS = 0x10
STATUS_SUCCESS = 0x00
STATUS_WRONG_CHECKSUM = 0x01
STATUS_UNKNOWN_COMMAND = 0x02

CMD_SET_BRIGHTNESS = 0x20

CMD_SET_ROTATE = 0x30
CMD_SET_POWER = 0x31

ADDR_LED_1 = 0x30
ADDR_LED_2 = 0x31
ADDR_MOTOR_1 = 0x40
ADDR_MOTOR_2 = 0x41


import os
import sys
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
        return os.write(self.fd, data)

    def read(self, n):
        return os.read(self.fd, n)


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
        data[:] = i2c.read(2 + 2)
        crc = self._crc16(data[:-2])
        crc_rcv = data[-2] | (data[-1] << 8)
        if crc != crc_rsv:
            print('Checksum error: 0x04x vs. 0x%04x for' % (crc, crc_rsv),
                    data[:-2])
            return False
        if data[0] != CMD_STATUS:
            print('Slave returned cmd=0x%02x instead of STATUS(0x%02x)' %
                    (data[0], CMD_STATUS))
            return False
        if data[1] != STATUS_SUCCESS:
            print('Slave status=0x%02x is not SUCCESS(0x%02x)' %
                    (data[1], STATUS_SUCCESS))
            return False
        return True

    def _write_with_cs(self, n, register, data):
        d = [register]
        d.extend(data)
        d.extend(self._to_le_array(self._crc16(data), n = 2))
        print('Writing to dev %d:' % n, d)
        if self.dry_run:
            return
        self.i2c.set_addr(self.addrs[n])
        while True:
            self.i2c.write(d)
            # xxx: Set timeout?
            if _read_and_check_status():
                break
            print('Status is not success; retrying...', file = sys.stderr)


def main():
    pass

if __name__ == '__main__':
    main()
