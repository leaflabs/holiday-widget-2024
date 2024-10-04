#ifndef __ACCELERATION_H__
#define __ACCELERATION_H__

/*
    Set up the acceleration sensor (lis3dh) and configure
    according to the parameters set in the function.
*/
void acceleration_setup(void);

/*
    Run through the acceleration sensor's state machine
*/
void acceleration_run(void);

#endif /* __ACCELERATION_H__ */
