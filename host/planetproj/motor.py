#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from math import pi
from . import planetproj


class Motor(planetproj.PlanetProj):

    def __init__(self,
            addrs = [planetproj.ADDR_MOTOR_1, planetproj.ADDR_MOTOR_2],
            degrees_per_step = 1.8 * (pi / 180), dry_run = False):
        assert(len(addrs) != 0)
        self.dry_run = dry_run
        self.num_devs = len(addrs)
        self.addrs = addrs
        self.degrees_per_step = degrees_per_step
        self.cur_pos = [0 for i in range(self.num_devs)]
        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()

    def set_power(self, n, power):
        assert(0 <= power <= 1)
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [0, int(round(power * 255))])
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [1, int(round(power * 255))])

    def set_zero_position(self, n):
        self.cur_pos[n] = 0

    def get_current_degree(self, n):
        return self.cur_pos[n] * self.degrees_per_step

    def do_rotate_step_relative(self, n, step):
        self.cur_pos[n] += step
        if step < 0:
            is_back = 1
            step = -step
        else:
            is_back = 0
        assert(step >= 0)
        print('Rotating', '-' if is_back else '+', step, 'steps')
        self._write_with_cs(n, planetproj.CMD_SET_ROTATE,
                [is_back, step & 0xff, step >> 8], wait = 1)

    def do_rotate_degree_relative(self, n, degree):
        # We don't have to concern reduction ratio here because it's handled by
        # interval_gen.py.
        step = int(round(degree / self.degrees_per_step))
        self.do_rotate_step_relative(n, step)

    def do_rotate_degree_absolute(self, n, degree):
        cur_degree = self.get_current_degree(n)
        step = int(round((degree - cur_degree) / self.degrees_per_step))
        self.do_rotate_step_relative(n, step)


def main():
    motor = Motor(dry_run = True)
    motor.set_power(0, 255)
    motor.set_power(1, 255)
    motor.do_rotate(0, 100)
    motor.do_rotate(1, 100)

if __name__ == '__main__':
    main()
