#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from . import planetproj


class LED(planetproj.PlanetProj):

    cur_brightness = None

    def __init__(self, addrs = [planetproj.ADDR_LED_1, planetproj.ADDR_LED_2],
            num_leds_per_dev = 6, dry_run = False):

        num_devs = len(addrs)
        if num_devs < 1:
            raise ValueError('num_devs (%d) is less than 1' % num_devs)

        self.dry_run = dry_run
        self.num_devs = num_devs
        self.addrs = addrs
        self.num_leds_per_dev = num_leds_per_dev
        self.cur_brightness = \
                [0 for i in range(self.num_devs * self.num_leds_per_dev)]
        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()

    def get_brightness(self, led):
        return self.cur_brightness[led]

    def set_brightness_multi(self, t):
        ds = [[] for i in range(self.num_devs)]
        for (led, brightness) in t:
            if not(0 <= brightness <= 1):
                raise ValueError('brightness is not in range: [0,1]')
            self.cur_brightness[led] = brightness
            ds[led // self.num_leds_per_dev].extend(
                    [led % self.num_leds_per_dev, int(round(brightness * 255))])
        for i in range(self.num_devs):
            if len(ds[i]) > 0:
                self._write_with_cs(i, planetproj.CMD_SET_BRIGHTNESS, ds[i])
