#include "Command_Parser.h"


void split_cmd_para(char *cmd_line){
    uint16_t line_index=0;
    uint16_t i;

    // store the command and parameter in the global variable
    // ignore any space before between and after 
    
    // skip the begin space
    while(cmd_line[line_index] == ' '){
        line_index++;
    }

    // store the command
    i=0;
    while(cmd_line[line_index] != ' ' && cmd_line[line_index] != '\0'){
        _command[i++] = cmd_line[line_index++];
        if(i == COMMAND_BUFFER_SIZE-1){
            break;
        }
    }
    _command[i] = '\0';

    // skip the space between command and parameter
    while(cmd_line[line_index] == ' '){
        line_index++;
    }

    // store the parameter
    i=0;
    while(cmd_line[line_index] != ' ' && cmd_line[line_index] != '\0'){
        _parameter[i++] = cmd_line[line_index++];
        if(i == PARAMETER_BUFFER_SIZE-1){
            break;
        }
    }
    _parameter[i] = '\0';
}



void execute_command(char *cmd_line) {
    // 分割指令名稱與參數
    split_cmd_para(cmd_line);

    char *_endptr;
    // # speed
    if(strcmp(_command, "set_speed") == 0){
        long int_val = strtol(_parameter, &_endptr, 10);
        if(*_endptr!='\0' || _parameter[0]=='\0'){
            uart_print(">error, parameter to int error\n");
        }
        else{
            uart_printf(">success, set speed to %d\n", int_val);
            speed = int_val;
        }
    }
    else if(strcmp(_command, "get_speed") == 0){
        uart_printf(">success %d, rpm\n", speed);
    }

    // # current_speed
    else if(strcmp(_command, "get_current_speed") == 0){
        uart_printf(">success %d, rpm\n", current_speed);
    }

    // # power_en
    else if(strcmp(_command, "set_power_en") == 0){
        if(strcmp(_parameter, "1") == 0){
            power_en = 1;
            uart_print(">success, power ON\n");
        }
        else if(strcmp(_parameter, "0") == 0){
            power_en = 0;
            uart_print(">success, power OFF\n");
        }
        else{
            uart_print(">error, parameter should be 1 or 0\n");
        }
    }

    else if(strcmp(_command, "get_power_en") == 0){
        uart_printf(">success %d\n, power is %s", power_en, power_en ? "ON" : "OFF");
    }

    // # p_value
    else if(strcmp(_command, "set_p") == 0){
        float float_val = strtof(_parameter, &_endptr);
        if(*_endptr!='\0' || _parameter[0]=='\0'){
            uart_print(">error, parameter to float error\n");
        }
        else{
            uart_printf(">success, set P value to %.2f\n", float_val);
            p_value = float_val;
        }
    }

    else if(strcmp(_command, "get_p") == 0){
        uart_printf(">success %.6f\n, P value", p_value);
    }
    
    // # i_value
    else if(strcmp(_command, "set_i") == 0){
        float float_val = strtof(_parameter, &_endptr);
        if(*_endptr!='\0' || _parameter[0]=='\0'){
            uart_print(">error, parameter to float error\n");
        }
        else{
            uart_printf(">success, set I value to %.2f\n", float_val);
            i_value = float_val;
        }
    }

    else if(strcmp(_command, "get_i") == 0){
        uart_printf(">success %f\n, I value", i_value);
    }


    
    else{
        uart_printf("error, command not found:%s\n", _command);
    }
}
