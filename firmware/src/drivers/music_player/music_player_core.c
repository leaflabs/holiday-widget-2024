#include "music_player_core.h"

#include <errno.h>

#define MUSIC_PLAYER_MAX_VOLUME 10U

#define MUSIC_PLAYER_VOLUME 3U

#if MUSIC_PLAYER_VOLUME > MUSIC_PLAYER_MAX_VOLUME
#error "MUSIC_PLAYER_VOLUME must not exceed MUSIC_PLAYER_MAX_VOLUME"
#endif

/* This function is called if an error occurs on the duration transfer DMA
 * Channel */
void duration_transfer_error_callback(DMA_HandleTypeDef *hdma);

/* This function is called if an error occurs on the note transfer DMA Channel
 */
void note_transfer_error_callback(DMA_HandleTypeDef *hdma);

/* This function is called whenever a song ends */
void song_complete_callback(DMA_HandleTypeDef *hdma);

/* Initializes GPIO */
static void gpio_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    /* Declare GPIO initialization structure */
    GPIO_InitTypeDef gpio = {0};

    /***************************/
    /* Configure Audio Out Pin */
    /***************************/

    /* GPIOx Peripheral Clock Enable */
    GPIOx_CLK_ENABLE(cfg->audio_out_pin.port);

    /* Configure Audio Out Pin as high-speed analog output */
    gpio.Pin = cfg->audio_out_pin.pin;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    /* Initialize GPIO port with configured structure */
    HAL_GPIO_Init(cfg->audio_out_pin.port, &gpio);
}

/* Initializes DAC, return 0 on success, -1 on failure */
static int dac_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    /* DAC Channel Configuration Structure Declaration */
    DAC_ChannelConfTypeDef dac_channel_config = {0};

    /* DAC Peripheral Clock Enable */
    __HAL_RCC_DAC_CLK_ENABLE();

    /* Set DAC instance to DAC1 */
    cfg->dac_handle.Instance = DAC1;

    /* Initialize DAC1 */
    if (HAL_DAC_Init(&cfg->dac_handle) != HAL_OK) {
        /* DAC Initiliazation Error */
        return -1;
    }

    /* DAC1 Channel 1 Configuration */
    dac_channel_config.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    dac_channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    /* Configure DAC1 Channel 1 */
    if (HAL_DAC_ConfigChannel(&cfg->dac_handle, &dac_channel_config,
                              DAC_CHANNEL_1) != HAL_OK) {
        /* DAC Channel configuration Error */
        return -1;
    }

    return 0;
}

/* Initializes DMA, return 0 on success, -1 on failure */
static int dma_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    /* DMA1 Peripheral Clock Enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /***Duration Transfer***/

    /* Set DMA parameters */
    cfg->hdma_tim2_durations.Init.Request = DMA_REQUEST_8;
    cfg->hdma_tim2_durations.Instance = DMA1_Channel1;
    cfg->hdma_tim2_durations.Init.Direction = DMA_MEMORY_TO_PERIPH;
    cfg->hdma_tim2_durations.Init.PeriphInc = DMA_PINC_DISABLE;
    cfg->hdma_tim2_durations.Init.MemInc = DMA_MINC_ENABLE;
    cfg->hdma_tim2_durations.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    cfg->hdma_tim2_durations.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    cfg->hdma_tim2_durations.Init.Mode = DMA_NORMAL;
    cfg->hdma_tim2_durations.Init.Priority = DMA_PRIORITY_HIGH;

    /***Note Transfer***/

    /* Set DMA parameters */
    cfg->hdma_tim2_notes.Init.Request = DMA_REQUEST_8;
    cfg->hdma_tim2_notes.Instance = DMA1_Channel3;
    cfg->hdma_tim2_notes.Init.Direction = DMA_MEMORY_TO_PERIPH;
    cfg->hdma_tim2_notes.Init.PeriphInc = DMA_PINC_DISABLE;
    cfg->hdma_tim2_notes.Init.MemInc = DMA_MINC_ENABLE;
    cfg->hdma_tim2_notes.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    cfg->hdma_tim2_notes.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    cfg->hdma_tim2_notes.Init.Mode = DMA_NORMAL;
    cfg->hdma_tim2_notes.Init.Priority = DMA_PRIORITY_HIGH;

    /***Waveform Transfer***/

    /* Set DMA parameters */
    cfg->hdma_dac.Instance = DMA1_Channel2;

    cfg->hdma_dac.Init.Request = DMA_REQUEST_9;
    cfg->hdma_dac.Init.Direction = DMA_MEMORY_TO_PERIPH;
    cfg->hdma_dac.Init.PeriphInc = DMA_PINC_DISABLE;
    cfg->hdma_dac.Init.MemInc = DMA_MINC_ENABLE;
    cfg->hdma_dac.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    cfg->hdma_dac.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    cfg->hdma_dac.Init.Mode = DMA_CIRCULAR;
    cfg->hdma_dac.Init.Priority = DMA_PRIORITY_HIGH;

    /* Link hdma_tim2_notes to hdma[TIM_DMA_ID_CC2] (channel2) */
    __HAL_LINKDMA(&cfg->tim2_handle, hdma[TIM_DMA_ID_CC2],
                  cfg->hdma_tim2_notes);

    /* Link hdma_tim2_durations to hdma[TIM_DMA_ID_CC3] (channel3) */
    __HAL_LINKDMA(&cfg->tim2_handle, hdma[TIM_DMA_ID_CC3],
                  cfg->hdma_tim2_durations);

    /* Link hdma_dac handle to the the DAC handle */
    __HAL_LINKDMA(&cfg->dac_handle, DMA_Handle1, cfg->hdma_dac);

    /* Initialize DMA for TIM2 CC2 request */
    if (HAL_DMA_Init(cfg->tim2_handle.hdma[TIM_DMA_ID_CC2]) != HAL_OK) {
        /* DMA Initialization Error */
        return -1;
    }

    /* Initialize DMA for TIM2 CC3 request */
    if (HAL_DMA_Init(cfg->tim2_handle.hdma[TIM_DMA_ID_CC3]) != HAL_OK) {
        /* DMA Initialization Error */
        return -1;
    }

    /* Initialize DMA for DAC request */
    if (HAL_DMA_Init(&cfg->hdma_dac) != HAL_OK) {
        /* DMA Initialization Error */
        return -1;
    }

    /* NVIC configuration for DMA transfer complete interrupt (Song Complete) */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    /* NVIC configuration for DMA transfer complete and error interrupt
     * (Duration Complete, DMA Errors)*/
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    return 0;
}

/* Initialize TIM2, return 0 on success, -1 on failure */
static int tim2_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    /* TIM2 Peripheral Clock Enable */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Declare configuration structures */
    TIM_ClockConfigTypeDef clock_source_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};
    TIM_SlaveConfigTypeDef slave_config = {0};
    TIM_OC_InitTypeDef tim_config = {0};

    /* Set configuration parameters */
    cfg->tim2_handle.Instance = TIM2;
    cfg->tim2_handle.Init.Prescaler = 0;
    cfg->tim2_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    cfg->tim2_handle.Init.Period = 0x1000;
    cfg->tim2_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    cfg->tim2_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    /* Configure clock source */
    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&cfg->tim2_handle, &clock_source_config)) {
        /* TIM2 Clock Source Configuration Error */
        return -1;
    }

    /* Configure Master parameters */
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&cfg->tim2_handle,
                                              &master_config)) {
        /* TIM2 Master Configuration Error */
        return -1;
    }

    /* Configure Slave parameters */
    slave_config.SlaveMode = TIM_SLAVEMODE_GATED;
    slave_config.InputTrigger = TIM_TS_ITR2;
    if (HAL_TIM_SlaveConfigSynchro(&cfg->tim2_handle, &slave_config) !=
        HAL_OK) {
        /* TIM2 Slave Configuration Error */
        return -1;
    }

    /* Initialize TIM2 registers */
    uint32_t tmpcr1;
    tmpcr1 = TIM2->CR1;

    /* Select the Counter Mode */
    tmpcr1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);
    tmpcr1 |= cfg->tim2_handle.Init.CounterMode;

    /* Set the auto-reload preload */
    MODIFY_REG(tmpcr1, TIM_CR1_ARPE, cfg->tim2_handle.Init.AutoReloadPreload);

    TIM2->CR1 = tmpcr1;

    /* Set the Autoreload value */
    TIM2->ARR = (uint32_t)cfg->tim2_handle.Init.Period;

    /* Set the Prescaler value */
    TIM2->PSC = cfg->tim2_handle.Init.Prescaler;

    /* Generate an update event to reload the period value immediately */
    TIM2->EGR = TIM_EGR_UG;

    /* Initialize the TIM channels state */
    TIM_CHANNEL_STATE_SET_ALL(&cfg->tim2_handle, HAL_TIM_CHANNEL_STATE_READY);

    /* Configure TIM2 Channel 3 */
    tim_config.OCMode = TIM_OCMODE_PWM1;
    tim_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    tim_config.Pulse = 0x1;
    if (HAL_TIM_PWM_ConfigChannel(&cfg->tim2_handle, &tim_config,
                                  TIM_CHANNEL_3) != HAL_OK) {
        /* TIM2 Channel 3 Configuration Error */
        return -1;
    }

    /* Configure TIM2 Channel 2 */
    tim_config.OCMode = TIM_OCMODE_PWM1;
    tim_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    tim_config.Pulse = 0x1;
    if (HAL_TIM_PWM_ConfigChannel(&cfg->tim2_handle, &tim_config,
                                  TIM_CHANNEL_2) != HAL_OK) {
        /* TIM2 Channel 3 Configuration Error */
        return -1;
    }

    /* Reset the CCxE Bits */
    TIM2->CCER &= ~((TIM_CCER_CC1E << (TIM_CHANNEL_2 & 0x1FU)) |
                    TIM_CCER_CC1E << (TIM_CHANNEL_3 & 0x1FU));

    /* Set CC2E and CC3E (Enable Capture/Compare Channels 2 & 3) Bits */
    TIM2->CCER |=
        (uint32_t)(TIM_CCx_ENABLE
                   << (TIM_CHANNEL_2 & 0x1FU)); /* 0x1FU = 31 bits max shift */
    TIM2->CCER |= (uint32_t)(TIM_CCx_ENABLE << (TIM_CHANNEL_3 & 0x1FU));

    /* Set Transfer Complete Callback to song_complete_callback function */
    cfg->tim2_handle.hdma[TIM_DMA_ID_CC3]->XferCpltCallback =
        song_complete_callback;

    /* Set Respective Error Callbacks */
    cfg->tim2_handle.hdma[TIM_DMA_ID_CC3]->XferErrorCallback =
        duration_transfer_error_callback;
    cfg->tim2_handle.hdma[TIM_DMA_ID_CC2]->XferErrorCallback =
        note_transfer_error_callback;

    return 0;
}

/* Initializes TIM3, return 0 on success, -1 on failure */
static int tim3_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;
    /* TIM3 Peripheral Clock Enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    TIM_ClockConfigTypeDef clock_source_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    cfg->tim3_handle.Instance = TIM3;
    cfg->tim3_handle.Init.Prescaler = 0;
    cfg->tim3_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    cfg->tim3_handle.Init.Period = 0xFFFF;
    cfg->tim3_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    cfg->tim3_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&cfg->tim3_handle) != HAL_OK) {
        /* TIM3 Initialization Error */
        return -1;
    }

    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&cfg->tim3_handle, &clock_source_config)) {
        /* TIM3 Clock Source Configuration Error */
        return -1;
    }

    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&cfg->tim3_handle,
                                              &master_config)) {
        /* TIM3 Master Configuration Error */
        return -1;
    }

    return 0;
}

/* Initializes TIM6, return 0 on success, -1 on failure */
static int tim6_init(struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    /* TIM6 Peripheral Clock Enable */
    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM_MasterConfigTypeDef master_config;

    /* Time base configuration */
    cfg->tim6_handle.Instance = TIM6;
    cfg->tim6_handle.Init.Period = D4_SHARP;
    cfg->tim6_handle.Init.Prescaler = 0;
    cfg->tim6_handle.Init.ClockDivision = 0;
    cfg->tim6_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&cfg->tim6_handle) != HAL_OK) {
        /* TIM6 Initialization Error */
        return -1;
    }

    /* TIM6 TRGO selection */
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&cfg->tim6_handle,
                                              &master_config) != HAL_OK) {
        /* TIM6 Master Configuration Error */
        return -1;
    }

    return 0;
}

/* Initializes Music Player instance, return 0 on success, -1 on failure */
enum music_player_error music_player_core_init(
    struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    for (int i = 0; i < SINE_WAVE_SAMPLES; i++) {
        sine_wave_volume_adj[i] =
            (sine_wave[i] * MUSIC_PLAYER_VOLUME) / MUSIC_PLAYER_MAX_VOLUME;
    }

    if (pam8302a_driver_init(cfg->pam8302a_driver)) {
        /* PAM8302A Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    /* Initialize GPIO */
    gpio_init(music_player);

    if (dma_init(music_player)) {
        /* DMA Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    if (tim6_init(music_player)) {
        /* TIM6 Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    if (tim3_init(music_player)) {
        /* TIM3 Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    if (tim2_init(music_player)) {
        /* TIM2 Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    if (dac_init(music_player)) {
        /* DAC Initialization Error */
        return MUSIC_PLAYER_INITIALIZATION_ERROR;
    }

    return MUSIC_PLAYER_NO_ERROR;
}

/* Plays the given song, returns 0 on success, -1 on failure */
enum music_player_error music_player_core_play_song(
    struct music_player *music_player, enum Song song) {
    struct music_player_config *cfg = &music_player->config;

    /* Enable waveform transfer frequency generation timer (TIM6) */
    if (HAL_TIM_Base_Start(&cfg->tim6_handle) != HAL_OK) {
        /* TIM6 Enable Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable DMA durations transfer */
    if (HAL_DMA_Start_IT(&cfg->hdma_tim2_durations, (uint32_t)DURATIONS[song],
                         (uint32_t)&cfg->tim3_handle.Instance->ARR,
                         SIZES[song]) != HAL_OK) {
        /* "Durations" DMA Enable Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable DMA notes transfer */
    if (HAL_DMA_Start_IT(&cfg->hdma_tim2_notes, (uint32_t)NOTES[song],
                         (uint32_t)&cfg->tim6_handle.Instance->ARR,
                         SIZES[song]) != HAL_OK) {
        /* "Notes" DMA Enable Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable DMA request generation timer (Slave - TIM2) */
    if (HAL_TIM_Base_Start(&cfg->tim2_handle)) {
        /* TIM2 Enable Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable duration generation timer (Master - TIM3) */
    if (HAL_TIM_Base_Start(&cfg->tim3_handle)) {
        /* TIM3 Enable Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable the TIM Capture/Compare 3 DMA request */
    __HAL_TIM_ENABLE_DMA(&cfg->tim2_handle, TIM_DMA_CC3);

    /* Enable the TIM Capture/Compare 2 DMA request */
    __HAL_TIM_ENABLE_DMA(&cfg->tim2_handle, TIM_DMA_CC2);

    /* Enable DAC and DMA waveform transfer */
    if (HAL_DAC_Start_DMA(&cfg->dac_handle, DAC_CHANNEL_1,
                          (uint32_t *)sine_wave_volume_adj, SINE_WAVE_SAMPLES,
                          DAC_ALIGN_12B_R) != HAL_OK) {
        /* Start DMA Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    /* Enable PAM8302A amplifier output */
    if (pam8302a_driver_enable(cfg->pam8302a_driver)) {
        /* Enable PAM8302A Error */
        return MUSIC_PLAYER_RUN_ERROR;
    }

    return MUSIC_PLAYER_NO_ERROR;
}

/* Aborts the song playing process, returns 0 on success, -1 on failure */
enum music_player_error music_player_core_abort_song(
    struct music_player *music_player) {
    struct music_player_config *cfg = &music_player->config;

    int return_value = 0;
    return_value |= HAL_TIM_Base_Stop(&cfg->tim6_handle);
    return_value |= HAL_TIM_Base_Stop(&cfg->tim3_handle);
    return_value |= HAL_TIM_Base_Stop(&cfg->tim2_handle);
    return_value |= HAL_DAC_Stop(&cfg->dac_handle, DAC_CHANNEL_1);
    return_value |= pam8302a_driver_disable(cfg->pam8302a_driver);

    HAL_DMA_Abort(&cfg->hdma_tim2_durations);
    HAL_DMA_Abort(&cfg->hdma_tim2_notes);

    if (return_value) {
        return MUSIC_PLAYER_STOP_ERROR;
    }

    return MUSIC_PLAYER_NO_ERROR;
}

/* This function is called if an error occurs on the duration transfer DMA
 * Channel */
void duration_transfer_error_callback(DMA_HandleTypeDef *hdma) {
    struct music_player_config *cfg =
        CONTAINER_OF(hdma, struct music_player_config, hdma_tim2_durations);
    struct music_player_context *context =
        &CONTAINER_OF(cfg, struct music_player, config)->context;

    context->error = MUSIC_PLAYER_DURATIONS_DMA_ERROR;
}

/* This function is called if an error occurs on the note transfer DMA Channel
 */
void note_transfer_error_callback(DMA_HandleTypeDef *hdma) {
    struct music_player_config *cfg =
        CONTAINER_OF(hdma, struct music_player_config, hdma_tim2_notes);
    struct music_player_context *context =
        &CONTAINER_OF(cfg, struct music_player, config)->context;

    context->error = MUSIC_PLAYER_NOTES_DMA_ERROR;
}

/* This function is called whenever a song ends */
void song_complete_callback(DMA_HandleTypeDef *hdma) {
    struct music_player_config *cfg =
        CONTAINER_OF(hdma, struct music_player_config, hdma_tim2_durations);
    struct music_player_context *context =
        &CONTAINER_OF(cfg, struct music_player, config)->context;

    context->current_song = NO_SONG;
}

/**
 * @brief  Error DAC callback for Channel1.
 * @param  hdac pointer to a DAC_HandleTypeDef structure that contains
 *         the configuration information for the specified DAC.
 * @retval None
 */
void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *hdac) {
    struct music_player_config *cfg =
        CONTAINER_OF(hdac, struct music_player_config, hdma_dac);
    struct music_player_context *context =
        &CONTAINER_OF(cfg, struct music_player, config)->context;

    context->error = MUSIC_PLAYER_DAC_DMA_ERROR;
}
