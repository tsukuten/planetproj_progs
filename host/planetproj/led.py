#!/usr/bin/env python

from __future__ import print_function, unicode_literals
import sys
from . import planetproj
#import Adafruit_GPIO.I2C as I2C


class LED(planetproj.PlanetProj):

    cur_brightness = None

    def __init__(self, addrs = [planetproj.ADDR_LED_1, planetproj.ADDR_LED_2],
            num_leds_per_dev = 6, dry_run = False):

        assert(len(addrs) != 0)
        self.dry_run = dry_run
        self.num_devs = len(addrs)
        self.addrs = addrs
        self.num_leds_per_dev = num_leds_per_dev
        self.cur_brightness = \
                [0 for i in range(self.num_devs * self.num_leds_per_dev)]
        if self.dry_run:
            print('Running in dry-run mode')
            return
        self.i2c = planetproj.I2C()
        #self.i2cs[i] = I2C.get_i2c_device(addrs[i])

    def set_brightness_multi(self, t):
        ds = [[] for i in range(self.num_devs)]
        for (led, brightness) in t:
            assert(brightness <= 0xff)
            self.cur_brightness[led] = brightness
            ds[led // self.num_leds_per_dev].extend(
                    [led % self.num_leds_per_dev, brightness])
        for i in range(self.num_devs):
            if len(ds[i]) > 0:
                self._write_with_cs(i, planetproj.CMD_SET_BRIGHTNESS, ds[i])

    def set_brightness(self, led, brightness):
        self.set_brightness_multi((led, brightness))


def main():
    assert(len(sys.argv) > 1)
    assert(len(sys.argv) % 2 == 1)
    a = []
    for i in range((len(sys.argv) - 1) // 2):
        a.append((int(sys.argv[1+i*2+0]), int(sys.argv[1+i*2+1])))
    led = LED(dry_run = True)
    led.set_brightness_multi(a)

if __name__ == '__main__':
    main()
