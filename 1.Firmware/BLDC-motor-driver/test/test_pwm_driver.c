#include "driverlib.h"
#include "gpio.h"
#include "timer_a.h"
// #include <cstdint>
/*
P1.2 TA0.1 CH
P1.3 TA0.2 CL
P1.4 TA0.3 BH
P1.5 TA0.4 BL 
P2.4 TA2.1 AH
P2.5 TA2.2 AL
*/

inline void delay(uint32_t ms){
    // cpu clock 12M
    // n=s*12M, n=ms*12k
    uint32_t n;
    for(n=0; n<ms; n++){
        __delay_cycles(12000);
    }
    
}

void set_phaseA_output(int16_t value){
    if(value<0){
        GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3);
    }
    else{
        
    }
}

void main (void){
    WDT_A_hold(WDT_A_BASE);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4|GPIO_PIN5);

    // GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);


    // SMCLK P2.2
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN2);
    
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3);

    // 12000,336 -> DCO 24M
    UCS_initFLLSettle(12000, 366);

    UCS_initClockSignal(
        UCS_SMCLK,
        UCS_DCOCLK_SELECT,
        UCS_CLOCK_DIVIDER_1
    );


    // 24M clk / 240 = 100k
    Timer_A_initUpDownModeParam a0_para = {0};
    a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    a0_para.timerPeriod = 240;
    a0_para.timerClear = TIMER_A_DO_CLEAR;

    Timer_A_initUpDownMode(TIMER_A0_BASE, &a0_para);


    // TOGGLE_SET > TOGGLE_RESET
    // TOGGLE_SET   duty : 1 - (compareValue / 240)
    // TOGGLE_RESET duty : compareValue / 240



    Timer_A_initCompareModeParam timer_a0_comp_para_ta01 = {0};
    timer_a0_comp_para_ta01.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    timer_a0_comp_para_ta01.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a0_comp_para_ta01.compareOutputMode = TIMER_A_OUTPUTMODE_TOGGLE_SET;
    timer_a0_comp_para_ta01.compareValue = 100;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a0_comp_para_ta01);



    Timer_A_initCompareModeParam timer_a0_comp_para_ta02 = {0};
    timer_a0_comp_para_ta02.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
    timer_a0_comp_para_ta02.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a0_comp_para_ta02.compareOutputMode = TIMER_A_OUTPUTMODE_TOGGLE_RESET;
    timer_a0_comp_para_ta02.compareValue = 100;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a0_comp_para_ta02);

    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UPDOWN_MODE); 




    // uint8_t state = 0;

    while(1){
        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3);
        delay(100);
        // GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3);
        // delay(100);
        
        Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UPDOWN_MODE); 
        delay(100);

        Timer_A_stop(TIMER_A0_BASE);
        delay(100);


    }    

    __no_operation();

}



























