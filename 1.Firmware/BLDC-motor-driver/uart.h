#ifndef __uart__
#define __uart__

#include "driverlib.h"
#include <stdio.h>
#include "gpio.h"
#include "msp430f5529.h"
#include "uart.h"


// uart buffer 
#define RX_BUFFER_SIZE 50
#define PRINTF_BUFFER_SIZE 50


// frame 間隔分割時間 ms ()
#define RECEIVE_FRAME_INTERVAL 5

// 通訊協議結構體
typedef struct {
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint8_t rx_length;
    uint16_t rx_interval_counter;
    uint8_t frame_received;
} UartReceiveFrame;

UartReceiveFrame _uart_receive_frame;
char _uart_receive_char;


void clear_receive_frame();
uint8_t is_new_frame_received();
uint8_t get_rx_length();
uint8_t *get_rx_buffer();
void rx_process_completed();



void uart_init();
void uart_receive_interval_init();
void uart_print(char *str);
void uart_printf(const char *format, ...);

void uart_write_bytes(uint8_t *array, uint8_t len);



#endif
