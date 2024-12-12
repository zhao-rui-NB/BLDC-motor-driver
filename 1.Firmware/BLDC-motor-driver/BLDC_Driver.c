#include "BLDC_Driver.h"
#include "gpio.h"
#include "sys/cdefs.h"
#include "timer_a.h"



// ## setting value
volatile  uint8_t power_en = 1;
uint8_t is_reverse = 1;
// uint8_t control_mode = MODE_MANUAL_PWM;
uint8_t control_mode = MODE_PI_SPEED;


// pwm control
uint16_t pwm_compare = DEFAULT_COMPARE_VALUE;

// pid speed control
float target_speed = 2000;
float p_value = 0.06;
float i_value = 0.02;

// ## measure value
float current_speed;

// clac the motor speed
volatile uint32_t count_hall = 0;
volatile uint32_t count_pwm_interrupt = 0;
volatile uint8_t hall_data = 0;
volatile uint8_t last_hall_data = 0;

// pi controller
volatile float _integral = 0;



// ### mos gate driver && timer A0 controller interrupt

void MOSGateDriver_init(){
    /*
        init timer A0 and A2 for mos pwm 
        when PWM_TIMER_PERIOD==500
        clk smclk 12.5M / 500 = 25k hz 
        timer A0 also interrupt at 25k for controller
    */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3|GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);

    Timer_A_initUpModeParam timer_a0_para = {0};
    timer_a0_para.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    // timer_a0_para.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_10; // ##########################
    timer_a0_para.timerPeriod = PWM_TIMER_PERIOD;
    timer_a0_para.timerClear = TIMER_A_DO_CLEAR;
    timer_a0_para.startTimer = true;
    Timer_A_initUpMode(TIMER_A2_BASE, &timer_a0_para);
    // interrupt for pid and bldc controller
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

inline void MOSGateDriver_write_step(uint8_t step){ // 0~5
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

// ### hall sensor

void hall_sensor_init(){
    // hall Y2.0 G2.6 B2.3 
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN6|GPIO_PIN3);
}

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

// contorller in ISR

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void){ // interrupt freq 25k hz
    __disable_interrupt();

    hall_data = read_hall_sensor();
    if(!power_en)
        MOSGateDriver_write_disable();
    else
        MOSGateDriver_by_hall_sensor(hall_data);
    
    // 2500 is 0.1s
    if(count_pwm_interrupt<2500){  // pass time counter, when 25000, pass 1s
        count_pwm_interrupt++;
    }
    else{ // time up, calc the motor speed 

        // GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        count_pwm_interrupt = 0;

        if(control_mode == MODE_MANUAL_PWM){
            MOSGateDriver_write_duty(pwm_compare);
        }
        else if(control_mode == MODE_PI_SPEED){// speed control
            float error = target_speed - current_speed; // 計算當前誤差
            _integral += error; // 積分

            if (_integral > INTEGRAL_MAX) // 防止積分飽和（加入積分限制）
                _integral = INTEGRAL_MAX;
            else if (_integral < INTEGRAL_MIN) 
                _integral = INTEGRAL_MIN;

            float duty = p_value * error + i_value * _integral; // 計算輸出
            duty = pwm_compare + duty;
            if(duty<MIN_PI_CONTROLLER_WRITE_DUTY)
                duty = MIN_PI_CONTROLLER_WRITE_DUTY;
            else if (duty>PWM_MAX_COMPARE_VALUE)
                duty = PWM_MAX_COMPARE_VALUE;

            pwm_compare = duty;
            MOSGateDriver_write_duty(pwm_compare); // direct write the duty is ok, function will limit the value

        }
    
    }
    __enable_interrupt();
}


// motor speed timer 
void motor_speed_timer_init(){
    Timer_A_initContinuousModeParam initContParam_TA1 = {0};
    initContParam_TA1.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // 12.5M 
    initContParam_TA1.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_8;
    initContParam_TA1.timerClear = TIMER_A_DO_CLEAR;
    initContParam_TA1.startTimer = true;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam_TA1);

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2, GPIO_PIN0); // read hall freq

    Timer_A_initCaptureModeParam initCaptureModeParam_TA1 = {0};
    initCaptureModeParam_TA1.captureInputSelect = TIMER_A_CAPTURE_INPUTSELECT_CCIxA;
    initCaptureModeParam_TA1.captureInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    initCaptureModeParam_TA1.captureMode = TIMER_A_CAPTUREMODE_RISING_EDGE;
    initCaptureModeParam_TA1.captureRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    initCaptureModeParam_TA1.synchronizeCaptureSource = TIMER_A_CAPTURE_SYNCHRONOUS;

    Timer_A_initCaptureMode(TIMER_A1_BASE, &initCaptureModeParam_TA1);

}


#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER1_A0_ISR (void){
    __disable_interrupt();
    // write the receive byte to buffer 
    switch (__even_in_range(TA1IV,2)){
        case  2:
            /*
                pre_rev_time = cnt / (12.5M/8) * 4 s, 4 is 8P/2
                rpm = 1 / pre_rev_time *60
                rpm = (12.5M/8)/(cnt*4) * 60
                rpm = 23437500/cnt
            */

            current_speed = 23437500 / (float)Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_1);
            
            // _speed_avg_buffer[_speed_avg_buffer_index++ % 3] = 23437500 / (float)Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_1);
            // current_speed = (_speed_avg_buffer[0] + _speed_avg_buffer[1] + _speed_avg_buffer[2])/3;
                
            Timer_A_clear(TIMER_A1_BASE);            
            break;
    }

    __enable_interrupt();
}


// ###############################################
// ####### api for control #######################
// ###############################################

void set_power_en(uint8_t en){ power_en = en & 0x01; }
uint8_t get_power_en(){ return power_en; }

void set_target_speed(float speed){ target_speed = speed; }
float get_target_speed(){ return target_speed; }

void set_is_reverse(uint8_t reverse){ is_reverse = reverse; }
uint8_t get_is_reverse(){ return is_reverse; }

void set_control_mode(uint8_t mode){ control_mode = mode; }
uint8_t get_control_mode(){ return control_mode; }

void set_p_value(float p){ p_value = p; }
float get_p_value(){ return p_value; }

void set_i_value(float i){ i_value = i; }
float get_i_value(){ return i_value; }

void set_pwm_compare(uint16_t compare){ pwm_compare = compare; }
uint16_t get_pwm_compare(){ return pwm_compare; }

// read only
float get_current_speed(){ return current_speed;}






























