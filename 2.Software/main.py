import serial.tools.list_ports
from textual.app import App, ComposeResult
from textual.containers import Container, Horizontal, Vertical, Grid
from textual.widgets import Select, Button, Static, Label, Header, Footer, Digits, Input, Switch
from textual.reactive import Reactive
from textual.binding import Binding 
from textual.validation import Integer, Number
# key event import
from textual.events import Key

from textual.widgets._select import SelectOverlay

from MotorDriver import MotorDriver  # 導入 MotorDriver 類

from threading import Thread
import asyncio

class SelectWithoutUpDown(Select):
    Select.BINDINGS = [
        Binding(key='enter,space', action='show_overlay', description='Show menu', show=False, key_display=None, priority=False, tooltip='', id=None)
    ]



class MotorControlApp(App):
    CSS_PATH = 'main.css'
    
    BINDINGS = [
        
        ("ctrl+t", "toggle_dark", "Toggle dark"),
        ("ctrl+r", "reload_driver_parameters", "reload"),
        # toggle power on/off
        ("ctrl+o", "toggle_power", "Toggle Power On/Off"),
        # direction switch
        ("ctrl+d", "toggle_direction", "Toggle Direction"),
    ]
    
    def __init__(self):
        super().__init__()
        self.motor_driver = MotorDriver()
        self.speed_timer = None
    
    def compose(self):
        yield Header("Motor Control")
        yield Footer()
        
        with Container(id="connection"):
            with Horizontal():
                # yield Static("Select Serial Port:")
                yield SelectWithoutUpDown([],id="com_port_select", prompt="Select COM Port", classes="nconnected")
                yield Button("Scan Ports", id="scan_button")
                yield Button("Connect", id="connect_button")                
                yield Button("Disconnect", id="disconnect_button", classes="hidden_class")

        
        with Horizontal():
            yield Label("Current Speed:")
            yield Digits("2009.87", id="speed_display")


        with Horizontal():
            yield Label("POWER ON/OFF:", id="power_label")
            yield Switch(id="power_switch")
            
            yield Label("Direction:", id="direction_label")
            yield Switch(id="direction_switch")
            
            # add a space make button align right
            yield Static("", classes="spacer")
            
            yield Button("RELOAD PARAMETER", id="reload")
        
        # the control type select pwm or pid
        with Horizontal():
            yield Label("Control Type:")
            yield SelectWithoutUpDown([("PWM","PWM"), ("PI Speed","PI Speed")], id="control_type_select", prompt="Select Control Type")
            
        
        with Container(id="pwm_patameters" , classes="hidden_class"):
            with Horizontal():
                yield Label("PWM compare value:")
                yield Input(placeholder="Enter PWM compare value", id="pwm_input", validators=Integer(minimum=0, maximum=500))
            
        with Container(id="pid_patameters", classes="hidden_class"):
            with Horizontal():
                yield Label("Kp:")
                yield Input(placeholder="Enter Kp float", id="kp_input", validators=Number(minimum=0))
            with Horizontal():
                yield Label("Ki:")
                yield Input(placeholder="Enter Ki float", id="ki_input", validators=Number(minimum=0))

            with Horizontal():
                yield Label("Target Speed:")
                yield Input(placeholder="Enter Target ", id="target_speed_input", validators=Number(minimum=0, maximum=10000))

    def action_toggle_dark(self) -> None:
        self.theme = ("textual-dark" if self.theme == "textual-light" else "textual-light")

    def action_toggle_power(self):
        power_switch = self.query_one("#power_switch", Switch)
        power_switch.value = not power_switch.value
        
    def action_toggle_direction(self):
        direction_switch = self.query_one("#direction_switch", Switch)
        direction_switch.value = not direction_switch.value

    def on_key(self, event: Key):
        print(event)
        
        if event.key == "left":
            if isinstance(self.focused, SelectOverlay):
                return
            self.action_focus_previous()
        
        elif event.key == "right":
            if isinstance(self.focused, SelectOverlay):
                return
            self.action_focus_next()
            
        elif event.key == "up":
            if isinstance(self.focused, SelectOverlay):
                return
            self.action_focus_previous()
            
        elif event.key == "down":
            if isinstance(self.focused, SelectOverlay):
                return
            self.action_focus_next()
            

    
    def _scan_ports(self):
        ports = serial.tools.list_ports.comports()
        port_options = [(f'{port.device}\t{port.description}', port.device) for port in ports]
        select = self.query_one("#com_port_select", Select)
        select.set_options(port_options)  # 更新選項
        print("Scanning Ports")
        
    # when scan button is pressed scan the com ports, and update the select widget
    async def on_button_pressed(self, event: Button.Pressed):
        if event.button.id == "scan_button":
            print("Scan Button Pressed")
            self._scan_ports()

            
        elif event.button.id == "connect_button":
            print("Connect Button Pressed")
            select = self.query_one("#com_port_select", Select)
            selected_port = select.value
            if selected_port:
                connect_succ = self.motor_driver.connect(selected_port, 115200)
                if connect_succ:
                    print("Connected")
                    con_container = self.query_one("#connection", Container)
                    con_container.add_class("connected")
                    self.query_one("#com_port_select", Select).add_class("connected")
                    self.query_one("#com_port_select", Select).remove_class("nconnected")
                    
                    self.query_one("#connect_button", Button).add_class("hidden_class")
                    self.query_one("#disconnect_button", Button).remove_class("hidden_class")
                else:
                    print("Connection Failed")
                    con_container = self.query_one("#connection", Container)
                    con_container.remove_class("connected")
                    self.query_one("#com_port_select", Select).add_class("nconnected")
                    self.query_one("#com_port_select", Select).remove_class("connected")
                    
                    self.query_one("#connect_button", Button).remove_class("hidden_class")
                    self.query_one("#disconnect_button", Button).add_class("hidden_class")
                    
                    if self.speed_timer:
                        self.speed_timer.stop()
                        self.speed_timer = None
                    
            self.speed_timer = self.set_interval(0.8, self._update_speed)  # 每秒更新速度
        
        elif event.button.id == "disconnect_button":
            print("Disconnect Button Pressed")
            disconnect_succ = self.motor_driver.disconnect()
            if disconnect_succ:
                print("Disconnected")
                self.query_one("#com_port_select", Select).add_class("nconnected")
                self.query_one("#com_port_select", Select).remove_class("connected")
                
                self.query_one("#connect_button", Button).remove_class("hidden_class")
                self.query_one("#disconnect_button", Button).add_class("hidden_class")
                
                if self.speed_timer:
                    self.speed_timer.stop()
                    self.speed_timer = None
                    
                self._clear_all_widget_data()
    
        elif event.button.id == "reload":
            print("Reload Button Pressed")
            
            # self._reload_driver_partameters()
            await self._action_reload_driver_parameters()
                    
        
    async def on_select_changed(self, event: Select.Changed):
        # print which select is changed
        if event.select.id == "control_type_select":
            if event.value == "PWM":
                print("PWM Selected")
                self.query_one("#pwm_patameters").remove_class("hidden_class")
                self.query_one("#pid_patameters").add_class("hidden_class")
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_CONTROL_MODE, 0)
                
            elif event.value == "PI Speed":
                print("PI Speed Selected")
                self.query_one("#pwm_patameters").add_class("hidden_class")
                self.query_one("#pid_patameters").remove_class("hidden_class")
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_CONTROL_MODE, 1)
            
        elif event.select.id == "control_type_select":
            print("Control Type Changed")
            await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_CONTROL_MODE, 0 if event.value == "PWM" else 1)            
                
    async def on_switch_changed(self, event: Switch.Changed):
        if event.switch.id == "power_switch":
            print("Power Switch Changed")
            await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_POWER_EN, 1 if event.value else 0)
            
        elif event.switch.id == "direction_switch":
            print("Direction Switch Changed")
            await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_IS_REVERSE, 1 if event.value else 0)
            
    async def on_input_submitted(self, event: Input.Submitted):
        if event.input.id == "target_speed_input":
            print("Target Speed Submitted")
            if len(event.validation_result.failures)==0:
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_TARGET_SPEED, float(event.value))
            
        elif event.input.id == "pwm_input":
            print("PWM Compare Submitted")
            if len(event.validation_result.failures)==0:
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_PWM_COMPARE, int(event.value))
            
        elif event.input.id == "kp_input":
            print("Kp Submitted")
            if len(event.validation_result.failures)==0:
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_P_VALUE, float(event.value))
            
        elif event.input.id == "ki_input":
            print("Ki Submitted")
            if len(event.validation_result.failures)==0:
                await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_SET_I_VALUE, float(event.value))                
    
    def on_mount(self):
        self._scan_ports()
    
    # motor driver api
    async def _update_speed(self):
        speed = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_CURRENT_SPEED)
        speed = f"{speed:.4f}" if speed is not None else "0"
        if speed is not None:
            speed_display = self.query_one("#speed_display", Digits)
            speed_display.update(speed)
        else:
            speed_display = self.query_one("#speed_display", Digits)
            speed_display.update("Error Reading Speed")
            
        await asyncio.sleep(0.1)
    
    
    def _clear_all_widget_data(self):
        self.query_one("#speed_display", Digits).update("0")
        self.query_one("#power_switch", Switch).value = False
        self.query_one("#direction_switch", Switch).value = False
        self.query_one("#target_speed_input", Input).value = ""
        self.query_one("#control_type_select", Select).value = Select.BLANK
        self.query_one("#pwm_input", Input).value = ""
        self.query_one("#kp_input", Input).value = ""
        self.query_one("#ki_input", Input).value = ""
        
    
    async def _action_reload_driver_parameters(self):
        
        self._clear_all_widget_data()
        # await asyncio.sleep(dlt)
        
        power_en = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_POWER_EN)
        power_switch = self.query_one("#power_switch", Switch)
        power_switch.value = True if power_en == 1 else False
        
        is_reverse = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_IS_REVERSE)
        direction_switch = self.query_one("#direction_switch", Switch)
        direction_switch.value = True if is_reverse == 1 else False

        
        target_speed = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_TARGET_SPEED)
        target_speed_input = self.query_one("#target_speed_input", Input)
        target_speed_input.value = f'{target_speed:.2f}' if target_speed is not None else ""

        control_mode = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_CONTROL_MODE)
        control_type_select = self.query_one("#control_type_select", Select)
        if control_mode == 0:
            control_type_select.value = "PWM"
        elif control_mode == 1:
            control_type_select.value = "PI Speed"
        else:
            control_type_select.value = Select.BLANK
                         
        
        pwm_compare = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_PWM_COMPARE)
        pwm_input = self.query_one("#pwm_input", Input)
        pwm_input.value = f'{pwm_compare}' if pwm_compare is not None else ""
        
        p_value = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_P_VALUE)
        kp_input = self.query_one("#kp_input", Input)
        kp_input.value = f'{p_value:.2f}' if p_value is not None else ""
        
        i_value = await self.motor_driver.execute_command_async(MotorDriver.CMDS.CMD_GET_I_VALUE)
        ki_input = self.query_one("#ki_input", Input)
        ki_input.value = f'{i_value:.2f}' if i_value is not None else ""
        print("Reloaded")
    
    



if __name__ == "__main__":
    app = MotorControlApp()
    app.run()
