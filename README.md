# planetproj_progs

Host and slave program files for the new planetarium projector.


## Installing

```
$ git clone https://github.com/tsukuten/planetproj_progs
$ cd planetproj_progs/
```

### Host

```
$ cd host/
$ pip install -r requirements.txt
$ python setup.py build
$ sudo python setup.py install
```

### Slave

Open `slave_led/` or `slave_motor/` in Arduino IDE and burn them, though you
don't normally need to do this.


## How to use

See [`host/planetproj.md`](host/planetproj.md) for usage.
