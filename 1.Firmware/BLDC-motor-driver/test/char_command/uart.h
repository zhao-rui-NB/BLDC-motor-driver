#ifndef __uart__
#define __uart__

#include "driverlib.h"
#include "intrinsics.h"
#include "stdbool.h"
#include "sys/cdefs.h"
#include <stdio.h>

#define PRINTF_BUFFER_SIZE 100

#define QUEUE_SIZE 150  // 佇列大小



typedef struct {
    char buffer[QUEUE_SIZE];
    int head;
    int tail;
} CircularQueue;




CircularQueue tx_q;
CircularQueue rx_q;
volatile bool tx_shutdown;


void initQueue(CircularQueue *q);
int isEmpty(const CircularQueue *q);
int isFull(const CircularQueue *q);
void enqueue(CircularQueue *q, char data);
char dequeue(CircularQueue *q);
char peekFront(const CircularQueue *q);
int readLineIfComplete(CircularQueue *q, char *line, int lineBufferSize);




void uart_print(char *str);
void uart_printf(const char *format, ...);
void uart_init();






#endif
