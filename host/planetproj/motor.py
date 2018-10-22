#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from . import planetproj


class Motor(planetproj.PlanetProj):

    cur_pos = 0

    def __init__(self,
            addrs = [planetproj.ADDR_MOTOR_1, planetproj.ADDR_MOTOR_2],
            dry_run = False):
        assert(len(addrs) != 0)
        self.dry_run = dry_run
        self.num_devs = len(addrs)
        self.addrs = addrs
        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()

    def do_rotate(self, n, step):
        self.cur_pos += step
        if step < 0:
            is_back = 1
            step = -step
        else:
            is_back = 0
        assert(step >= 0)
        self._write_with_cs(n, planetproj.CMD_SET_ROTATE,
                [is_back, step & 0xff, step >> 8], wait = 1)

    def set_power(self, n, power):
        self._write_with_cs(n, planetproj.CMD_SET_POWER, [0, power])
        self._write_with_cs(n, planetproj.CMD_SET_POWER, [1, power])


def main():
    motor = Motor(dry_run = True)
    motor.set_power(0, 255)
    motor.set_power(1, 255)
    motor.do_rotate(0, 100)
    motor.do_rotate(1, 100)

if __name__ == '__main__':
    main()
