#include "imp23absu_driver.h"

#include "uart_logger.h"

/* Mask for branchless circular buffer indexing */
#define AUDIO_BUFF_SIZE_MASK (AUDIO_BUFF_SIZE - 1U)

/* Maximum allowable read size to avoid race condition */
#define MAX_READ_SIZE (AUDIO_BUFF_SIZE - 2U)

/* Evaluates to the scalar equivalent of the given prescaler value */
#define ADC_SYNC_PRESCALER_TO_VAL(__presc__)      \
    (__presc__ == ADC_CLOCK_SYNC_PCLK_DIV1        \
         ? 1                                      \
         : (__presc__ == ADC_CLOCK_SYNC_PCLK_DIV2 \
                ? 2                               \
                : (__presc__ == ADC_CLOCK_SYNC_PCLK_DIV4 ? 4 : -1)))

/* Evaluates to the number of clock cycles needed to perform a conversion at the
 * given resolution  */
#define ADC_RESOLUTION_TO_CLOCK_CYCLES(__res__)  \
    (__res__ == ADC_RESOLUTION_6B                \
         ? 6.5f                                  \
         : (__res__ == ADC_RESOLUTION_8B         \
                ? 8.5f                           \
                : (__res__ == ADC_RESOLUTION_10B \
                       ? 10.5f                   \
                       : (__res__ == ADC_RESOLUTION_12B ? 12.5f : -1))))

/* Evaluates to the number of clock cycles represented by the given sample time
 */
#define ADC_SAMPLE_TIME_TO_CLOCK_CYCLES(__sample_time__)                                 \
    (__sample_time__ == ADC_SAMPLETIME_1CYCLE_5                                          \
         ? 1.5f                                                                          \
         : (__sample_time__ == ADC_SAMPLETIME_3CYCLES_5                                  \
                ? 3.5f                                                                   \
                : (__sample_time__ == ADC_SAMPLETIME_7CYCLES_5                           \
                       ? 7.5f                                                            \
                       : (__sample_time__ == ADC_SAMPLETIME_12CYCLES_5                   \
                              ? 12.5f                                                    \
                              : (__sample_time__ == ADC_SAMPLETIME_19CYCLES_5            \
                                     ? 19.5f                                             \
                                     : (__sample_time__ ==                               \
                                                ADC_SAMPLETIME_39CYCLES_5                \
                                            ? 39.5f                                      \
                                            : (__sample_time__ ==                        \
                                                       ADC_SAMPLETIME_79CYCLES_5         \
                                                   ? 79.5f                               \
                                                   : (__sample_time__ ==                 \
                                                              ADC_SAMPLETIME_160CYCLES_5 \
                                                          ? 160.5f                       \
                                                          : -1))))))))

/* Evaluates to the scalar multiplicative equivalent of the given oversampling
 * ratio */
#define ADC_OVERSAMPLING_RATIO_TO_VAL(__oversample__)                                    \
    (__oversample__ == ADC_OVERSAMPLING_RATIO_2                                          \
         ? 2                                                                             \
         : (__oversample__ == ADC_OVERSAMPLING_RATIO_4                                   \
                ? 4                                                                      \
                : (__oversample__ == ADC_OVERSAMPLING_RATIO_8                            \
                       ? 8                                                               \
                       : (__oversample__ == ADC_OVERSAMPLING_RATIO_16                    \
                              ? 16                                                       \
                              : (__oversample__ == ADC_OVERSAMPLING_RATIO_32             \
                                     ? 32                                                \
                                     : (__oversample__ ==                                \
                                                ADC_OVERSAMPLING_RATIO_64                \
                                            ? 64                                         \
                                            : (__oversample__ ==                         \
                                                       ADC_OVERSAMPLING_RATIO_128        \
                                                   ? 128                                 \
                                                   : (__oversample__ ==                  \
                                                              ADC_OVERSAMPLING_RATIO_256 \
                                                          ? 256                          \
                                                          : -1))))))))
/* Private IMP23ABSU driver instance */
static struct imp23absu_driver imp23absu_driver;

uint32_t imp23absu_get_sample_frequency() {
    return imp23absu_driver.sample_frequency;
}

/* Returns next value from the given ring buffer */
static uint16_t pop(struct audio_ring_buffer *buffer) {
    return buffer->data[(++buffer->read_idx) & AUDIO_BUFF_SIZE_MASK];
}

/* Initializes GPIO */
static void gpio_init() {
    GPIO_InitTypeDef gpio;

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* ADC1 Channel 0 GPIO (PA0 - audio_in) and COMP2 non-inverting
     * Analog GPIO (PA3 - audio_in) pin configuration */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* Audio Amplifier GPIO pin configuration (PB9 - audio_amp_en) */
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
}

/* Initializes DMA */
static int dma_init() {
    int ret;

    /* Enable DMA1 clock */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /****************************/
    /* Configure DMA parameters */
    /****************************/

    /* Set DMA handle instance to DMA1 Channel 1 */
    imp23absu_driver.dma_handle.Instance = DMA1_Channel1;

    /* Set DMA transfer direction to Peripheral to Memory (ADC->Buffer)
     */
    imp23absu_driver.dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;

    /* Disable incrementation of peripheral address */
    imp23absu_driver.dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;

    /* Enable incrementation of memory address */
    imp23absu_driver.dma_handle.Init.MemInc = DMA_MINC_ENABLE;

    /* Set data alignment to half-word (16-bit) */
    imp23absu_driver.dma_handle.Init.PeriphDataAlignment =
        DMA_PDATAALIGN_HALFWORD;
    imp23absu_driver.dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;

    /* Enable circular transfer */
    imp23absu_driver.dma_handle.Init.Mode = DMA_CIRCULAR;

    /* Set priority to HIGH */
    imp23absu_driver.dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;

    /* Use request line 0 (ADC) */
    imp23absu_driver.dma_handle.Init.Request = DMA_REQUEST_0;

    /**************************/
    /* Perform initialization */
    /**************************/

    /* Restore DMA peripheral to default state */
    if ((ret = HAL_DMA_DeInit(&imp23absu_driver.dma_handle)) != HAL_OK) {
        return ret;
    }

    /* Initialize the DMA for new transfer */
    if ((ret = HAL_DMA_Init(&imp23absu_driver.dma_handle)) != HAL_OK) {
        return ret;
    }

    /* Associate the DMA handle */
    __HAL_LINKDMA(&imp23absu_driver.adc_handle, DMA_Handle,
                  imp23absu_driver.dma_handle);

    return 0;
}

/* Initializes ADC */
static int adc_init() {
    int ret;

    /* ADC channel configuration structure declaration */
    static ADC_ChannelConfTypeDef channel_config;

    /* Enable ADC1 Periph clock */
    __HAL_RCC_ADC1_CLK_ENABLE();

    /****************************/
    /* Configure ADC parameters */
    /****************************/

    /**
     * SYSCLK Source set to MSI so SYSCLK is 2.097 MHz
     *
     * HCLK Divider is DIV1 so HCLK is 2.097 MHz
     *
     * PCLK2 Divider is DIV1 so PCLK2 is 2.097 MHz
     */
    float pclk2_frequency = HAL_RCC_GetPCLK2Freq();
    float adcclk_frequency;
    float cycles_per_conversion = 0;

    /* Set ADC handle instance to ADC1 */
    imp23absu_driver.adc_handle.Instance = ADC1;

    /*
     * ADC Prescaler is DIV1 so ADCCLK is 2.097 MHz
     */
    imp23absu_driver.adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;

    adcclk_frequency =
        pclk2_frequency / ADC_SYNC_PRESCALER_TO_VAL(
                              imp23absu_driver.adc_handle.Init.ClockPrescaler);

    /* ADCCLK = 2.097 MHz is less than 3.5 MHz so LowPowerFrequencyMode
     * must be enabled */
    if (adcclk_frequency < 3500000) {
        imp23absu_driver.adc_handle.Init.LowPowerFrequencyMode = ENABLE;
    } else {
        imp23absu_driver.adc_handle.Init.LowPowerFrequencyMode = DISABLE;
    }

    /* Disable LowPowerAutoPowerOff and LowPowerAutoWait*/
    imp23absu_driver.adc_handle.Init.LowPowerAutoPowerOff = DISABLE;
    imp23absu_driver.adc_handle.Init.LowPowerAutoWait = DISABLE;

    /*
     * 12-bit ADC Conversion Time is 12.5 cycles
     */
    imp23absu_driver.adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
    cycles_per_conversion += ADC_RESOLUTION_TO_CLOCK_CYCLES(
        imp23absu_driver.adc_handle.Init.Resolution);

    /* Set data alignment to right-aligned */
    imp23absu_driver.adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;

    /*
     * ADC Sample Time is 7.5 ADCCLK cycles
     */
    imp23absu_driver.adc_handle.Init.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;

    cycles_per_conversion += ADC_SAMPLE_TIME_TO_CLOCK_CYCLES(
        imp23absu_driver.adc_handle.Init.SamplingTime);

    /**
     * Total Conversion Time
     *   	= Sample Time + Conversion Time
     *  	= 7.5 cycles + 12.5 cycles
     *  	= 20 cycles
     */

    /* Enable 4x oversampling */
    imp23absu_driver.adc_handle.Init.OversamplingMode = ENABLE;
    imp23absu_driver.adc_handle.Init.Oversample.Ratio =
        ADC_OVERSAMPLING_RATIO_4;

    imp23absu_driver.adc_handle.Init.Oversample.RightBitShift =
        ADC_RIGHTBITSHIFT_2;
    imp23absu_driver.adc_handle.Init.Oversample.TriggeredMode =
        ADC_TRIGGEREDMODE_SINGLE_TRIGGER;

    /* Oversampling Ratio is 4 so samples per conversion is quadrupled to 80
     * cycles */
    cycles_per_conversion *= ADC_OVERSAMPLING_RATIO_TO_VAL(
        imp23absu_driver.adc_handle.Init.Oversample.Ratio);

    /*
     * Sample Frequency = ADCCLK / cycles_per_conversion = 2.097 MHz / 80 cycles
     * = 26.2125 KHz
     */
    imp23absu_driver.sample_frequency =
        (uint32_t)(adcclk_frequency / cycles_per_conversion);

    /* Enable continuous DMA requests */
    imp23absu_driver.adc_handle.Init.DMAContinuousRequests = ENABLE;

    /* Enable continuous conversions and disable discontinuous
     * conversions */
    imp23absu_driver.adc_handle.Init.ContinuousConvMode = ENABLE;
    imp23absu_driver.adc_handle.Init.DiscontinuousConvMode = DISABLE;

    /* Set end of conversion flag after single conversion */
    imp23absu_driver.adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    /* Scanning is not performed - set field to default value */
    imp23absu_driver.adc_handle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;

    /* Overwrite overrun values */
    imp23absu_driver.adc_handle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

    /* Disable external trigger */
    imp23absu_driver.adc_handle.Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_NONE;

    /******************************************/
    /* Perform initialization and calibration */
    /******************************************/

    /* Reset ADC peripheral to default state */
    if ((ret = HAL_ADC_DeInit(&imp23absu_driver.adc_handle)) != HAL_OK) {
        return ret;
    }

    /* Initialize ADC peripheral according to the passed parameters */
    if ((ret = HAL_ADC_Init(&imp23absu_driver.adc_handle)) != HAL_OK) {
        return ret;
    }

    /* Start ADC calibration */
    if ((ret = HAL_ADCEx_Calibration_Start(&imp23absu_driver.adc_handle,
                                           ADC_SINGLE_ENDED)) != HAL_OK) {
        return ret;
    }

    /* Select Channel 0 (Analog Input PA0) to be converted */
    channel_config.Channel = ADC_CHANNEL_0;
    if ((ret = HAL_ADC_ConfigChannel(&imp23absu_driver.adc_handle,
                                     &channel_config)) != HAL_OK) {
        return ret;
    }

    return 0;
}

/* Initializes IMP23ABSU driver */
int imp23absu_driver_init() {
    int ret;

    /* If driver is enabled, disable before continuing */
    if (imp23absu_driver.enabled && (ret = imp23absu_driver_disable())) {
        return ret;
    }

    /* (Re)Set the initialized flag to false */
    imp23absu_driver.initialized = false;

    /* Initialize GPIO */
    gpio_init();

    /* Initialize ADC */
    if ((ret = adc_init())) {
        return ret;
    }

    /* Initialize DMA */
    if ((ret = dma_init())) {
        return ret;
    }

    /* Set initialized and enabled flags to reflect post-init state */
    imp23absu_driver.initialized = true;
    imp23absu_driver.enabled = false;

    return 0;
}

/*
    Start continuous transfer of ADC conversions into audio buffer.
    Enable audio event detection.
*/
int imp23absu_driver_enable() {
    int ret;

    if (!imp23absu_driver.initialized) {
        /* Not initialized error */
        return -1;
    }

    if (imp23absu_driver.enabled) {
        /* Already enabled error */
        return -1;
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

    /* Enable ADC and DMA stream */
    if ((ret = HAL_ADC_Start_DMA(&imp23absu_driver.adc_handle,
                                 (uint32_t *)imp23absu_driver.audio_buffer.data,
                                 AUDIO_BUFF_SIZE))) {
        return ret;
    }

    /* Set enabled flag */
    imp23absu_driver.enabled = true;

    return 0;
}

/*
    Stop continuous transfer of ADC conversions into audio buffer.
    Disable audio event detection.
*/
int imp23absu_driver_disable() {
    int ret;

    if (!imp23absu_driver.initialized) {
        /* Not initialized error */
        return -1;
    }

    if (!imp23absu_driver.enabled) {
        /* Already disabled error */
        return -1;
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);

    /* Disable ADC and DMA stream */
    if ((ret = HAL_ADC_Stop_DMA(&imp23absu_driver.adc_handle))) {
        return ret;
    }

    /* Clear enabled flag */
    imp23absu_driver.enabled = false;

    return 0;
}

/* Flush the contents of the audio buffer with 0 */
void imp23absu_driver_flush_buffer() {
    memset(imp23absu_driver.audio_buffer.data, 0, AUDIO_BUFF_SIZE);
}

/**
 * Read the most recent 'len' values from the audio buffer into 'buf',
 * returns the number of values read. 'len' cannot exceed MAX_READ_SIZE.
 */
ssize_t imp23absu_driver_read(uint16_t *buf, size_t len) {
    if (!imp23absu_driver.enabled) {
        /* Not enabled error */
        return -1;
    }

    /* Clamp read size to MAX_READ_SIZE */
    if (len > MAX_READ_SIZE) {
        len = MAX_READ_SIZE;
    }

    /* Set read index to last write location */
    imp23absu_driver.audio_buffer.read_idx =
        AUDIO_BUFF_SIZE - imp23absu_driver.dma_handle.Instance->CNDTR - len - 1;

    /* Pop the most recent 'len' values into the buffer */
    for (int i = 0; i < len; i++) {
        buf[i] = pop(&imp23absu_driver.audio_buffer);
    }

    /* Return the number of values read */
    return len;
}
