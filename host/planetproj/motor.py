#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from math import pi
from . import planetproj


class Motor(planetproj.PlanetProj):

    def __init__(self,
            addrs = [planetproj.ADDR_MOTOR_1, planetproj.ADDR_MOTOR_2],
            degrees_per_step = 1.8 * (pi / 180),
            reduction_ratios = [100, 10], dry_run = False):
        assert(len(addrs) != 0)
        self.dry_run = dry_run
        self.num_devs = len(addrs)
        self.addrs = addrs
        self.degrees_per_step = degrees_per_step
        self.reduction_ratios = reduction_ratios
        self.cur_pos = [0 for i in range(self.num_devs)]
        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()
        # XXX: Setup max_idx and idx_step for all motors.

    def set_power(self, n, power):
        assert(0 <= n < self.num_devs)
        assert(0 <= power <= 1)
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [0, int(round(power * 255))])
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [1, int(round(power * 255))])

    def set_max_idx(self, n, max_idx):
        assert(0 <= n < self.num_devs)
        assert(0 <= max_idx < (1<<16))
        self._write_with_cs(n, planetproj.CMD_SET_MAX_IDX,
                [max_idx & 0xff, max_idx >> 8])

    def set_idx_step(self, n, idx_step):
        assert(0 <= n < self.num_devs)
        assert(1 <= idx_step < (1<<8))
        self._write_with_cs(n, planetproj.CMD_SET_IDX_STEP, [idx_step])

    def set_zero_position(self, n):
        assert(0 <= n < self.num_devs)
        self.cur_pos[n] = 0

    def get_current_degree(self, n):
        assert(0 <= n < self.num_devs)
        return self.cur_pos[n] * self.degrees_per_step \
                / self.reduction_ratios[n]

    def do_rotate_step_relative(self, n, step):
        assert(0 <= n < self.num_devs)
        if step == 0:
            return
        self.cur_pos[n] += step
        if step < 0:
            is_back = 1
            step = -step
        else:
            is_back = 0
        print('Rotating', '-' if is_back else '+', step, 'steps')
        self._write_with_cs(n, planetproj.CMD_SET_ROTATE,
                [is_back, step & 0xff, step >> 8], wait = 1)

    def do_rotate_degree_relative(self, n, degree):
        assert(0 <= n < self.num_devs)
        if degree == 0:
            return
        step = int(round(
                degree * self.reduction_ratios[n] / self.degrees_per_step))
        self.do_rotate_step_relative(n, step)

    def do_rotate_degree_absolute(self, n, degree):
        assert(0 <= n < self.num_devs)
        if degree == 0:
            return
        degree_rel = degree - self.get_current_degree(n)
        self.do_rotate_degree_relative(n, degree_rel)
