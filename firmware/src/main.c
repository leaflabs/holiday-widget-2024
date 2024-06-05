#include "main.h"
#include "uart_logger.h"


/*
 * Basic system clock initalization code
 *
 */
void system_clock_init(void) {
	RCC_OscInitTypeDef rcc_osc_init = {0};
	RCC_ClkInitTypeDef rcc_clk_init = {0};

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
	  
	/* Enable MSI Oscillator */
	rcc_osc_init.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
	rcc_osc_init.MSIState            = RCC_MSI_ON;
	rcc_osc_init.MSICalibrationValue = 0;
	rcc_osc_init.MSIClockRange = RCC_MSIRANGE_5;
	rcc_osc_init.PLL.PLLState = RCC_PLL_NONE;


	if (HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK) {
		// error message here
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
	* clocks' dividers.
	*/
	rcc_clk_init.ClockType      = (RCC_CLOCKTYPE_SYSCLK
				 | RCC_CLOCKTYPE_HCLK
				 | RCC_CLOCKTYPE_PCLK1
				 | RCC_CLOCKTYPE_PCLK2);

	rcc_clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;  
	rcc_clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV2;          
	rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV1;            
	rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;            
	if (HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_0) != HAL_OK) {
		// Error message here
	}

}


/*
 * Custom delay made to test clock speeds
 *
 */
void delay(volatile uint32_t time) {
	for(volatile uint32_t i = 0; i < time; i++) {
		// Just do some wasteful calculations
		for(volatile uint32_t j = 0; j < 50;j++) {
			i++;
			i--;
		}
		
	}
}



/*
 * Initalize everything for the GPIO pins
 *
 */
void GPIO_Init() {

	// Enable the GPIOC clock
	__HAL_RCC_GPIOC_CLK_ENABLE();


	GPIO_InitTypeDef gpio = {0};

	// Set up LED Pins (8-11);
	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_PULLDOWN;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(GPIOC, &gpio);


}



int main(void) {

	// Init the HAL
	HAL_Init();

	// Init the system clock
	system_clock_init();
	
	// Set up the GPIO pins for the button and led
	GPIO_Init();


	// Set up the uart for communication with the computer
	uart_logger_init();

	// Turn 8 and 11 on initially so we can have the leds flash alternating
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_11, 1);

 	while(1) {

		// Flip all leds
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
		
		HAL_Delay(200);

		
	}
}
