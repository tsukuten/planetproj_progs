#!/usr/bin/env python3


T0 = 100e-3
T_min = 2.5e-3
a = 100


import sys

T = T0
Ts = []
while round(T * 1e6) > T_min * 1e6:
    #print(T, '[s]')
    Ts.append(T * 1e6)
    T = 1.0 / (1.0 / T + a * T)
Ts.append(T_min * 1e6)

n = len(Ts)
print('n =', n)
print(n * (32/8) / 1024, 'kB of 32kB')
if n * (32/8) / 1024 > 32:
    print('Too much!')
    exit(1)


print('Writing to interval_table_d10.h and interval_table_d100.h')
for (d, m) in [(10, 10), (100, 1)]:
    sys.stdout = open('interval_table_d%d.h' % d, 'w')
    print('#ifndef INTERVAL_TABLE_D%d_H_' % d)
    print('#define INTERVAL_TABLE_D%d_H_' % d)
    print()
    print('#include <stdint.h>')
    print('#include <avr/pgmspace.h>')
    print()
    print('static const uint32_t interval_table[%d] PROGMEM = {' % n)
    for T in Ts:
        print('    %d,' % round(T * m))
    print('};')
    print()
    print('#endif /* INTERVAL_TABLE_D%d_H_ */' % d)
    sys.stdout.close()
