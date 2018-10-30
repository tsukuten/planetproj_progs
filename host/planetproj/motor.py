#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from math import pi
from . import planetproj

# For Python 2.x...
inf = float('inf')


class Motor(planetproj.PlanetProj):

    def _degree_to_step(self, degree, conv = lambda x: int(round(x))):
        return conv(degree / self.degrees_per_step)

    def _step_to_degree(self, step):
        return step * self.degrees_per_step

    def __init__(self,
            addrs = [planetproj.ADDR_MOTOR_1, planetproj.ADDR_MOTOR_2],
            degrees_per_step = 1.8 * (pi / 180),
            degree_range = [[-pi, pi], [-inf, inf]], dry_run = False):
        assert(len(addrs) != 0)
        self.dry_run = dry_run
        self.num_devs = len(addrs)
        self.addrs = addrs
        self.degrees_per_step = degrees_per_step
        self.cur_pos = [0 for i in range(self.num_devs)]

        for t in degree_range:
            assert(len(t) == 2)
            assert(t[0] <= 0 <= t[1])
        a = []
        for i in range(len(degree_range)):
            a.append([])
            for v in degree_range[i]:
                a[i].append(self._degree_to_step(v, conv = lambda x: x))
        self.step_range = a

        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()

    def set_power(self, n, power):
        assert(0 <= n < self.num_devs)
        assert(0 <= power <= 1)
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [0, int(round(power * 255))])
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [1, int(round(power * 255))])

    def set_zero_position(self, n):
        assert(0 <= n < self.num_devs)
        self.cur_pos[n] = 0

    def get_current_degree(self, n):
        assert(0 <= n < self.num_devs)
        return self._step_to_degree(self.cur_pos[n])

    def do_rotate_step_relative(self, n, step):
        assert(0 <= n < self.num_devs)
        if step == 0:
            return
        if not (self.step_range[n][0] <= self.cur_pos[n] + step <=
                self.step_range[n][1]):
            raise ValueError(
                    'Too many steps=%d for cur_pos[%d]=%d (range is [%f, %f])' %
                    (step, n, self.cur_pos[n],
                    self.step_range[n][0], self.step_range[n][1]))
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
        # We don't have to concern reduction ratio here because it's handled by
        # interval_gen.py.
        step = self._degree_to_step(degree)
        self.do_rotate_step_relative(n, step)

    def do_rotate_degree_absolute(self, n, degree):
        assert(0 <= n < self.num_devs)
        cur_degree = self.get_current_degree(n)
        step = self._degree_to_step(degree - cur_degree)
        self.do_rotate_step_relative(n, step)
