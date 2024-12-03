#include "driverlib.h"
#include "command_parser.h"
#include "gpio.h"
#include "uart.h"
#include "BLDC_Driver.h"
#include "ucs.h"


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
    // mclk -> 12M, (DCO -> 24M)
    // smclk -> 12M (DCO/2)
    // UCS_initFLLSettle(12000, 366); // DCO 24M // for 100k interrupt too slow

    PMM_setVCore(PMM_CORE_LEVEL_3);

    // FLLREFCLK DEFAULT xt1(32768)
    UCS_initFLLSettle(25000, 763); // DCO 25M
    // UCS_initFLLSettle(24000, 732);


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
    
    while(1){

        if(is_new_frame_received()){
            // uart_printf("got new frame\n");

            uint8_t *rx_buffer = get_rx_buffer();
            uint8_t rx_length = get_rx_length();

            // uart_printf("frame len %d:", rx_length);
            // uart_write_bytes(rx_buffer, rx_length);
            // uart_print("\n");

            parse_command(rx_buffer, rx_length);
            rx_process_completed();
        }  

    }

}
