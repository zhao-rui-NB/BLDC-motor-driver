#ifndef __BLDC_Driver__
#define __BLDC_Driver__

#include "driverlib.h"
#include "BLDC_Driver.h"
#include "uart.h"

// timer on mos driver pin
// P1.2 TA0.1 CH PWM
// P1.3 TA0.2 CL
// P1.4 TA0.3 BH PWM
// P1.5 TA0.4 BL
// P2.4 TA2.1 AH PWM
// P2.5 TA2.2 AL

// mos driver pin
// 2.4 BH AH
// 2.5 BL AL
// 1.4 GH BH
// 1.5 GL BL 
// 1.2 YH CH
// 1.3 YL CL

// #### mos driver ### 

#define write_en_AH(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN4) : GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4))
#define write_en_AL(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5) : GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5))

#define write_en_BH(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN4) : GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4))
#define write_en_BL(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5))

#define write_en_CH(x) (x ? GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN2) : GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2))
#define write_en_CL(x) (x ? GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3) : GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3))

#define write_en_AhAlBhBlChCl(x, y, w, a, b, c) \
    do{ \
        write_en_AH(x);write_en_AL(y);write_en_BH(w);write_en_BL(a);write_en_CH(b);write_en_CL(c); \
    }while(0)


#define PWM_TIMER_PERIOD 500
#define PWM_MAX_COMPARE_VALUE 450

#define DEFAULT_COMPARE_VALUE 100

#define HALL_SENSOR_ENCODE_OFFSET 0
#define HAll_SENSOR_ENCODE_REVERSE 1 // ok


#define MODE_MANUAL_PWM 0
#define MODE_PI_SPEED 1

// PI control PARAMETER
#define MIN_PI_CONTROLLER_WRITE_DUTY 50
#define INTEGRAL_MAX 50
#define INTEGRAL_MIN -50

// ### control parameter ###

// ## setting value
extern volatile  uint8_t power_en;
extern uint8_t is_reverse;
extern uint8_t control_mode;

// pwm control
extern uint16_t pwm_compare;

// pid speed control
extern float target_speed;
extern float p_value;
extern float i_value;

// ## measure value
extern float current_speed;

extern volatile uint32_t count_hall;
extern volatile uint32_t count_pwm_interrupt;


// ### mos driver ###
void MOSGateDriver_init();
void MOSGateDriver_write_duty(uint16_t);
inline void MOSGateDriver_write_disable();
inline void MOSGateDriver_write_step(uint8_t);
void MOSGateDriver_by_hall_sensor(uint8_t hall_encode);

// ### hall sensor ###
void hall_sensor_init();
uint8_t read_hall_sensor();

// ### controller
void BLDC_controller_init();
void motor_speed_timer_init();


// #####################
// ### control api #####
// #####################

void set_power_en(uint8_t power_en);
uint8_t get_power_en();

void set_target_speed(float speed);
float get_target_speed();

void set_is_reverse(uint8_t is_reverse);
uint8_t get_is_reverse();

void set_control_mode(uint8_t control_mode);
uint8_t get_control_mode();

void set_p_value(float p_value);
float get_p_value();

void set_i_value(float i_value);
float get_i_value();

void set_pwm_compare(uint16_t compare);
uint16_t get_pwm_compare();

// read only
float get_current_speed();







#endif
