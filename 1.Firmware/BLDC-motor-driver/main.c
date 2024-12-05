#include "driverlib.h"
#include "BLDC_Driver.h"
#include "uart.h"
#include "command_parser.h"

/*
timer A0 A2 pwm for mos driver 
timer A0 UP mode CCR0 interrupt for motor controller
timer A1 Continuous ,CaptureMode read the hall motor speed from P2.0
timer B0 CCR0 interrupt for uart split frame

uart USCI A1 RX/TX command 
*/


inline void delay(uint32_t ms){
    // cpu clock 25M
    // n=s*25M, n=ms*25k
    uint32_t n;
    for(n=0; n<ms; n++){
        __delay_cycles(25000);
    }
    
}

void sys_clock_init(){
    /*
        mclk = 25M
        smclk = 12.5M
    */

    PMM_setVCore(PMM_CORE_LEVEL_3);

    // FLLREFCLK DEFAULT xt1(32768)
    UCS_initFLLSettle(25000, 763); // DCO 25M

    // 25M / 2 = 12.5M
    UCS_initClockSignal(
        UCS_SMCLK,
        UCS_DCOCLK_SELECT,
        UCS_CLOCK_DIVIDER_2
    );

    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN2);
}

void main (void){
    WDT_A_hold(WDT_A_BASE);
    sys_clock_init();
    uart_init();
    
    hall_sensor_init();
    MOSGateDriver_init();
    motor_speed_timer_init();

    while(1){

        // if new command receive from uart
        if(is_new_frame_received()){

            uint8_t *rx_buffer = get_rx_buffer();
            uint8_t rx_length = get_rx_length();

            parse_command(rx_buffer, rx_length);
            rx_process_completed();
        }  

    }

}
