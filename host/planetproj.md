# プラネタリウム (v2) API一覧

## `class LED`

### `set_brightness_multi(t)`
- `t`: [(led*1*, brightness*1*), ..., (led*n* , brightness*n*)]
    - led*n*: *[0,11]*
    - brightness*n*: *[0,255]*
    - *n*個目のLED (led*n*) の明るさをbrightness*n*に設定する。

### `set_brightness(led, brightness)`
- `led`: *[0,11]*
- `brightness`: *[0,255]*
- LED (led) の明るさをbrightnessに設定する。

### 例

```python
#!/usr/bin/env python

import planetproj

LED = planetproj.LED()
LED.set_brightness_multi([(0, 20), (1, 30), (2, 56), (8, 213), (11,255)])
LED.set_brightness(9, 123)
```


## `class Motor`

### `set_power(n, power)`
- `n`: *[0,1]*
- `power`: *[0, 255]*
- モータ*n*の出力を最大値の*power / 255*倍にする。

### `do_rotate(n, step)`
- `n`: *[0,1]*
- `step`: *int*
- モーター*n*を*step*ステップ回転させる。
- *step*が正なら正回転、負なら逆回転となる。
- _**回転が完了するまで返らない**_。

### 例

```python
#!/usr/bin/env python

import planetproj

Motor = planetproj.Motor()
Motor.set_power(0, 255)
Motor.set_power(1, 234)
Motor.do_rotate(0, 1234)
Motor.do_rotate(1, -5678)
```
