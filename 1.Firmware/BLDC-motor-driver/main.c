#include "driverlib.h"
#include "BLDC_Driver.h"
#include "electric_throttle.h"
#include "gpio.h"
#include "intrinsics.h"
#include "uart.h"
#include "command_parser.h"

#include "electric_throttle.h"

/*
timer A0 A2 pwm for mos driver 
timer A0 UP mode CCR0 interrupt for motor controller
timer A1 Continuous ,CaptureMode read the hall motor speed from P2.0
timer B0 CCR0 interrupt for uart split frame

uart USCI A1 RX/TX command 

adc p6.1(A1) throttle hall
GPIO 1.0 throttle enable button
GPIO 2.1 throttle enable state led

// #### BLDC ####
// timer on mos driver pin
// P1.2 TA0.1 CH PWM
// P1.3 TA0.2 CL
// P1.4 TA0.3 BH PWM
// P1.5 TA0.4 BL
// P2.4 TA2.1 AH PWM
// P2.5 TA2.2 AL

// mos driver pin
// 2.4 BH AH
// 2.5 BL AL
// 1.4 GH BH
// 1.5 GL BL 
// 1.2 YH CH
// 1.3 YL CL

*/


inline void delay(uint32_t ms){
    // cpu clock 25M
    // n=s*25M, n=ms*25k
    uint32_t n;
    for(n=0; n<ms; n++){
        __delay_cycles(25000);
    }
    
}

// map the two number range , src_low to src_high to dest_low to dest_high and value
inline float map(float value, float src_low, float src_high, float dest_low, float dest_high){
    return (value - src_low) * (dest_high - dest_low) / (src_high - src_low) + dest_low;
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


    adc_init();
    throttle_en_btn_init();

    while(1){
        
        // uart_printf("is_throttle_en_btn_press :%d\n", GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN1) );
        if(is_throttle_en_btn_press()){ // enable or disable throttle
            _delay_cycles(2500);
            while(is_throttle_en_btn_press());
            throttle_en = !throttle_en;
            if(throttle_en)
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
            else
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

            if(throttle_en==0){
                set_control_mode(MODE_PI_SPEED);
                set_target_speed(3500);
            }

        }

        // control by throttle 
        if(throttle_en && new_throttle_val){
            new_throttle_val = 0;
            // get now motor control mode 
            uint8_t control_mode = get_control_mode();
            if(control_mode == MODE_MANUAL_PWM){
                uint16_t pwm = map(throttle_val, min_throttle_adc, max_throttle_adc, 0, PWM_MAX_COMPARE_VALUE);
                set_pwm_compare(pwm);
            }
            else if(control_mode == MODE_PI_SPEED){
                float speed = map(throttle_val, min_throttle_adc, max_throttle_adc, 0, 6000);
                // uart_printf("throttle_val: %d\n", (int)(throttle_val));
                // uart_printf("map: %d\n", (int)(speed));
                set_target_speed(speed);
            }
        }


        // if new command receive from uart
        if(is_new_frame_received()){

            uint8_t *rx_buffer = get_rx_buffer();
            uint8_t rx_length = get_rx_length();

            parse_command(rx_buffer, rx_length);
            rx_process_completed();
        }  

    }

}
