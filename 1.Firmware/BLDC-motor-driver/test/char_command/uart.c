#include "uart.h"
#include "intrinsics.h"
#include "usci_a_uart.h"


void initQueue(CircularQueue *q) {
    q->head = 0;
    q->tail = 0;
}

int isEmpty(const CircularQueue *q) {
    return q->head == q->tail;
}

int isFull(const CircularQueue *q) {
    return ((q->tail + 1) % QUEUE_SIZE) == q->head;
}

void enqueue(CircularQueue *q, char data) {
    if (!isFull(q)) {
        q->buffer[q->tail] = data;
        q->tail = (q->tail + 1) % QUEUE_SIZE;
    }
}

char dequeue(CircularQueue *q) {
    if (!isEmpty(q)) {
        char ch = q->buffer[q->head];
        q->head = (q->head + 1) % QUEUE_SIZE;
        return ch;
    }
    return 0;
}

int readLineIfComplete(CircularQueue *q, char *line, int lineBufferSize) {
    int lineLen = 0;
    int index = q->head;
    int foundNewline = 0;
    char ch;

    // 查看佇列中是否有完整的一行
    while (index != q->tail) {
        ch = q->buffer[index];
        if (ch == '\n') {
            foundNewline = 1;
            break;
        }
        index = (index + 1) % QUEUE_SIZE;
    }

    // 只有當找到換行符時才從佇列中讀取一行
    if (foundNewline) {
        while(!isEmpty(q)) {
            ch = dequeue(q);
            if (ch == '\n') { // if first is '0' still return 0(line len)
                line[lineLen] = '\0';
                return lineLen;  // 返回讀取的行長度
            } else {
                if (lineLen < lineBufferSize - 1 && ch!='\r') {
                    line[lineLen++] = ch;
                }
            }
        }
    }
    
    return 0;
}



// UART
// only when uart interrupt disable 
void uart_print_block(char *str){ // print str
    while(*str){
        USCI_A_UART_transmitData(USCI_A1_BASE, *str);
        str++;
    }
}

void uart_printf_block(const char *format, ...) {
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

void uart_print(char *str){ // print str
    while(*str){
        enqueue(&tx_q, *str);
        str++;
    }
    
    if(tx_shutdown && !isEmpty(&tx_q)){
        USCI_A_UART_enableInterrupt(USCI_A1_BASE,USCI_A_UART_TRANSMIT_INTERRUPT_FLAG);
        USCI_A_UART_transmitData(USCI_A1_BASE, dequeue(&tx_q));
        tx_shutdown = false;
    }
    


}

void uart_printf(const char *format, ...) {
    char buffer[PRINTF_BUFFER_SIZE];          // 定義緩衝區
    va_list args;                      // 定義變數參數列表
    va_start(args, format);            // 初始化參數列表

    // 使用 snprintf 格式化字串到緩衝區
    int length = vsnprintf(buffer, PRINTF_BUFFER_SIZE, format, args);

    va_end(args);                      // 結束參數處理


    uart_print(buffer);  // 傳輸格式化字串

}

void uart_init(){
    initQueue(&rx_q);
    initQueue(&tx_q);
    tx_shutdown = true;

        //P3.4,5 = USCI_A1 TXD/RXD
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P4,
        GPIO_PIN4 + GPIO_PIN5
        );

    // smclk: 12M
    // baud = 115200, UCBR=104, UCBRS=1, UCBRF=0, (UCOS16 = 0) p953
    USCI_A_UART_initParam uart_init_para = {0};
    uart_init_para.selectClockSource = USCI_A_UART_CLOCKSOURCE_SMCLK;
    uart_init_para.clockPrescalar = 104;
    uart_init_para.firstModReg = 0;
    uart_init_para.secondModReg = 1;
    uart_init_para.parity = USCI_A_UART_NO_PARITY;
    uart_init_para.msborLsbFirst = USCI_A_UART_LSB_FIRST;
    uart_init_para.numberofStopBits = USCI_A_UART_ONE_STOP_BIT;
    uart_init_para.uartMode = USCI_A_UART_MODE;
    uart_init_para.overSampling = USCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;


    if (STATUS_FAIL == USCI_A_UART_init(USCI_A1_BASE, &uart_init_para)){
        while(1);
    }

    //Enable UART module for operation
    USCI_A_UART_enable(USCI_A1_BASE);

    //Enable Receive Interrupt
	USCI_A_UART_clearInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT|USCI_A_UART_TRANSMIT_INTERRUPT_FLAG );
    USCI_A_UART_enableInterrupt(USCI_A1_BASE,USCI_A_UART_RECEIVE_INTERRUPT);

    __enable_interrupt();

    // uart_print("[UART] init OK\n");    
}





#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void){
    switch (__even_in_range(UCA1IV,4)){
        case 2: //Vector 2 - RXIFG
            _uart_receive_char = USCI_A_UART_receiveData(USCI_A1_BASE);       
            // uart_printf("[USCI_A1_ISR] receive data: %d\n", _uart_receive_char);
            if(!isFull(&rx_q)){
                enqueue(&rx_q, _uart_receive_char);
            }
            break;
        
        case 4: //UCTXIFG, Transmit buffer empty;
            if(!isEmpty(&tx_q)){
                char ch = dequeue(&tx_q);
                USCI_A_UART_transmitData(USCI_A1_BASE, ch);
            }
            else{
                // empty shut down 
                // uart_print("shutdown!!\n");
                USCI_A_UART_disableInterrupt(USCI_A1_BASE,USCI_A_UART_TRANSMIT_INTERRUPT_FLAG);
                tx_shutdown = true;
            }
            break;

    }
}

