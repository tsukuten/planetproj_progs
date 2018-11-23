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
$ pip install .
```

### Slave

```
$ cd slave_xxx/
$ platformio run
$ platformio run --target=upload
```


## How to use

See [`host/planetproj.md`](host/planetproj.md) for usage.
