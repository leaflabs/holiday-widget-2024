#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

/*
    Set up the temperature sensor (tmp102) and configure it
    to the parameters specified in the function
*/
void temperature_setup(void);

/*
    Run through the temperature sensor's state machine
*/
void temperature_run(void);

#endif /* __TEMPERATURE_H__ */
