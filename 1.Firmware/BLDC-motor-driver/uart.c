#include "uart.h"

// ### uart print

void uart_print(char *str){ // print str
    while(*str){
        USCI_A_UART_transmitData(USCI_A1_BASE, *str);
        str++;
    }
}

void uart_write_bytes(uint8_t *array, uint8_t len){
    while(len--){
        USCI_A_UART_transmitData(USCI_A1_BASE, *array);
        array++;
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
        uart_printf("len %d\n",length);
    } else {
        uart_print(buffer);  // 傳輸格式化字串
    }
}

// ### uart init

void uart_init(){
    //P4.4, 4.5 = USCI_A1 TXD/RXD
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P4,
        GPIO_PIN4 + GPIO_PIN5
    );

    // smclk: 12M
    // baud = 115200, UCBR=104, UCBRS=1, UCBRF=0, (UCOS16 = 0) p953

    // smclk : 12.5M
    // The recommended parameters for DriverLib are:
    // clockPrescalar:6 // firstModReg:11 // secondModReg:2 // overSampling:1
    
    USCI_A_UART_initParam uart_init_para = {0};
    uart_init_para.selectClockSource = USCI_A_UART_CLOCKSOURCE_SMCLK;
    uart_init_para.clockPrescalar = 6;
    uart_init_para.firstModReg = 11;
    uart_init_para.secondModReg = 2;
    uart_init_para.parity = USCI_A_UART_NO_PARITY;
    uart_init_para.msborLsbFirst = USCI_A_UART_LSB_FIRST;
    uart_init_para.numberofStopBits = USCI_A_UART_ONE_STOP_BIT;
    uart_init_para.uartMode = USCI_A_UART_MODE;
    uart_init_para.overSampling = USCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    USCI_A_UART_init(USCI_A1_BASE, &uart_init_para);

    //Enable UART module for operation
    USCI_A_UART_enable(USCI_A1_BASE);

    //Enable Receive Interrupt
	USCI_A_UART_clearInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT);
    USCI_A_UART_enableInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT);

    uart_receive_interval_init(); // for split the receive frame
    clear_receive_frame();

    __enable_interrupt();
}

void uart_receive_interval_init(){
    /*
    // 12.5M clk / 12500 = 1k, 1ms interrupt counter 
    */
    // Timer_A_initUpModeParam timer_a1_para = {0};
    // timer_a1_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    // timer_a1_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    // timer_a1_para.timerPeriod = 12500;
    // timer_a1_para.timerClear = TIMER_A_DO_CLEAR;
    // timer_a1_para.startTimer = true;
    // timer_a1_para.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    // Timer_A_initUpMode(TIMER_A1_BASE, &timer_a1_para);


    //change to timer B 
    Timer_B_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_B_CLOCKSOURCE_SMCLK;
    initUpParam.clockSourceDivider = TIMER_B_CLOCKSOURCE_DIVIDER_1;
    initUpParam.timerPeriod = 12500;
    initUpParam.timerClear = TIMER_B_DO_CLEAR;
    initUpParam.startTimer = true;
    initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_B_CCIE_CCR0_INTERRUPT_ENABLE;
    Timer_B_initUpMode(TIMER_B0_BASE, &initUpParam);


}

// ### receive buffer

void clear_receive_frame(){
    _uart_receive_frame.rx_length = 0;
    _uart_receive_frame.rx_interval_counter = 0;
    _uart_receive_frame.frame_received = 0;
}

uint8_t is_new_frame_received(){
    return _uart_receive_frame.frame_received;
}

uint8_t get_rx_length(){
    return _uart_receive_frame.rx_length;
}

uint8_t *get_rx_buffer(){
    return _uart_receive_frame.rx_buffer;
}

void rx_process_completed(){
    clear_receive_frame();
}


#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR (void){
    __disable_interrupt();
    // each 1ms, count receive interval, if receive timeout write 1 to frame_receive 
    if(!_uart_receive_frame.frame_received){
        if(_uart_receive_frame.rx_interval_counter < RECEIVE_FRAME_INTERVAL){
            _uart_receive_frame.rx_interval_counter++;
        }

        if (_uart_receive_frame.rx_interval_counter >= RECEIVE_FRAME_INTERVAL && _uart_receive_frame.rx_length > 0){
            _uart_receive_frame.frame_received = 1;
        }
    }
    __enable_interrupt();
}


#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void){
    __disable_interrupt();
    // write the receive byte to buffer 
    switch (__even_in_range(UCA1IV,4)){
        case 2: //Vector 2 - RXIFG
            _uart_receive_char = USCI_A_UART_receiveData(USCI_A1_BASE);
            // if frame is not received and buffer not full, write to buffer
            if (!_uart_receive_frame.frame_received){ // frame process finish 
                if (_uart_receive_frame.rx_length < RX_BUFFER_SIZE){
                    _uart_receive_frame.rx_interval_counter = 0;
                    _uart_receive_frame.rx_buffer[_uart_receive_frame.rx_length] = _uart_receive_char;
                    _uart_receive_frame.rx_length++;
                }
                else{
                    // out of buffer
                    clear_receive_frame();
                }
            }
            break;
    }

    __enable_interrupt();
}

