#include "driverlib.h"
#include "intrinsics.h"
#include "ucs.h"
// #include <cstdint>

/*
P1.2 TA0.1 CH
P1.3 TA0.2 CL PWM
P1.4 TA0.3 BH
P1.5 TA0.4 BL PWM
P2.4 TA2.1 AH
P2.5 TA2.2 AL PWM
*/

// 定義控制腳位啟用/停用
#define write_en_AH(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN4) : GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4))
#define write_en_AL(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN5) : GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5))

#define write_en_BH(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4))
#define write_en_BL(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN5) : GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5))

#define write_en_CH(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2))
#define write_en_CL(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN3) : GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3))

inline void delay(uint32_t ms){
    // cpu clock 12M
    // n=s*12M, n=ms*12k
    uint32_t n;
    for(n=0; n<ms; n++){
        __delay_cycles(12000);
    }
    
}

void main(void){

    WDT_A_hold(WDT_A_BASE);
    // GPIO_setAsOutputPin
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4);

    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);


    // // SMCLK P2.2

    // 12000,336 -> DCO 24M
    UCS_initFLLSettle(12000, 366);

    UCS_initClockSignal(
        UCS_SMCLK,
        UCS_DCOCLK_SELECT,
        UCS_CLOCK_DIVIDER_16
    );


    // Timer_A_initUpDownModeParam a0_para = {0};
    // a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    // a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    // a0_para.timerPeriod = 240;
    // a0_para.timerClear = TIMER_A_DO_CLEAR;

    // Timer_A_initUpDownMode(TIMER_A0_BASE, &a0_para);



    // 24M clk / 240 = 100k
    Timer_A_initUpModeParam timer_a0_para = {0};
    timer_a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    timer_a0_para.timerPeriod = 240;
    timer_a0_para.timerClear = TIMER_A_DO_CLEAR;
    timer_a0_para.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &timer_a0_para);
    Timer_A_initUpMode(TIMER_A2_BASE, &timer_a0_para);


    Timer_A_initCompareModeParam timer_a_comp_para_ta02 = {0};
    timer_a_comp_para_ta02.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
    timer_a_comp_para_ta02.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta02.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta02.compareValue = 120;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a_comp_para_ta02);

    Timer_A_initCompareModeParam timer_a_comp_para_ta04 = {0};
    timer_a_comp_para_ta04.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_4;
    timer_a_comp_para_ta04.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta04.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta04.compareValue = 120;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a_comp_para_ta04);


    Timer_A_initCompareModeParam timer_a_comp_para_ta22 = {0};
    timer_a_comp_para_ta22.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
    timer_a_comp_para_ta22.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta22.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta22.compareValue = 120;
    Timer_A_initCompareMode(TIMER_A2_BASE, &timer_a_comp_para_ta22);


/*
P1.2 TA0.1 CH
P1.3 TA0.2 CL PWM
P1.4 TA0.3 BH
P1.5 TA0.4 BL PWM
P2.4 TA2.1 AH
P2.5 TA2.2 AL PWM
*/





    uint8_t state=0;
    while(1){
        state++;
        if(state>=6){
            state = 0;
        }

        // 2.4 BH AH
        // 2.5 BL AL
        // 1.4 GH BH
        // 1.5 GL BL 
        // 1.2 YH CH
        // 1.3 YL CL

        switch(state){
            case 0:
                // 0 AH BL 2.4 1.5
                write_en_AL(0);
                write_en_BH(0);
                write_en_CH(0);
                write_en_CL(0);
                __delay_cycles(10);
                write_en_AH(1);
                write_en_BL(1);
                break;

            case 1:
                // 1 AH CL 2.4 1.3
                write_en_AL(0);
                write_en_BH(0);
                write_en_BL(0);
                write_en_CH(0);
                __delay_cycles(10);
                write_en_AH(1);
                write_en_CL(1);
                break;
            case 2:
                // 2 BH CL 1.4 1.3
                write_en_AH(0);
                write_en_AL(0);
                write_en_BL(0);
                write_en_CH(0);
                __delay_cycles(10);
                write_en_BH(1);
                write_en_CL(1);
                break;
            case 3:
                // 3 BH AL 1.4 2.5
                write_en_AH(0);
                write_en_BL(0);
                write_en_CH(0);
                write_en_CL(0);
                __delay_cycles(10);
                write_en_BH(1);
                write_en_AL(1);
                break;
            case 4:
                // 4 CH AL 1.2 2.5
                write_en_AH(0);
                write_en_BH(0);
                write_en_BL(0);
                write_en_CL(0);
                __delay_cycles(10);
                write_en_CH(1);
                write_en_AL(1);
                break;
            case 5:
                // 5 CH BL 1.2 1.5
                write_en_AH(0);
                write_en_AL(0);
                write_en_BH(0);
                write_en_CL(0);
                __delay_cycles(10);
                write_en_CH(1);
                write_en_BL(1);
                break;
                
        }
        // __delay_cycles(70000);
        delay(1000);
    }    

}


























