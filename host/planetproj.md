# プラネタリウム (v2) API一覧

## `class LED`

### `set_brightness_multi(t)`
- `t`: [(led*1*, brightness*1*), ..., (led*n* , brightness*n*)]
    - led*n*: *int* *[0,11]*
    - brightness*n*: *float* *[0,1]*
    - *n*個目のLED (led*n*) の明るさをbrightness*n*に設定する。

### `get_brightness(led)`
- `led`: *int* *[0,11]*
- *->* *float* *[0,1]*
- *led*個目のLEDの明るさを返す。

### 例

```python
#!/usr/bin/env python

import planetproj

LED = planetproj.LED()
LED.set_brightness_multi([(9, 0.8)])
LED.set_brightness_multi([(0, 0.2), (1, 0.53), (2, 0.45), (8, 0.9), (11, 1)])
print(LED.get_brightness(2))
```


## `class Motor`

### `set_power(n, power)`
- `n`: *int* *[0,1]*
- `power`: *float* *[0, 1]*
- モータ*n*の出力を最大値の*power*倍にする。

### `set_zero_position(n)`
- `n`: *int* *[0,1]*
- モーター*n*のゼロ点を現在位置とする。

### `get_current_degree(n)`
- `n`: *int* *[0,1]*
- *->* *float*
- モーター*n*の現在位置 (単位:ラジアン) を返す。

### `do_rotate_step_relative(n, relative_step)`
- `n`: *int* *[0,1]*
- `relative_step`: *int*
- モーター*n*を現在位置から相対的に*relative_step*ステップ回転させる。
- *step*が正なら正回転、負なら逆回転となる。
- _**回転が完了するまで返らない**_。

### `do_rotate_degree_relative(n, relative_degree)`
- `n`: *int* *[0,1]*
- `relative_degree`: *float*
- モーター*n*を現在位置から相対的に*relative_degree*ラジアン回転させる。
- _**回転が完了するまで返らない**_。

### `do_rotate_degree_absolute(n, absolute_degree)`
- `n`: *int* *[0,1]*
- `absolute_degree`: *float*
- モーター*n*を*absolute_degree*ラジアンの位置まで回転させる。
- _**回転が完了するまで返らない**_。

### 例

```python
#!/usr/bin/env python

import planetproj
from math import pi

Motor = planetproj.Motor()
Motor.set_power(0, 1)
Motor.set_power(1, 0.8)
Motor.do_rotate_degree_relative(0, 2*pi)
Motor.do_rotate_degree_relative(1, -pi)
print(Motor.get_current_degree(0))
Motor.set_zero_position(0)
Motor.do_rotate_degree_absolute(0, 0)
Motor.do_rotate_degree_absolute(1, pi/4)
print(Motor.get_current_degree(0))
```
