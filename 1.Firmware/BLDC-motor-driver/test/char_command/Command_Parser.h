#ifndef __Command_Parser__
#define __Command_Parser__



#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"

#include "BLDC_Driver.h"


#define COMMAND_BUFFER_SIZE 50
#define PARAMETER_BUFFER_SIZE 50 

char _command[COMMAND_BUFFER_SIZE];
char _parameter[PARAMETER_BUFFER_SIZE];


//     // 測試指令
//     const char *test_commands[] = {
//         "set_speed 1000",
//         "get_speed",
//         "get_current_speed",
//         "set_power_en",
//         "get_power_en",
//         "set_p 5.0",
//         "get_p",
//         "set_i 0.2",
//         "get_i",
//         "unknown_command",
//         NULL
//     };

void execute_command(char *cmd_line);


#endif
