#include <stdarg.h>
#include <avr/io.h>
#include <avr/cpufunc.h>
#include <Wire.h>
#include "crc16.h"
#include "planetproj.h"
#include "interval_table.h"

#define PIN_DB0 A0
#define PIN_DB1 A1
#define PIN_DB2 A2
#define PIN_DB3 A3

/*
 * PK267JDA is 1.8 degree/step.  So the maximum number of steps to round one
 * time is: 360/1.8*100=20000, so 16-bit number is required for step.
 */

#if 1
#define ADDR ADDR_MOTOR_1
#define PIN_n_PS_ON 8
#else
#define ADDR ADDR_MOTOR_2
#endif

#define PIN_PWM_A 9
#define PIN_PWM_B 10

/* PIN_{coil}_{polar}_{channel} */
#define PIN_A_POS_P 3
#define PIN_A_POS_N 2
#define PIN_A_NEG_P 1
#define PIN_A_NEG_N 0
#define PIN_B_POS_P 4
#define PIN_B_POS_N 5
#define PIN_B_NEG_P 6
#define PIN_B_NEG_N 7

#define MOTOR_COIL_A 0
#define MOTOR_COIL_B 1

#define MOTOR_POLAR_POS 0
#define MOTOR_POLAR_NEG 1

#define MOTOR_PULSE_POS 0
#define MOTOR_PULSE_NEG 1
#define MOTOR_PULSE_NTL 2

static inline uint8_t motor_set_pulse_core(const unsigned coil,
    const unsigned polar, const unsigned pulse)
{
#define PIN_P ((coil == MOTOR_COIL_A) ? \
    ((polar == MOTOR_POLAR_POS) ? PIN_A_POS_P : PIN_A_NEG_P) : \
    ((polar == MOTOR_POLAR_POS) ? PIN_B_POS_P : PIN_B_NEG_P))
#define PIN_N ((coil == MOTOR_COIL_A) ? \
    ((polar == MOTOR_POLAR_POS) ? PIN_A_POS_N : PIN_A_NEG_N) : \
    ((polar == MOTOR_POLAR_POS) ? PIN_B_POS_N : PIN_B_NEG_N))

  /*
   * pin_p: Pull to HIGH to turn on.
   * pin_n: Pull to LOW to turn on.
   */
  /*
   * pulse_pos: p=on  n=off
   * pulse_neg: p=off n=on
   * pulse_ntl: p=off n=off
   */
  return (pulse == MOTOR_PULSE_POS) ? (_BV(PIN_P) | _BV(PIN_N)) :
      ((pulse == MOTOR_PULSE_NEG) ? 0 : _BV(PIN_N));

#undef PIN_P
#undef PIN_N
}

#define motor_set_pulse(apos, aneg, bpos, bneg) \
  do { \
    PORTD = \
          motor_set_pulse_core(MOTOR_COIL_A, MOTOR_POLAR_POS, MOTOR_PULSE_##apos) \
        | motor_set_pulse_core(MOTOR_COIL_A, MOTOR_POLAR_NEG, MOTOR_PULSE_##aneg) \
        | motor_set_pulse_core(MOTOR_COIL_B, MOTOR_POLAR_POS, MOTOR_PULSE_##bpos) \
        | motor_set_pulse_core(MOTOR_COIL_B, MOTOR_POLAR_NEG, MOTOR_PULSE_##bneg); \
  } while (0)

static void do_wave_drive(const _Bool is_back)
{
  const unsigned n = 4;
  static unsigned turn_prev = n - 1;
  unsigned turn_cur;

  if (!is_back)
    turn_cur = (turn_prev + 1) % n;
  else
    turn_cur = (turn_prev + n - 1) % n;

  switch (turn_cur) {
    case 0:
      motor_set_pulse(POS, NEG, NTL, NTL);
      break;
    case 1:
      motor_set_pulse(NTL, NTL, POS, NEG);
      break;
    case 2:
      motor_set_pulse(NEG, POS, NTL, NTL);
      break;
    case 3:
      motor_set_pulse(NTL, NTL, NEG, POS);
      break;
  }

  turn_prev = turn_cur;
}

static void do_full_step_drive(const _Bool is_back)
{
  const unsigned n = 4;
  static unsigned turn_prev = n - 1;
  unsigned turn_cur;

  if (!is_back)
    turn_cur = (turn_prev + 1) % n;
  else
    turn_cur = (turn_prev + n - 1) % n;

  switch (turn_cur) {
    case 0:
      motor_set_pulse(POS, NEG, NEG, POS);
      break;
    case 1:
      motor_set_pulse(POS, NEG, POS, NEG);
      break;
    case 2:
      motor_set_pulse(NEG, POS, POS, NEG);
      break;
    case 3:
      motor_set_pulse(NEG, POS, NEG, POS);
      break;
  }

  turn_prev = turn_cur;
}

static void do_half_step_drive(const _Bool is_back)
{
  const unsigned n = 8;
  static unsigned turn_prev = n - 1;
  unsigned turn_cur;

  if (!is_back)
    turn_cur = (turn_prev + 1) % n;
  else
    turn_cur = (turn_prev + n - 1) % n;

  switch (turn_cur) {
    case 0:
      motor_set_pulse(POS, NEG, NEG, POS);
      break;
    case 1:
      motor_set_pulse(POS, NEG, NTL, NTL);
      break;
    case 2:
      motor_set_pulse(POS, NEG, POS, NEG);
      break;
    case 3:
      motor_set_pulse(NTL, NTL, POS, NEG);
      break;
    case 4:
      motor_set_pulse(NEG, POS, POS, NEG);
      break;
    case 5:
      motor_set_pulse(NEG, POS, NTL, NTL);
      break;
    case 6:
      motor_set_pulse(NEG, POS, NEG, POS);
      break;
    case 7:
      motor_set_pulse(NTL, NTL, NEG, POS);
      break;
  }

  turn_prev = turn_cur;
}

static uint8_t drive_mode = 0;
static _Bool drive_mode_locked = 0;

static inline void do_drive(const _Bool is_back)
{
  drive_mode_locked = !0;
  _MemoryBarrier();

  switch (drive_mode) {
    case 0:
      do_wave_drive(is_back);
      break;
    case 1:
      do_full_step_drive(is_back);
      break;
    case 2:
      do_half_step_drive(is_back);
      break;
  }
}

static uint8_t recvbuf[64];
static uint8_t sendbuf[64];
static size_t sendbuf_count = 0;
static volatile _Bool is_sendbuf_ready = 0;

static volatile _Bool doing_rotate = 0;
static volatile _Bool rotate_is_back = 0;
static volatile uint16_t rotate_step = 0;

static int16_t idx_step = 1;
static int16_t idx_max = sizeof(interval_table) / sizeof(*interval_table) - 1;


static void prepare_sendbuf(const size_t count, ...)
{
  size_t i;
  va_list ap;
  is_sendbuf_ready = 0;
  _MemoryBarrier();
  sendbuf_count = count + 2;
  va_start(ap, count);
  for (i = 0; i < count; i ++)
    sendbuf[i] = va_arg(ap, int);
  crc16_append(sendbuf, count);
  _MemoryBarrier();
  is_sendbuf_ready = !0;
  _MemoryBarrier();
}

static void process_set_rotate(const uint8_t is_back, const uint16_t step)
{
  doing_rotate = !0;
  rotate_is_back = is_back ? !0 : 0;
  rotate_step = step;
}

static void process_set_power(const uint8_t polar, const uint8_t power)
{
  if (polar == 0)
    analogWrite(PIN_PWM_A, power);
  else
    analogWrite(PIN_PWM_B, power);
  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);
}

static void process_set_max_idx(const int16_t v)
{
  const int16_t n = sizeof(interval_table) / sizeof(*interval_table);
  idx_max = max(0, min(v, n-1));
  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);
}

static void process_set_idx_step(const uint8_t v)
{
  idx_step = v;
  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);
}

static void process_set_drive_mode(const uint8_t v)
{
  if (drive_mode_locked) {
    prepare_sendbuf(2, CMD_STATUS, STATUS_INVALID_VALUE);
    return;
  }

  drive_mode = v;
  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);
}

static void callback_receive(const int n)
{
  int i;

  is_sendbuf_ready = 0;
  _MemoryBarrier();

  for (i = 0; i < n; i ++) {
    while (!Wire.available())
      ;
    recvbuf[i] = Wire.read();
  }
  if (!crc16_check(recvbuf, n - 2)) {
    prepare_sendbuf(2, CMD_STATUS, STATUS_WRONG_CHECKSUM);
    return;
  }
  /* If we are rotating, do not prepare sendbuf i.e. do not set is_sendbuf_ready
   * to true, which results in responding with NOT_READY status when requested.
   */
  if (doing_rotate)
    return;

  switch (recvbuf[0]) {
    case CMD_SET_ROTATE:
      process_set_rotate(recvbuf[1],
          ((uint16_t) recvbuf[2]) | (((uint16_t) recvbuf[3]) << 8));
      break;
    case CMD_SET_POWER:
      process_set_power(recvbuf[1], recvbuf[2]);
      break;
    case CMD_SET_MAX_IDX:
      process_set_max_idx(
          ((int16_t) recvbuf[1]) | (((int16_t) recvbuf[2]) << 8));
      break;
    case CMD_SET_IDX_STEP:
      process_set_idx_step(recvbuf[1]);
      break;
    case CMD_SET_DRIVE_MODE:
      process_set_drive_mode(recvbuf[1]);
      break;
    default:
      prepare_sendbuf(2, CMD_STATUS, STATUS_UNKNOWN_COMMAND);
      break;
  }
}

static void callback_request(void)
{
  if (!is_sendbuf_ready) {
    const uint8_t msg[4] = {CMD_STATUS, STATUS_NOT_READY, 0x10, 0x33};
    Wire.write(msg, sizeof(msg));
    return;
  }

  Wire.write(sendbuf, sendbuf_count);

  _MemoryBarrier();
  is_sendbuf_ready = 0;
  sendbuf_count = 0;
}

static void delayMicroseconds_coarse(const uint32_t us)
{
  int32_t c = us >> 7;
  while (c-- > 0)
    _delay_us(1UL << 7);
}

static inline void do_step(const int16_t idx)
{
  PINB = _BV(5);
  delayMicroseconds_coarse(
      pgm_read_dword(&(interval_table[min(idx, idx_max)])));
  do_drive(rotate_is_back);
}

static void ps_init(void)
{
#if ADDR == ADDR_MOTOR_1
  _delay_ms(1000);
  pinMode(PIN_n_PS_ON, OUTPUT);
#endif /* ADDR == ADDR_MOTOR_1 */
}

void setup_debug(void)
{
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PIN_DB0, INPUT_PULLUP);
  pinMode(PIN_DB1, INPUT_PULLUP);
  pinMode(PIN_DB2, INPUT_PULLUP);
  pinMode(PIN_DB3, INPUT_PULLUP);
  const byte db0 = digitalRead(PIN_DB0);
  const byte db1 = digitalRead(PIN_DB1);
  const byte db2 = digitalRead(PIN_DB2);

  drive_mode = (!db0) | ((!db1) << 1);

  if (db2)
    return;

  /* Self operation mode */

  analogWrite(PIN_PWM_A, 255);
  analogWrite(PIN_PWM_B, 255);

  for (; ; ) {
    static int16_t idx = 0;
    static _Bool is_bk = 0;
    const _Bool do_fw = !digitalRead(PIN_DB1);
    const _Bool do_bk = !digitalRead(PIN_DB0);

    if (do_fw) {
      if (!is_bk)
        idx ++;
      else { /* do_fw while is_bk */
        if (idx <= 0)
          is_bk = 0;
        else
          idx --;
      }
    } else if (do_bk) {
      if (is_bk)
        idx ++;
      else { /* do_bk while !is_bk */
        if (idx <= 0)
          is_bk = !0;
        else
          idx --;
      }
    } else {
      if (idx <= 0)
        continue;
      else
        idx --;
    }

    rotate_is_back = is_bk;
    do_step(idx);
  }
}

void setup(void)
{
  ps_init();
  pinMode(PIN_PWM_A, OUTPUT);
  pinMode(PIN_PWM_B, OUTPUT);
  pinMode(PIN_A_POS_P, OUTPUT);
  pinMode(PIN_A_POS_N, OUTPUT);
  pinMode(PIN_A_NEG_P, OUTPUT);
  pinMode(PIN_A_NEG_N, OUTPUT);
  pinMode(PIN_B_POS_P, OUTPUT);
  pinMode(PIN_B_POS_N, OUTPUT);
  pinMode(PIN_B_NEG_P, OUTPUT);
  pinMode(PIN_B_NEG_N, OUTPUT);
  setup_debug();

  Wire.begin(ADDR);
  Wire.onReceive(callback_receive);
  Wire.onRequest(callback_request);
}

void loop(void)
{
  if (!doing_rotate)
    return;

  _MemoryBarrier();

  int16_t i;
  int16_t idx;
  const int16_t step_half = rotate_step >> 1;

  for (i = 0, idx = 0; i < step_half; i ++, idx += idx_step)
    do_step(idx);
  for (i = 0, idx -= idx_step; i < step_half; i ++, idx -= idx_step)
    do_step(idx);
  if (rotate_step & 1)
    do_step(0);

  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);

  doing_rotate = 0;
}
