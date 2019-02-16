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
            reduction_ratios = [10, 100],
            degree_range = [[-pi, pi], [-inf, inf]], dry_run = False):

        num_devs = len(addrs)
        if num_devs < 1:
            raise ValueError('num_devs (%d) is less than 1' % num_devs)
        if len(reduction_ratios) != num_devs:
            raise ValueError('reduction_ratios is not specified for all slaves')
        if len(degree_range) != num_devs:
            raise ValueError('degree_range is not specified for all slaves')
        for t in degree_range:
            if len(t) != 2 or not(t[0] <= 0 <= t[1]):
                raise ValueError('degree_range is ill-formatted')

        self.dry_run = dry_run
        self.num_devs = num_devs
        self.addrs = addrs
        self.degrees_per_step = degrees_per_step
        self.reduction_ratios = reduction_ratios
        self.cur_pos = [0 for i in range(self.num_devs)]

        a = []
        for i in range(self.num_devs):
            a.append([])
            for v in degree_range[i]:
                a[i].append(self._degree_to_step(
                        v * self.reduction_ratios[i], conv = lambda x: x))
        self.step_range = a

        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()
        # XXX: Setup max_idx and idx_step for all motors.

    def set_power(self, n, power):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        if not(0 <= power <= 1):
            raise ValueError('power is not in range: [0, 1]')
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [0, int(round(power * 255))])
        self._write_with_cs(n, planetproj.CMD_SET_POWER,
                [1, int(round(power * 255))])

    def set_max_idx(self, n, max_idx):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        if not(0 <= max_idx < (1<<16)):
            raise ValueError('max_idx is not an unsigned 16-bit integer')
        self._write_with_cs(n, planetproj.CMD_SET_MAX_IDX,
                [max_idx & 0xff, max_idx >> 8])

    def set_idx_step(self, n, idx_step):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        if not(1 <= idx_step < (1<<8)):
            raise ValueError('idx_step is not an unsigned 8-bit integer')
        self._write_with_cs(n, planetproj.CMD_SET_IDX_STEP, [idx_step])

    def set_drive_mode(self, n, mode):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        if not(1 <= mode < (1<<8)):
            raise ValueError('mode is not an unsigned 8-bit integer')
        self._write_with_cs(n, planetproj.CMD_SET_DRIVE_MODE, [mode])

    def set_zero_position(self, n):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        self.cur_pos[n] = 0

    def get_current_degree(self, n):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        return self._step_to_degree(self.cur_pos[n]) / self.reduction_ratios[n]

    def do_rotate_step_relative(self, n, step):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
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
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        if degree == 0:
            return
        step = self._degree_to_step(degree * self.reduction_ratios[n])
        self.do_rotate_step_relative(n, step)

    def do_rotate_degree_absolute(self, n, degree):
        if not(0 <= n < self.num_devs):
            raise ValueError('slave number is not in range: [0,%d)' %
                    self.num_devs)
        degree_rel = degree - self.get_current_degree(n)
        if degree_rel == 0:
            return
        self.do_rotate_degree_relative(n, degree_rel)
