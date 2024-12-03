#include "command_parser.h"


uint8_t validate_checksum(const uint8_t *buffer, uint8_t length) {
    uint8_t checksum = 0;
    uint8_t i;
    for (i = 0; i < length - 1; i++) {
        checksum ^= buffer[i];
    }
    return checksum == buffer[length - 1];
}

void reply_status(uint8_t status){
    // frame head + status + checksum
    uint8_t reply_buffer[3] = {FRAME_HEAD, status, 0};
    reply_buffer[2] = reply_buffer[0] ^ reply_buffer[1];
    uart_write_bytes(reply_buffer, 3);    
}

// float
void reply_status_float(uint8_t status, float value){
    // frame head + status + value(4) + checksum
    uint8_t reply_buffer[7] = {FRAME_HEAD, status,0,0,0,0,0};
    memcpy(reply_buffer + 2, &value, 4);
    uint8_t i;
    for (i = 0; i < 7 - 1; i++) {
        reply_buffer[7 - 1] ^= reply_buffer[i];
    }
    uart_write_bytes(reply_buffer, 7);
}
    
void reply_status_uint8(uint8_t status, uint8_t value){
    // frame head + status + value(1) + checksum
    uint8_t reply_buffer[4] = {FRAME_HEAD, status, value,0};
    reply_buffer[3] = reply_buffer[0] ^ reply_buffer[1] ^ reply_buffer[2];
    uart_write_bytes(reply_buffer, 4);
}

void reply_status_uint16(uint8_t status, uint16_t value){
    // frame head + status + value(2) + checksum
    uint8_t reply_buffer[5] = {FRAME_HEAD, status, 0, 0, 0};
    memcpy(reply_buffer + 2, &value, 2);
    uint8_t i;
    for (i = 0; i < 5 - 1; i++) {
        reply_buffer[5 - 1] ^= reply_buffer[i];
    }
    uart_write_bytes(reply_buffer, 5);
}




// 指令解析與處理
void parse_command(uint8_t *buffer, uint8_t length) {
    if (length < 3 || buffer[0]!=FRAME_HEAD) {
        reply_status(STATUS_INVALID_FRAME);
        return;
    }

    // 檢查校驗
    if (!validate_checksum(buffer, length)) {
        reply_status(STATUS_CHECKSUM_ERROR);
        return;
    }

    // 解析命令
    uint8_t command = buffer[1];
    switch (command) {
        case CMD_SET_TARGET_SPEED:
            if (length != 7) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            float speed;
            memcpy(&speed, buffer + 2, 4);
            set_target_speed(speed);
            reply_status(STATUS_OK);
            break;
            
        case CMD_GET_TARGET_SPEED:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_float(STATUS_OK, get_target_speed());
            break;

        case CMD_SET_POWER_EN:
            if (length != 4) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            set_power_en(buffer[2]);
            reply_status(STATUS_OK);
            break;

        case CMD_GET_POWER_EN:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_uint8(STATUS_OK, get_power_en());
            break;
        
        case CMD_SET_IS_REVERSE:
            if (length != 4) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            set_is_reverse(buffer[2]);
            reply_status(STATUS_OK);
            break;
        
        case CMD_GET_IS_REVERSE:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_uint8(STATUS_OK, get_is_reverse());
            break;

        case CMD_SET_CONTROL_MODE:
            if (length != 4) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            set_control_mode(buffer[2]);
            reply_status(STATUS_OK);
            break;

        case CMD_GET_CONTROL_MODE:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_uint8(STATUS_OK, get_control_mode());
            break;

        case CMD_SET_P_VALUE:
            if (length != 7) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            float p;
            memcpy(&p, buffer + 2, 4);
            set_p_value(p);
            reply_status(STATUS_OK);
            break;

        case CMD_GET_P_VALUE:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_float(STATUS_OK, get_p_value());
            break;

        case CMD_SET_I_VALUE:
            if (length != 7) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            float i;
            memcpy(&i, buffer + 2, 4);
            set_i_value(i);
            reply_status(STATUS_OK);
            break;

        case CMD_GET_I_VALUE:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_float(STATUS_OK, get_i_value());
            break;

        case CMD_SET_PWM_COMPARE:
            if (length != 5) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            uint16_t compare;
            memcpy(&compare, buffer + 2, 2);
            set_pwm_compare(compare);
            reply_status(STATUS_OK);
            break;

        case CMD_GET_PWM_COMPARE:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_uint16(STATUS_OK, get_pwm_compare());
            break;

        case CMD_GET_CURRENT_SPEED:
            if (length != 3) {
                reply_status(STATUS_INVALID_PARAM);
                return;
            }
            reply_status_float(STATUS_OK, get_current_speed());
            break;

        default:
            reply_status(STATUS_INVALID_CMD);
            break;

        
    }
}
