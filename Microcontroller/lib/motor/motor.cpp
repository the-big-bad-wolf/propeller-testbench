#include "motor.h"
// This is optmized for simplicty not performance.
// If (more) performance is needed motor object should not be passed around as much
// On an ESP2 this is not av significant problem

// Alle values for PWM width and resolutions need to be changed if new resolution is needed
#define MOTOR_TIMER_BIT_RES 10
#define MOTOR_TIMER_MAX 1023
#define MOTOR_1_MS_PWM 51  // (1ms/(20ms)) * 2^resolution - 1
#define MOTOR_2_MS_PWM 102 // (2ms/(20ms)) * 2^resolution - 1

/*
 * Takes in speed values between MOTOR_1_MS_PWM and MOTOR_2_MS_PWM (1ms and 2ms on-time in PWM)
 * Give max resolution on PWM, should only be used internally in motor module.
 */
static void motorBLDCSetSpeed(uint32_t speed, uint8_t motorChannel)
{
  if ((speed > MOTOR_2_MS_PWM) || (speed < MOTOR_1_MS_PWM))
  {
    Serial.printf("Invalid motor speed\n");
    return;
  }
  else
  {
    ledcWrite(motorChannel, speed);
  }
}

void motorInit(Motor motor)
{
  pinMode(motor.pin0, OUTPUT);
  ledcSetup(motor.channel0, motor.freq, MOTOR_TIMER_BIT_RES);
  ledcAttachPin(motor.pin0, motor.channel0);
  // DC
  if (motor.motorType == MOTOR_TYPE_DC)
  { // DC uses 2 pins
    pinMode(motor.pin1, OUTPUT);
    ledcSetup(motor.channel1, motor.freq, MOTOR_TIMER_BIT_RES);
    ledcAttachPin(motor.pin1, motor.channel1);
    Serial.printf("DC ");
  }

  // BLDC
  if (motor.motorType == MOTOR_TYPE_BLDC)
  {
    setMotorSpeed(-127, motor);
    Serial.printf("BLDC ");
  }

  Serial.printf("motor set up pin %i,  chan %i,  freq %i\n", motor.pin0, motor.channel0, motor.freq);
}

void setMotorSpeed(int8_t speed, Motor motor)
{
  if (motor.motorType == MOTOR_TYPE_BLDC)
  {
    const int8_t min_speed = -127;
    const int8_t max_speed = 127;
    // This is where we could take advantage of 16 bit resolution to ramp up more slowly, but ESP does not support high resolution in all cases
    uint32_t mapped_speed = map(speed, min_speed, max_speed, MOTOR_1_MS_PWM, MOTOR_2_MS_PWM); // 3277, 6553);
    motorBLDCSetSpeed(mapped_speed, motor.channel0);
    int freq = ledcReadFreq(motor.channel0);
  }
  else if (motor.motorType == MOTOR_TYPE_DC)
  {
    if (speed > 0)
    {
      uint32_t mapped_speed = map(speed, 0, 127, 0, 1023);
      ledcWrite(motor.channel0, 0);
      ledcWrite(motor.channel1, mapped_speed);
    }
    else if (speed < 0)
    {
      uint32_t mapped_speed = map(speed, 0, -127, 0, 1023);
      ledcWrite(motor.channel1, 0);
      ledcWrite(motor.channel0, mapped_speed);
    }
    else if (speed == 0)
    {
      ledcWrite(motor.channel0, speed);
      ledcWrite(motor.channel1, speed);
    }
  }
  else
  {
    Serial.printf("Invalid motor type\n");
  }
}