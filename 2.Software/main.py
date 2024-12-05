import serial
import struct
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, Select, Button, Static, Input
from textual.screen import Screen
from textual.containers import Container, Vertical, Horizontal

# Copy the constants and configuration from send_command.py
FRAME_HEAD = 0xAA

# Command constants (copying from original script)
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

# Data type constants
TYPE_NONE = 0x00
TYPE_FLOAT = 0x01
TYPE_UINT16 = 0x02
TYPE_UINT8 = 0x03

# Mapping of commands to their descriptions
COMMAND_MAP = {
    CMD_SET_POWER_EN: "設置電源使能",
    CMD_GET_POWER_EN: "獲取電源使能",
    CMD_SET_TARGET_SPEED: "設置目標速度",
    CMD_GET_TARGET_SPEED: "獲取目標速度",
    CMD_SET_IS_REVERSE: "設置反向",
    CMD_GET_IS_REVERSE: "獲取反向",
    CMD_SET_CONTROL_MODE: "設置控制模式",
    CMD_GET_CONTROL_MODE: "獲取控制模式",
    CMD_SET_P_VALUE: "設置 P 值",
    CMD_GET_P_VALUE: "獲取 P 值",
    CMD_SET_I_VALUE: "設置 I 值",
    CMD_GET_I_VALUE: "獲取 I 值",
    CMD_SET_PWM_COMPARE: "設置 PWM 比較值",
    CMD_GET_PWM_COMPARE: "獲取 PWM 比較值",
    CMD_GET_CURRENT_SPEED: "獲取當前速度",
}

# Mapping of command types
SEND_DATA_TYPE = {
    CMD_SET_POWER_EN: TYPE_UINT8,
    CMD_SET_TARGET_SPEED: TYPE_FLOAT,
    CMD_SET_IS_REVERSE: TYPE_UINT8,
    CMD_SET_CONTROL_MODE: TYPE_UINT8,
    CMD_SET_P_VALUE: TYPE_FLOAT,
    CMD_SET_I_VALUE: TYPE_FLOAT,
    CMD_SET_PWM_COMPARE: TYPE_UINT16,
}

RESPONSE_DATA_TYPE = {
    CMD_GET_POWER_EN: TYPE_UINT8,
    CMD_GET_TARGET_SPEED: TYPE_FLOAT,
    CMD_GET_IS_REVERSE: TYPE_UINT8,
    CMD_GET_CONTROL_MODE: TYPE_UINT8,
    CMD_GET_P_VALUE: TYPE_FLOAT,
    CMD_GET_I_VALUE: TYPE_FLOAT,
    CMD_GET_PWM_COMPARE: TYPE_UINT16,
    CMD_GET_CURRENT_SPEED: TYPE_FLOAT,
}

class SerialCommunication:
    def __init__(self, port='COM4', baudrate=115200):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=2, inter_byte_timeout=0.5)
        except serial.SerialException as e:
            print(f"Serial connection error: {e}")
            self.ser = None

    def calculate_checksum(self, data):
        checksum = 0
        for byte in data:
            checksum ^= byte
        return checksum

    def send_command(self, command, parameters=[]):
        if not self.ser:
            return "Serial port not connected"
        
        frame_head = 0xAA
        data = [frame_head, command] + parameters
        checksum = self.calculate_checksum(data)
        data.append(checksum)
        
        try:
            self.ser.write(bytearray(data))
            return "指令發送成功: " + ' '.join(f"{byte:02X}" for byte in data)
        except Exception as e:
            return f"發送指令失敗: {e}"

    def handle_command(self, command):
        if not self.ser:
            return "Serial port not connected"
        
        try:
            response = list(self.ser.read_until())
            
            if len(response) < 3 or response[0] != 0xAA:
                return "校驗碼錯誤或幀頭不正確"
            
            if response[1] != 0x00:
                return f"錯誤: 狀態碼 {response[1]}"
            
            response_type = RESPONSE_DATA_TYPE.get(command, TYPE_NONE)
            
            if response_type == TYPE_FLOAT:
                if len(response) == 7:
                    value = struct.unpack('<f', bytes(response[2:6]))[0]
                    return f"浮點回應值: {value}"
                
            elif response_type == TYPE_UINT16:
                if len(response) == 5:
                    value = struct.unpack('<H', bytes(response[2:4]))[0]
                    return f"UINT16 回應值: {value}"
                
            elif response_type == TYPE_UINT8:
                if len(response) == 4:
                    value = response[2]
                    return f"UINT8 回應值: {value}"
            
            return "回應成功，無額外數據"
        
        except Exception as e:
            return f"接收回應失敗: {e}"

class SerialControlApp(App):
    """Textual Serial Control Panel Application"""
    
    CSS___ = """
    Screen {
        background: black;
        color: white;
    }
    
    #command-select {
        width: 50%;
        margin: 1 1;
    }
    
    #input-container {
        height: 5;
        margin: 1 1;
        padding: 1 1;
        border: tall $text;
    }
    
    #response-display {
        height: 10;
        border: tall $primary;
        padding: 1 1;
        overflow-y: auto;
    }
    
    Button {
        width: auto;
        margin: 1 1;
        background: $primary 40%;
    }
    
    Button:hover {
        background: $primary 60%;
    }
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.serial_comm = SerialCommunication()
        self.responses = []

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        
        with Container(id="main-container"):
            # Command Selection Dropdown
            yield Select(
                [(description, command) for command, description in COMMAND_MAP.items()], 
                prompt="選擇指令", 
                id="command-select"
            )
            
            # Input Container for data entry
            with Container(id="input-container"):
                yield Input(placeholder="輸入數值（如需要）", id="data-input")
            
            # Buttons for actions
            with Horizontal(id="action-buttons"):
                yield Button("發送指令", variant="primary", id="send-btn")
                yield Button("獲取回應", variant="success", id="get-response-btn")
            
            # Response Display
            with Container(id="response-display"):
                yield Static("回應將顯示在此處", id="response-text")

        yield Footer()

    def on_mount(self):
        self.query_one("#send-btn").on_click(self.send_command)
        self.query_one("#get-response-btn").on_click(self.get_response)
        self.query_one("#command-select").on_change(self.on_command_change)

    def on_command_change(self, message):
        # Reset input when command changes
        self.query_one("#data-input").value = ""
        
        # Check if selected command requires input
        command = message.value
        if command in SEND_DATA_TYPE:
            input_type = SEND_DATA_TYPE[command]
            placeholders = {
                TYPE_FLOAT: "輸入浮點數",
                TYPE_UINT16: "輸入 16 位無符號整數",
                TYPE_UINT8: "輸入 8 位無符號整數"
            }
            self.query_one("#data-input").placeholder = placeholders.get(input_type, "輸入數值")
        else:
            self.query_one("#data-input").placeholder = "此指令不需要輸入"

    def send_command(self, _):
        # Get selected command and input value
        command = self.query_one("#command-select").value
        input_value = self.query_one("#data-input").value
        
        try:
            # Prepare data based on command type
            data_type = SEND_DATA_TYPE.get(command, TYPE_NONE)
            
            if data_type == TYPE_FLOAT and input_value:
                data = struct.pack('<f', float(input_value))
            elif data_type == TYPE_UINT16 and input_value:
                data = struct.pack('<H', int(input_value))
            elif data_type == TYPE_UINT8 and input_value:
                data = bytes([int(input_value)])
            else:
                data = []
            
            # Send command
            result = self.serial_comm.send_command(command, list(data))
            self.update_response(result)
        
        except ValueError:
            self.update_response("輸入值格式錯誤")
        except Exception as e:
            self.update_response(f"發送指令失敗: {e}")

    def get_response(self, _):
        # Get selected command
        command = self.query_one("#command-select").value
        
        # Handle response
        result = self.serial_comm.handle_command(command)
        self.update_response(result)

    def update_response(self, message):
        # Add message to responses and update display
        self.responses.append(message)
        if len(self.responses) > 10:
            self.responses.pop(0)
        
        # Update response text
        response_text = "\n".join(self.responses)
        self.query_one("#response-text").update(response_text)

def main():
    app = SerialControlApp()
    app.run()

if __name__ == "__main__":
    main()