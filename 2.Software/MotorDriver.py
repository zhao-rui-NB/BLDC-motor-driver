import json 
import serial
import struct
from enum import Enum, auto

class MotorDriver:
    # command enum
    class CMDS(Enum):
        CMD_SET_POWER_EN = 'CMD_SET_POWER_EN'
        CMD_GET_POWER_EN = 'CMD_GET_POWER_EN'
        CMD_SET_TARGET_SPEED = 'CMD_SET_TARGET_SPEED'
        CMD_GET_TARGET_SPEED = 'CMD_GET_TARGET_SPEED'
        CMD_SET_IS_REVERSE = 'CMD_SET_IS_REVERSE'
        CMD_GET_IS_REVERSE = 'CMD_GET_IS_REVERSE'
        CMD_SET_CONTROL_MODE = 'CMD_SET_CONTROL_MODE'
        CMD_GET_CONTROL_MODE = 'CMD_GET_CONTROL_MODE'
        CMD_SET_P_VALUE = 'CMD_SET_P_VALUE'
        CMD_GET_P_VALUE = 'CMD_GET_P_VALUE'
        CMD_SET_I_VALUE = 'CMD_SET_I_VALUE'
        CMD_GET_I_VALUE = 'CMD_GET_I_VALUE'
        CMD_SET_PWM_COMPARE = 'CMD_SET_PWM_COMPARE'
        CMD_GET_PWM_COMPARE = 'CMD_GET_PWM_COMPARE'
        CMD_GET_CURRENT_SPEED = 'CMD_GET_CURRENT_SPEED'
        
    # data type enum
    DATA_TYPES_RX_FRAME_LEN = {
        'TYPE_NONE': 3,
        'TYPE_FLOAT': 7,
        'TYPE_UINT16': 5,
        'TYPE_UINT8': 4
    }
        

    def __init__(self, port='COM4', baudrate=115200):
        with open('command_config.json', 'r') as f:
            self.command_config = json.load(f)
        self.serial = serial.Serial(port, baudrate, timeout=2, inter_byte_timeout=0.3)
        
    def _calculate_checksum(self, data):
        checksum = 0
        for byte in data:
            checksum ^= byte
        return checksum
    
    def _send_command(self, cmd_code:int, parameters: list[int]):
        # clear the buffer
        self.serial.reset_input_buffer()
        
        frame_head = 0xAA
        data = [frame_head, cmd_code] + parameters
        checksum = self._calculate_checksum(data)
        
        data.append(checksum)
        self.serial.write(bytearray(data))
        
    def _receive_response(self):
        response = self.serial.read(100)
        return response

    def _bytes_to_num(self, types, buffer):
        '''Convert bytes to number'''
        # TYPE_FLOAT, TYPE_UINT16, TYPE_UINT8
        try:
            if types == 'TYPE_FLOAT':
                return struct.unpack('<f', buffer)[0]
            elif types == 'TYPE_UINT16':
                return struct.unpack('<H', buffer)[0]
            elif types == 'TYPE_UINT8':
                return struct.unpack('B', buffer)[0]
        except Exception as e:   
            print(e)     
            return None
        return None
        
    def _num_to_bytes(self, types, num):
        '''Convert number to bytes'''
        # TYPE_FLOAT, TYPE_UINT16, TYPE_UINT8
        try:
            if types == 'TYPE_FLOAT':
                return struct.pack('<f', num)
            elif types == 'TYPE_UINT16':
                return struct.pack('<H', num)
            elif types == 'TYPE_UINT8':
                return struct.pack('B', num)
        except Exception as e:        
            return None
        return None
        
    def execute_command(self, command: CMDS, value=None):
        cmd_info_dict = self.command_config[command.name]
        
        code = cmd_info_dict['code']
        code = int(code, 16) if 'x' in code or 'X' in code else int(code)
        
        parameters = list(self._num_to_bytes(cmd_info_dict['send_type'], value)) if value is not None else []
        
        
        self._send_command(code, parameters)
        response = self._receive_response()
        
        # 校驗碼錯誤或幀頭不正確
        if len(response)<3 or response[0] != 0xAA or response[-1] != self._calculate_checksum(response[:-1]):
            return None

        # 狀態碼不正確
        if response[1] != 0x00:
            return None
        
        response_type = cmd_info_dict['response_type']
        expected_length = self.DATA_TYPES_RX_FRAME_LEN[response_type]
        
        if len(response) != expected_length:
            return None
        
        return self._bytes_to_num(response_type, response[2:-1])
        

if __name__ == "__main__":
    import time
    
    motor = MotorDriver()
    # print(motor.execute_command(MotorDriver.CMDS.CMD_GET_POWER_EN))
    # print(motor.execute_command(MotorDriver.CMDS.CMD_SET_POWER_EN, 0))
    # time.sleep(3)
    # print(motor.execute_command(MotorDriver.CMDS.CMD_SET_POWER_EN, 1))
    # # print(motor.execute_command(MotorDriver.CMDS.CMD_GET_TARGET_SPEED))
    # # print(motor.execute_command(MotorDriver.CMDS.CMD_GET_CURRENT_SPEED))
    
    def get_speed():
        print(motor.execute_command(MotorDriver.CMDS.CMD_GET_CURRENT_SPEED))
        time.sleep(1)
    
    print(motor.execute_command(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, 1000))
    for i in range(5):
        get_speed()
    print(motor.execute_command(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, 2000))
    for i in range(5):
        get_speed()
    print(motor.execute_command(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, 3000))
    for i in range(5):
        get_speed()
    print(motor.execute_command(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, 4000))
    for i in range(5):
        get_speed()
    print(motor.execute_command(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, 2500))
    for i in range(5):
        get_speed()