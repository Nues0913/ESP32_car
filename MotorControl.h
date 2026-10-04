#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Adafruit_PWMServoDriver.h>
#include "pinConfig.h"

// Global variables
extern Adafruit_PWMServoDriver pwm;
extern int head_delta;
extern int tail_delta;
extern int head_handler_left_bound;
extern int head_handler_right_bound;
extern int tail_handler_right_bound;
extern int tail_handler_left_bound;
extern short carStatus;
extern int threshold;
extern int handler;

// Function declarations
void initMotor();
void carStop();
void forward(int value);
void reverse(int value);
int mapThrottle(int val, bool reverseDir);
void parseAndApply(char *s);

#endif
