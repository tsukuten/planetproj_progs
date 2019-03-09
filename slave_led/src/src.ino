#include <stdarg.h>
#include <avr/cpufunc.h>
#include <Wire.h>
#include "crc16.h"
#include "planetproj.h"

#if 1
#define ADDR ADDR_LED_1
#else
#define ADDR ADDR_LED_2
#endif

#define NUM_LEDS 6

/*
 * 1 - 5 - PD5 - D5
 * 2 - 4 - PD3 - D3
 * 3 - 6 - PD6 - D6
 * 4 - 2 - PB2 - D10
 * 5 - 3 - PB3 - D11
 * 6 - 1 - PB1 - D9
 */

static const int led_to_pin[NUM_LEDS] = {
   5,
   3,
   6,
  10,
  11,
   9,
};


static uint8_t recvbuf[64];
static uint8_t sendbuf[64];
static size_t sendbuf_count = 0;
static volatile _Bool is_sendbuf_ready = 0;

static volatile _Bool changing_brightness = 0;
static volatile uint8_t brightnesses[NUM_LEDS] = {0};


static void prepare_sendbuf(const size_t count, ...)
{
  va_list ap;
  is_sendbuf_ready = 0;
  _MemoryBarrier();
  sendbuf_count = count + 2;
  va_start(ap, count);
  for (size_t i = 0; i < count; i ++)
    sendbuf[i] = va_arg(ap, int);
  crc16_append(sendbuf, count);
  _MemoryBarrier();
  is_sendbuf_ready = !0;
  _MemoryBarrier();
}

static void process_set_brightness(const int n)
{
  uint8_t brightnesses_tmp[NUM_LEDS];

  memcpy((void*) brightnesses_tmp, (void*) brightnesses, sizeof(brightnesses));
  for (int i = 0; i < n; i ++) {
    const uint8_t led        = recvbuf[1 + (i << 1) + 0];
    const uint8_t brightness = recvbuf[1 + (i << 1) + 1];
    if (led >= NUM_LEDS) {
      prepare_sendbuf(2, CMD_STATUS, STATUS_UNKNOWN_COMMAND);
      return;
    }
    brightnesses_tmp[led] = brightness;
  }
  memcpy((void*) brightnesses, (void*) brightnesses_tmp, sizeof(brightnesses));
  _MemoryBarrier();
  changing_brightness = !0;
  /* Not preparing sendbuf. */
}

static void callback_receive(const int n)
{
  is_sendbuf_ready = 0;
  _MemoryBarrier();

  for (int i = 0; i < n; i ++) {
    while (!Wire.available())
      ;
    recvbuf[i] = Wire.read();
  }
  if (!crc16_check(recvbuf, n - 2)) {
    prepare_sendbuf(2, CMD_STATUS, STATUS_WRONG_CHECKSUM);
    return;
  }

  switch (recvbuf[0]) {
    case CMD_SET_BRIGHTNESS:
      process_set_brightness((n - 2) >> 1);
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

void setup(void)
{
  for (int i = 0; i < NUM_LEDS; i ++)
    pinMode(led_to_pin[i], OUTPUT);

  Wire.begin(ADDR);
  Wire.onReceive(callback_receive);
  Wire.onRequest(callback_request);
}

void loop(void)
{
  if (!changing_brightness)
    return;

  for (int i = 0; i < NUM_LEDS; i ++)
    analogWrite(led_to_pin[i], brightnesses[i]);

  prepare_sendbuf(2, CMD_STATUS, STATUS_SUCCESS);

  _MemoryBarrier();
  changing_brightness = 0;
}
