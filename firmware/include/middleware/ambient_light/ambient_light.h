#ifndef __AMBIENT_LIGHT_H__
#define __AMBIENT_LIGHT_H__

/*
    Note: The ambient light sensor also includes proximity readings.
    The results for ambient light and proximity are aquired at the
    same time, but for simplicity, this is called ambient light
    as that has the more useful data.
*/

/*
    Set up the ambient light sensor (vcnl4020) and configure it
    to the parameters specified in the function.
*/
void ambient_light_setup(void);

/*
    Run through the ambient light sensor's state machine
*/
void ambient_light_run(void);

#endif /* __AMBIENT_LIGHT_H__ */
