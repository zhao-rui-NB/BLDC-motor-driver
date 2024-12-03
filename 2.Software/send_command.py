import serial
import struct

# 串行端口配置
ser = serial.Serial('COM4', 115200, timeout=2)  # 確保 COM 端口與波特率設定正確


FRAME_HEAD = 0xAA


CMD_SET_POWER_EN = 0x01
CMD_GET_POWER_EN = 0x02
CMD_SET_TARGET_SPEED = 0x03
CMD_GET_TARGET_SPEED = 0x04
CMD_SET_IS_REVERSE = 0x05
CMD_GET_IS_REVERSE = 0x06
CMD_SET_CONTROL_MODE = 0x07
CMD_GET_CONTROL_MODE = 0x08
CMD_SET_P_VALUE = 0x09
CMD_GET_P_VALUE = 0x0A
CMD_SET_I_VALUE = 0x0B
CMD_GET_I_VALUE = 0x0C
CMD_SET_PWM_COMPARE = 0x0D
CMD_GET_PWM_COMPARE = 0x0E
CMD_GET_CURRENT_SPEED = 0x0F

# reponse data type 

TYPE_NONE = 0x00
TYPE_FLOAT = 0x01
TYPE_UINT16 = 0x02
TYPE_UINT8 = 0x03


SEND_DATA_TYPE = {
    CMD_SET_POWER_EN: TYPE_UINT8,
    CMD_GET_POWER_EN: TYPE_NONE,
    
    CMD_SET_TARGET_SPEED: TYPE_FLOAT,
    CMD_GET_TARGET_SPEED: TYPE_NONE,
    
    CMD_SET_IS_REVERSE: TYPE_UINT8,
    CMD_GET_IS_REVERSE: TYPE_NONE,
    
    CMD_SET_CONTROL_MODE: TYPE_UINT8,
    CMD_GET_CONTROL_MODE: TYPE_NONE,
    
    CMD_SET_P_VALUE: TYPE_FLOAT,
    CMD_GET_P_VALUE: TYPE_NONE,
    
    CMD_SET_I_VALUE: TYPE_FLOAT,
    CMD_GET_I_VALUE: TYPE_NONE,
    
    CMD_SET_PWM_COMPARE: TYPE_UINT16,
    CMD_GET_PWM_COMPARE: TYPE_NONE,
    
    CMD_GET_CURRENT_SPEED: TYPE_NONE,
}
    

# ONLY GET will have response data type, other command will not have response data type
RESPONSE_DATA_TYPE = {
    CMD_SET_POWER_EN: TYPE_NONE,
    CMD_GET_POWER_EN: TYPE_UINT8,
    
    CMD_SET_TARGET_SPEED: TYPE_NONE,
    CMD_GET_TARGET_SPEED: TYPE_FLOAT,    

    CMD_SET_IS_REVERSE: TYPE_NONE,
    CMD_GET_IS_REVERSE: TYPE_UINT8,
    
    CMD_SET_CONTROL_MODE: TYPE_NONE,
    CMD_GET_CONTROL_MODE: TYPE_UINT8,
    
    CMD_SET_P_VALUE: TYPE_NONE,
    CMD_GET_P_VALUE: TYPE_FLOAT,
    
    CMD_SET_I_VALUE: TYPE_NONE,
    CMD_GET_I_VALUE: TYPE_FLOAT,
    
    CMD_SET_PWM_COMPARE: TYPE_NONE,
    CMD_GET_PWM_COMPARE: TYPE_UINT16,
    
    CMD_GET_CURRENT_SPEED: TYPE_FLOAT,
}


# header(1) + command(1) + data(N) + checksum(1)
TYPE_TO_LENGTH = {
    TYPE_NONE: 3,
    TYPE_FLOAT: 7,
    TYPE_UINT16: 5,
    TYPE_UINT8: 4,
}
    


def calculate_checksum(data):
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum



def send_command(command, parameters=[]):
    frame_head = 0xAA
    data = [frame_head, command] + parameters
    checksum = calculate_checksum(data)
    data.append(checksum)
    ser.write(bytearray(data))
    print("指令發送:", ' '.join(f"{byte:02X}" for byte in data))

def parse_float_response(response):
    if len(response) == 7:
        value = struct.unpack('<f', bytes(response[2:6]))[0]
        print(f"浮點回應值: {value}")
    else:
        print("回應格式錯誤或數據不完整")

def parse_uint16_response(response):
    if len(response) == 5:
        value = struct.unpack('<H', bytes(response[2:4]))[0]
        print(f"UINT16 回應值: {value}")
    else:
        print("回應格式錯誤或數據不完整")

def parse_uint8_response(response):
    if len(response) == 4:
        value = response[2]
        print(f"UINT8 回應值: {value}")
    else:
        print("回應格式錯誤或數據不完整")

def parse_generic_response(response):
    if len(response) == 3:
        print("回應成功，無額外數據")
    else:
        print("回應格式錯誤或數據不完整")

def handle_command(command):
    response = list(ser.read_until())
    print("回應接收:", ' '.join(f"{byte:02X}" for byte in response))
    
    if len(response) < 3 or response[0] != 0xAA or response[-1] != calculate_checksum(response[:-1]):
        print("校驗碼錯誤或幀頭不正確")
        return
    
    if response[1] != 0x00:
        print(f"錯誤: 狀態碼 {response[1]}")
        return
    
    response_type = RESPONSE_DATA_TYPE[command]
    expected_length = TYPE_TO_LENGTH[response_type]
    
    if len(response) != expected_length:
        print(f"預期回應長度為 {expected_length}, 但收到的回應長度為 {len(response)}")
        return
    
    
    if response_type == TYPE_FLOAT:
        parse_float_response(response)
    elif response_type == TYPE_UINT16:
        parse_uint16_response(response)
    elif response_type == TYPE_UINT8:
        parse_uint8_response(response)
    else:
        parse_generic_response(response)
        



def main():
    commands = {
        1: (CMD_SET_POWER_EN, "設置電源使能"),
        2: (CMD_GET_POWER_EN, "獲取電源使能"),
        3: (CMD_SET_TARGET_SPEED, "設置目標速度"),
        4: (CMD_GET_TARGET_SPEED, "獲取目標速度"),
        5: (CMD_SET_IS_REVERSE, "設置反向"),
        6: (CMD_GET_IS_REVERSE, "獲取反向"),
        7: (CMD_SET_CONTROL_MODE, "設置控制模式"),
        8: (CMD_GET_CONTROL_MODE, "獲取控制模式"),
        9: (CMD_SET_P_VALUE, "設置 P 值"),
        10: (CMD_GET_P_VALUE, "獲取 P 值"),
        11: (CMD_SET_I_VALUE, "設置 I 值"),
        12: (CMD_GET_I_VALUE, "獲取 I 值"),
        13: (CMD_SET_PWM_COMPARE, "設置 PWM 比較值"),
        14: (CMD_GET_PWM_COMPARE, "獲取 PWM 比較值"),
        15: (CMD_GET_CURRENT_SPEED, "獲取當前速度"),
        16: (0, "退出程序"),
    }
    
    
    while True:
        print("\n選擇操作:")
        for key, value in commands.items():
            print(f"{key}. {value[1]}")
        choice = int(input("\n輸入選擇: "))
        
        print()
        print('#' * 20)
        
        
        if choice == 16:
            print("程序退出")
            break
        elif choice in commands:
            # print command description
            command, description = commands[choice]
            print('選擇 ->', description) 
            # if command need to send data, ask user to input data
            command, _ = commands[choice]
            send_data_type = SEND_DATA_TYPE[command]
            if send_data_type == TYPE_FLOAT:
                data = struct.pack('<f', float(input("輸入浮點數: ")))
            elif send_data_type == TYPE_UINT16:
                data = struct.pack('<H', int(input("輸入 UINT16 數: ")))
            elif send_data_type == TYPE_UINT8:
                data = bytes([int(input("輸入 UINT8 數: "))])
            else:
                data = []
            send_command(command, list(data))
            handle_command(command)
        else:
            print("無效選擇")
            


if __name__ == '__main__':
    main()
