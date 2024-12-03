#include "driverlib.h"
#include "gpio.h"
#include "intrinsics.h"
#include "msp430f5529.h"
#include "pmap.h"
#include "timer_a.h"
#include "ucs.h"
// #include <cstdint>

/*
AB->AC->BC->BA->CA->CB

2.4 BH AH
2.5 BL AL
1.4 GH BH
1.5 GL BL 
1.2 YH CH
1.3 YL CL

1 AH BL
2 AH CL
3 BH CL
4 BH AL
5 CH AL
6 CH BL

*/
void main (void)
{
    WDT_A_hold(WDT_A_BASE);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4|GPIO_PIN5);

    // SMCLK P2.2
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN2);

    // 12000,336 -> DCO 24M
    UCS_initFLLSettle(12000, 366);

    UCS_initClockSignal(
        UCS_SMCLK,
        UCS_DCOCLK_SELECT,
        UCS_CLOCK_DIVIDER_4
    );


    Timer_A_initUpModeParam timer_a0_para = {0};
    timer_a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    timer_a0_para.timerPeriod = 100;
    timer_a0_para.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    timer_a0_para.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    timer_a0_para.timerClear = TIMER_A_DO_CLEAR;
    timer_a0_para.startTimer = true;
    // Timer_A_initUpMode(TIMER_A0_BASE, &timer_a0_para);


    Timer_A_initCompareModeParam timer_a0_comp_para = {0};
    timer_a0_comp_para.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    timer_a0_comp_para.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a0_comp_para.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a0_comp_para.compareValue = 10;
    // Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a0_comp_para);

    uint8_t state = 0;

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
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3|GPIO_PIN4);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                // 0 AH BL 2.4 1.5
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN5);//15
                GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);//24
                break;
            case 1:
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN4|GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                // 1 AH CL 2.4 1.3
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN3);//13
                GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);//24
                break;
            case 2:
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4|GPIO_PIN5);
                // 2 BH CL 1.4 1.3
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN3|GPIO_PIN4);//13,14
                break;
            case 3:
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2|GPIO_PIN3|GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4);
                // 3 BH AL 1.4 2.5
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN4);//14
                GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN5);//25
                break;
            case 4:
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3|GPIO_PIN4|GPIO_PIN5);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4);
                // 4 CH AL 1.2 2.5
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN2);//12
                GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN5);//25
                break;
            case 5:
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3|GPIO_PIN4);
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4|GPIO_PIN5);
                // 5 CH BL 1.2 1.5
                GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN2|GPIO_PIN5);//12,15
                break;
                
        }
        __delay_cycles(60000);
    }    

    __no_operation();

}



























