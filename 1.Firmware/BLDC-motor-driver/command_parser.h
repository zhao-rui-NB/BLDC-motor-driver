#ifndef __command_parser__
#define __command_parser__

#include "driverlib.h"
#include "uart.h"
#include "BLDC_Driver.h"

/*
set speed 1000: 0xAA 0x01 0xE8 0x03 0x40
get speed: 0xAA 0x02 0x08


*/



// 通訊協議常量定義
#define FRAME_HEAD     0xAA
// UINT8
#define CMD_SET_POWER_EN  0x01
#define CMD_GET_POWER_EN  0x02
// 4 bytes float
#define CMD_SET_TARGET_SPEED  0x03
#define CMD_GET_TARGET_SPEED  0x04
// UINT8
#define CMD_SET_IS_REVERSE  0x05
#define CMD_GET_IS_REVERSE  0x06
// UINT8
#define CMD_SET_CONTROL_MODE  0x07
#define CMD_GET_CONTROL_MODE  0x08
// 4 bytes float
#define CMD_SET_P_VALUE  0x09
#define CMD_GET_P_VALUE  0x0A
// 4 bytes float
#define CMD_SET_I_VALUE  0x0B
#define CMD_GET_I_VALUE  0x0C
// UINT16
#define CMD_SET_PWM_COMPARE  0x0D
#define CMD_GET_PWM_COMPARE  0x0E
// 4 bytes float
#define CMD_GET_CURRENT_SPEED  0x0F



// reply status
#define STATUS_OK 0x00
#define STATUS_INVALID_FRAME 0x01
#define STATUS_CHECKSUM_ERROR 0x02
#define STATUS_INVALID_CMD 0x03
#define STATUS_INVALID_PARAM 0x04


uint8_t validate_checksum(const uint8_t *buffer, uint8_t length);
void reply_status(uint8_t status);
void reply_status_uint8(uint8_t status, uint8_t value);
void reply_status_uint16(uint8_t status, uint16_t value);
void parse_command(uint8_t *buffer, uint8_t length);











#endif // __command_parser__
