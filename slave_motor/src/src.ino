#include <stdarg.h>
#include <avr/cpufunc.h>
#include <Wire.h>
#include "crc16.h"
#include "planetproj.h"
#include "motor.h"

#define PIN_DB0 A0
#define PIN_DB1 A1
#define PIN_DB2 A2
#define PIN_DB3 A3

/*
 * PK267JDA is 1.8 degree/step.  So the maximum number of steps to round one
 * time is: 360/1.8*100=20000, so 16-bit number is required for step.
 */

#if 1
#include "interval_table_d100.h"
#define ADDR ADDR_MOTOR_1
#else
#include "interval_table_d10.h"
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

static const unsigned pins[2][2][2] = {
  {
    {
      PIN_A_POS_P,
      PIN_A_POS_N
    },
    {
      PIN_A_NEG_P,
      PIN_A_NEG_N
    }
  },
  {
    {
      PIN_B_POS_P,
      PIN_B_POS_N
    },
    {
      PIN_B_NEG_P,
      PIN_B_NEG_N
    }
  }
};

static void motor_set_pulse_core(const enum motor_coil coil,
    const enum motor_polar polar, const enum motor_pulse pulse)
{
  unsigned pin_p, pin_n;

  pin_p = pins[coil][polar][MOTOR_CHANNEL_P];
  pin_n = pins[coil][polar][MOTOR_CHANNEL_N];

  /*
   * pin_p: Pull to HIGH to turn on.
   * pin_n: Pull to LOW to turn on.
   */
  switch (pulse) {
    case MOTOR_PULSE_POS:
      digitalWrite(pin_n, HIGH); // n: off
      digitalWrite(pin_p, HIGH); // p: on
      break;
    case MOTOR_PULSE_NEG:
      digitalWrite(pin_p, LOW);  // p: off
      digitalWrite(pin_n, LOW);  // n: on
      break;
    case MOTOR_PULSE_NTL:
      digitalWrite(pin_p, LOW);  // p: off
      digitalWrite(pin_n, HIGH); // n: off
      break;
    default:
      return;
  }
}

#define motor_set_pulse(apos, aneg, bpos, bneg) \
  do { \
    motor_set_pulse_core(MOTOR_COIL_A, MOTOR_POLAR_POS, MOTOR_PULSE_##apos); \
    motor_set_pulse_core(MOTOR_COIL_A, MOTOR_POLAR_NEG, MOTOR_PULSE_##aneg); \
    motor_set_pulse_core(MOTOR_COIL_B, MOTOR_POLAR_POS, MOTOR_PULSE_##bpos); \
    motor_set_pulse_core(MOTOR_COIL_B, MOTOR_POLAR_NEG, MOTOR_PULSE_##bneg); \
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

static inline void do_drive(const _Bool is_back)
{
  if (drive_mode == 0)
    do_wave_drive(is_back);
  if (drive_mode == 1)
    do_full_step_drive(is_back);
  if (drive_mode == 2)
    do_half_step_drive(is_back);
}

static uint8_t recvbuf[64];
static uint8_t sendbuf[64];
static size_t sendbuf_count = 0;
static volatile _Bool is_sendbuf_ready = 0;

static volatile _Bool doing_rotate = 0;
static _Bool rotate_is_back = 0;
static uint16_t rotate_step = 0;


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

void setup_debug(void)
{
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PIN_DB0, INPUT_PULLUP);
  pinMode(PIN_DB1, INPUT_PULLUP);
  pinMode(PIN_DB2, INPUT_PULLUP);
  pinMode(PIN_DB3, INPUT_PULLUP);
  const byte db0 = digitalRead(PIN_DB0);
  const byte db1 = digitalRead(PIN_DB1);

  drive_mode = (!db0) | ((!db1) << 1);
}

void setup(void)
{
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

static void delayMicroseconds_coarse(const uint32_t us)
{
  int32_t c = us >> 7;
  while (c-- > 0)
    _delay_us(1UL << 7);
}

static inline void do_step(const int16_t idx)
{
  const int16_t n = sizeof(interval_table) / sizeof(*interval_table);
  static _Bool stat = 0;

  digitalWrite(LED_BUILTIN, stat = !stat);
  delayMicroseconds_coarse(pgm_read_dword(&(interval_table[min(idx, n - 1)])));
  do_drive(rotate_is_back);
}

void loop(void)
{
  if (!doing_rotate)
    return;

  _MemoryBarrier();

  int16_t i;
  int16_t idx;
  const int16_t step_half = rotate_step >> 1;

  for (i = 0, idx = 0; i < step_half; i ++, idx ++)
    do_step(idx);
  for (i = 0, idx -= 1; i < step_half; i ++, idx --)
    do_step(idx);
  if (rotate_step & 1)
    do_step(0);

  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);

  doing_rotate = 0;
}
