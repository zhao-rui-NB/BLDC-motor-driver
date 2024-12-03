#include "BLDC_Driver.h"
#include "gpio.h"


/*
    sys clock for pwm timer
    WDT_A_hold(WDT_A_BASE);
    // 12000,336 -> DCO 24M
    UCS_initFLLSettle(12000, 366);
    UCS_initClockSignal(UCS_SMCLK, UCS_DCOCLK_SELECT, UCS_CLOCK_DIVIDER_1);
*/



// ## setting value
volatile  uint8_t power_en = 1;
uint8_t is_reverse = 1;
uint8_t control_mode = MODE_MANUAL_PWM;

// pwm control
uint16_t pwm_compare = DEFAULT_COMPARE_VALUE;

// pid speed control
float target_speed = 100;
float p_value = 1;
float i_value = 0;

// ## measure value
float current_speed;

void MOSGateDriver_init(){
    // 
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3|GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);

    // 24M clk / 240 = 100k
    // 12.5M / 1 / 125 = 100k
    Timer_A_initUpModeParam timer_a0_para = {0};
    timer_a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    // timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_5; // 20k
    timer_a0_para.timerPeriod = PWM_TIMER_PERIOD;
    timer_a0_para.timerClear = TIMER_A_DO_CLEAR;
    timer_a0_para.startTimer = true;
    Timer_A_initUpMode(TIMER_A2_BASE, &timer_a0_para);
    // interrupt for pid 
    timer_a0_para.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    Timer_A_initUpMode(TIMER_A0_BASE, &timer_a0_para);

    Timer_A_initCompareModeParam timer_a_comp_para_ta01 = {0};
    timer_a_comp_para_ta01.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    timer_a_comp_para_ta01.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta01.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta01.compareValue = DEFAULT_COMPARE_VALUE;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a_comp_para_ta01);

    Timer_A_initCompareModeParam timer_a_comp_para_ta03 = {0};
    timer_a_comp_para_ta03.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_3;
    timer_a_comp_para_ta03.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta03.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta03.compareValue = DEFAULT_COMPARE_VALUE;
    Timer_A_initCompareMode(TIMER_A0_BASE, &timer_a_comp_para_ta03);


    Timer_A_initCompareModeParam timer_a_comp_para_ta21 = {0};
    timer_a_comp_para_ta21.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    timer_a_comp_para_ta21.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    timer_a_comp_para_ta21.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    timer_a_comp_para_ta21.compareValue = DEFAULT_COMPARE_VALUE;
    Timer_A_initCompareMode(TIMER_A2_BASE, &timer_a_comp_para_ta21);
}

volatile uint32_t count_hall = 0;
volatile uint32_t count_pwm_interrupt = 0;
volatile uint8_t last_hall_data = 0;


#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void){
    __disable_interrupt();

    if(count_pwm_interrupt<25000){
        count_pwm_interrupt++;
    }
    else{
        GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        current_speed = (float)count_hall/24*60;
        count_pwm_interrupt = 0;
        count_hall = 0;
    }

    // BLDC_controller_update();  
    if(!power_en){
        MOSGateDriver_write_disable();
        return;
    }else {
        uint8_t hall_data = read_hall_sensor();
        MOSGateDriver_by_hall_sensor(hall_data);

        if(hall_data!=last_hall_data){
            count_hall++;
            last_hall_data = hall_data;
        }
    }
    
    if(control_mode == MODE_MANUAL_PWM){
        MOSGateDriver_write_duty(pwm_compare);
        return;
    }
    else if(control_mode == MODE_PI_SPEED){
        // speed control
        float error = target_speed - current_speed;
        float duty = p_value * error + i_value * error;
        if(duty < 0){
            duty = 0;
        }
        else{
            uint16_t uint_duty = (uint16_t)duty;
            MOSGateDriver_write_duty(uint_duty);
        }
    }
    __enable_interrupt();
}


void MOSGateDriver_write_duty(uint16_t cmp_value){
    if(cmp_value > PWM_MAX_COMPARE_VALUE){
        cmp_value = PWM_MAX_COMPARE_VALUE;
    }

    // ta 01 03 21
    TA0CCR1 = cmp_value;
    TA0CCR3 = cmp_value;
    TA2CCR1 = cmp_value;
}

inline void MOSGateDriver_write_disable(){
    write_en_AhAlBhBlChCl(0, 0, 0, 0, 0, 0);
}

inline void MOSGateDriver_write_step(uint8_t step){ // 0~5 //-1 disable all
    switch(step){
        // 0 AH BL 2.4 1.5
        case 0: write_en_AhAlBhBlChCl(1, 0, 0, 1, 0, 0); break;

        // 1 AH CL 2.4 1.3
        case 1: write_en_AhAlBhBlChCl(1, 0, 0, 0, 0, 1); break;

        // 2 BH CL 1.4 1.3
        case 2: write_en_AhAlBhBlChCl(0, 0, 1, 0, 0, 1); break;

        // 3 BH AL 1.4 2.5
        case 3: write_en_AhAlBhBlChCl(0, 1, 1, 0, 0, 0); break;

        // 4 CH AL 1.2 2.5
        case 4: write_en_AhAlBhBlChCl(0, 1, 0, 0, 1, 0); break;

        // 5 CH BL 1.2 1.5
        case 5: write_en_AhAlBhBlChCl(0, 0, 0, 1, 1, 0); break;

        default: write_en_AhAlBhBlChCl(0, 0, 0, 0, 0, 0); break;
    }
}


void hall_sensor_init(){
    // hall Y2.0 G2.6 B2.3 
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN6|GPIO_PIN3);
}


// MSP430 port interrupt can not both pos  and neg edge  
// void hall_sensor_interrupt_init(){
//     GPIO_enableInterrupt(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN3|GPIO_PIN6);
//     GPIO_selectInterruptEdge(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN3|GPIO_PIN6, 

// }


uint8_t read_hall_sensor(){
    uint8_t hall_y = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN0);
    uint8_t hall_g = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN6);
    uint8_t hall_b = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN3);
    uint8_t hall_encode = hall_y<<2 | hall_g<<1 | hall_b;
    return hall_encode;
}


void MOSGateDriver_by_hall_sensor(uint8_t hall_encode){

    if(!power_en){
        return;
    }

    hall_encode = (HAll_SENSOR_ENCODE_REVERSE&1)<<3 | hall_encode;

    switch(hall_encode){
        case 5: MOSGateDriver_write_step((0+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 4: MOSGateDriver_write_step((1+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 6: MOSGateDriver_write_step((2+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 2: MOSGateDriver_write_step((3+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 3: MOSGateDriver_write_step((4+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1: MOSGateDriver_write_step((5+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;

        case 1<<3 | 1: MOSGateDriver_write_step((0+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1<<3 | 3: MOSGateDriver_write_step((1+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1<<3 | 2: MOSGateDriver_write_step((2+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1<<3 | 6: MOSGateDriver_write_step((3+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1<<3 | 4: MOSGateDriver_write_step((4+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
        case 1<<3 | 5: MOSGateDriver_write_step((5+HALL_SENSOR_ENCODE_OFFSET+3*(is_reverse&1))%6); break;
    }

}


void BLDC_controller_init(){
    if(!power_en){
        MOSGateDriver_write_disable();
        return;
    }
    
    if(control_mode == MODE_MANUAL_PWM){
        MOSGateDriver_write_duty(pwm_compare);
        return;
    }
    else if(control_mode == MODE_PI_SPEED){
        // speed control
        float error = target_speed - current_speed;
        float duty = p_value * error + i_value * error;
        if(duty < 0){
            duty = 0;
        }
        else{
            uint16_t uint_duty = (uint16_t)duty;
            MOSGateDriver_write_duty(uint_duty);
        }
    }

}

inline void BLDC_controller_update(){
    if(!power_en){
        MOSGateDriver_write_disable();
        return;
    }
    
    if(control_mode == MODE_MANUAL_PWM){
        MOSGateDriver_write_duty(pwm_compare);
        return;
    }
    else if(control_mode == MODE_PI_SPEED){
        // speed control
        float error = target_speed - current_speed;
        float duty = p_value * error + i_value * error;
        if(duty < 0){
            duty = 0;
        }
        else{
            uint16_t uint_duty = (uint16_t)duty;
            MOSGateDriver_write_duty(uint_duty);
        }
    }

}



// ###############################################
// ####### api for control #######################
// ###############################################

// now just print the action first , for test
void set_power_en(uint8_t en){
    power_en = en & 0x01;
}

uint8_t get_power_en(){
    return power_en;
}


void set_target_speed(float speed){
    target_speed = speed;
}

float get_target_speed(){
    return target_speed;
}

void set_is_reverse(uint8_t reverse){
    is_reverse = reverse;
}

uint8_t get_is_reverse(){
    return is_reverse;
}

void set_control_mode(uint8_t mode){
    control_mode = mode;
}

uint8_t get_control_mode(){
    return control_mode;
}

void set_p_value(float p){
    p_value = p;
}

float get_p_value(){
    return p_value;
}

void set_i_value(float i){
    i_value = i;
}

float get_i_value(){
    return i_value;
}

void set_pwm_compare(uint16_t compare){
    pwm_compare = compare;
}

uint16_t get_pwm_compare(){
    return pwm_compare;
}

// measure read only
float get_current_speed(){
    return current_speed;
}






























