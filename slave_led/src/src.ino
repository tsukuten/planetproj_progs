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

#define PIN_LED0 6
#define PIN_LED1 5
#define PIN_LED2 9
#define PIN_LED3 10
#define PIN_LED4 11
#define PIN_LED5 3

static int PIN_LEDS[6] = {
  PIN_LED0,
  PIN_LED1,
  PIN_LED2,
  PIN_LED3,
  PIN_LED4,
  PIN_LED5,
};


static uint8_t recvbuf[64];
static uint8_t sendbuf[64];
static size_t sendbuf_count = 0;
static volatile _Bool is_sendbuf_ready = 0;


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

static void process_set_brightness(const int n)
{
  int i;
  for (i = 0; i < n; i ++) {
    uint8_t led        = recvbuf[1 + (i << 1) + 0];
    uint8_t brightness = recvbuf[1 + (i << 1) + 1];
    analogWrite(PIN_LEDS[led], brightness);
  }
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

  switch (recvbuf[0]) {
    case CMD_SET_BRIGHTNESS:
      process_set_brightness(n >> 1);
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
  pinMode(PIN_LED0, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(PIN_LED4, OUTPUT);
  pinMode(PIN_LED5, OUTPUT);

  Wire.begin(ADDR);
  Wire.onReceive(callback_receive);
  Wire.onRequest(callback_request);
}

void loop(void)
{
}
