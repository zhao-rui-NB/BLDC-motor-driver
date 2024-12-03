
#include "driverlib.h"

// #include <cstdint>
#include <stdio.h>
#include <stdarg.h>


#define USCI_BASE USCI_A1_BASE 
#define PRINTF_BUFFER_SIZE 100


inline void delay(uint32_t ms){
    // cpu clock 12M
    // n=s*12M, n=ms*12k
    uint32_t n;
    for(n=0; n<ms; n++){
        __delay_cycles(12000);
    }
    
}

void uart_print(char *str){ // print str
    while(*str){
        USCI_A_UART_transmitData(USCI_A1_BASE, *str);
        str++;
    }
}

void uart_printf(const char *format, ...) {
    char buffer[PRINTF_BUFFER_SIZE];          // 定義緩衝區
    va_list args;                      // 定義變數參數列表
    va_start(args, format);            // 初始化參數列表

    // 使用 snprintf 格式化字串到緩衝區
    int length = vsnprintf(buffer, PRINTF_BUFFER_SIZE, format, args);

    va_end(args);                      // 結束參數處理

    // 檢查是否發生截斷
    if (length >= PRINTF_BUFFER_SIZE) {
        uart_print("Buffer overflow occurred!\r\n");
    } else {
        uart_print(buffer);  // 傳輸格式化字串
    }
}


void main (void)
{
    //Stop WDT
    WDT_A_hold(WDT_A_BASE);

    // GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2, GPIO_PIN2);


    GPIO_setAsPeripheralModuleFunctionOutputPin(
        GPIO_PORT_P2,
        GPIO_PIN2
    );

    // DCO 24M
    UCS_initFLLSettle(12000, 366);

    UCS_initClockSignal(
        UCS_SMCLK,
        UCS_DCOCLK_SELECT,
        UCS_CLOCK_DIVIDER_2
    );



    //P3.4,5 = USCI_A1 TXD/RXD
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P4,
        GPIO_PIN4 + GPIO_PIN5
        );

    // smclk: 12M
    // baud = 115200, UCBR=104, UCBRS=1, UCBRF=0, (UCOS16 = 0) p953
    USCI_A_UART_initParam param = {0};
    param.selectClockSource = USCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 104;
    param.firstModReg = 0;
    param.secondModReg = 1;
    param.parity = USCI_A_UART_NO_PARITY;
    param.msborLsbFirst = USCI_A_UART_LSB_FIRST;
    param.numberofStopBits = USCI_A_UART_ONE_STOP_BIT;
    param.uartMode = USCI_A_UART_MODE;
    param.overSampling = USCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;

    if (STATUS_FAIL == USCI_A_UART_init(USCI_A1_BASE, &param)){
        return;
    }

    //Enable UART module for operation
    USCI_A_UART_enable(USCI_A1_BASE);

    //Enable Receive Interrupt
	USCI_A_UART_clearInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT);
    USCI_A_UART_enableInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT);

    __enable_interrupt();


    uart_print("[UART] init OK\n");


    while(1){
        uint16_t time=0;
        while(1){
            uart_printf("now time: %d\n", time);
            delay(500);
            time++;
        }
    }



}

char _receive_char;

#pragma vector=USCI_A1_VECTOR
__interrupt
void USCI_A1_ISR(void){
    switch (__even_in_range(UCA1IV,4)){
        case 2: //Vector 2 - RXIFG
            _receive_char = USCI_A_UART_receiveData(USCI_A1_BASE);       
            uart_printf("[USCI_A1_ISR] receive data: %c\n", _receive_char);
            break;

    }


}


