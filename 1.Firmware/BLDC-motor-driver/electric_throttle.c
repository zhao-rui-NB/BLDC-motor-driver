#include "electric_throttle.h"
#include "gpio.h"

uint8_t throttle_en = 0;

void adc_init(){
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN1);
    ADC12_A_init(
        ADC12_A_BASE,
        ADC12_A_SAMPLEHOLDSOURCE_SC,
        ADC12_A_CLOCKSOURCE_ADC12OSC,
        // ADC12_A_CLOCKDIVIDER_1
        ADC12_A_CLOCKDIVIDER_32
    );       
    ADC12_A_enable(ADC12_A_BASE);

    ADC12_A_setupSamplingTimer(
        ADC12_A_BASE,
        ADC12_A_CYCLEHOLD_1024_CYCLES,
        ADC12_A_CYCLEHOLD_1024_CYCLES,
        ADC12_A_MULTIPLESAMPLESENABLE
    );    

    ADC12_A_configureMemoryParam adc12_mem_para_acc = {0};
    adc12_mem_para_acc.memoryBufferControlIndex = ADC12_A_MEMORY_0;
	adc12_mem_para_acc.inputSourceSelect = ADC12_A_INPUT_A1;
	adc12_mem_para_acc.positiveRefVoltageSourceSelect = ADC12_A_VREFPOS_AVCC;
	adc12_mem_para_acc.negativeRefVoltageSourceSelect = ADC12_A_VREFNEG_AVSS;
	adc12_mem_para_acc.endOfSequence = ADC12_A_NOTENDOFSEQUENCE;
	ADC12_A_configureMemory(ADC12_A_BASE ,&adc12_mem_para_acc);

    //Enable memory buffer 0 interrupt
    ADC12_A_clearInterrupt(ADC12_A_BASE, ADC12IFG0);
    ADC12_A_enableInterrupt(ADC12_A_BASE, ADC12IE0);
    

    // start 
    ADC12_A_startConversion(
        ADC12_A_BASE,
        ADC12_A_MEMORY_0,
        ADC12_A_REPEATED_SINGLECHANNEL
    );
}

void throttle_en_btn_init(){
    // GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN1);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN1);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
}

inline uint8_t is_throttle_en_btn_press(){
    return GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN1)==0;
}


volatile uint8_t new_throttle_val = 0;
volatile uint16_t throttle_val = 0;
volatile uint16_t max_throttle_adc = 550;
volatile uint16_t min_throttle_adc = 2650;

uint16_t _val;
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_A_ISR(){
    switch (__even_in_range(ADC12IV,34)){
        case  6:          //Vector  6:  ADC12IFG0
            //Is Memory Buffer 0 = A0 > 0.5AVcc?
            _val = ADC12_A_getResults(ADC12_A_BASE, ADC12_A_MEMORY_0);
            // uart_printf("adc: %d\n", adc_val);

            throttle_val = _val;
            if(_val>max_throttle_adc){
                max_throttle_adc = _val;
            }
            else if(_val<min_throttle_adc){
                min_throttle_adc= _val;        
            }

            new_throttle_val = 1;
    }
}
