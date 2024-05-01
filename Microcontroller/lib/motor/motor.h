#include "Arduino.h"


typedef enum  {
    MOTOR_TYPE_DC, MOTOR_TYPE_BLDC
} MOTOR_TYPE;

/*
* Channel must be between 0 and 15
*/
typedef struct Motor {
    uint8_t pin0;
    uint8_t pin1; //optional, only for DC motors 
    uint8_t channel0;
    uint8_t channel1; //optional, only for DC motors
    uint32_t freq;
    MOTOR_TYPE motorType;
} Motor;

/*
* Initializes DC motor
*/
void motorInit(Motor motor);

/*
* Min speed is -127 
* Max speed is 127
*/
void setMotorSpeed(int8_t speed, Motor motor);
