#ifndef PLANETPROJ_H_
#define PLANETPROJ_H_


#define CMD_STATUS 0x10
#define STATUS_SUCCESS 0x00
#define STATUS_WRONG_CHECKSUM 0x01
#define STATUS_UNKNOWN_COMMAND 0x02
#define STATUS_NOT_READY 0x03

#define CMD_SET_BRIGHTNESS 0x20

#define CMD_SET_ROTATE 0x30
#define CMD_SET_POWER 0x31
#define CMD_SET_MAX_IDX 0x32
#define CMD_SET_IDX_STEP 0x33


#define ADDR_LED_1 0x30
#define ADDR_LED_2 0x31
#define ADDR_MOTOR_1 0x40
#define ADDR_MOTOR_2 0x41


#endif /* PLANETPROJ_H_ */
