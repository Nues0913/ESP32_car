#include "MotorControl.h"
#include "I2cLock.h"
#include <Wire.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

int head_delta = 7;
int tail_delta = -6;
int head_handler_left_bound  = 285 + head_delta;
int head_handler_right_bound = 455 + head_delta;
int tail_handler_right_bound = 285 + tail_delta;
int tail_handler_left_bound  = 455 + tail_delta;
short carStatus = 0; // -1 reverse, 0 stop, 1 forward

int threshold = 128; // 0..255
int handler   = 128; // 0..255

void initMotor() {
    i2cLock();
    pwm.begin();
    pwm.setPWMFreq(50);
    
    carStop();
    pwm.setPWM(8, 0, map(128, 0, 255, head_handler_right_bound, head_handler_left_bound));
    pwm.setPWM(9, 0, map(128, 0, 255, tail_handler_left_bound, tail_handler_right_bound));
    i2cUnlock();
}

void carStop() {
    i2cLock();
    pwm.setPWM(7, 0, 0);
    pwm.setPWM(4, 0, 0);
    pwm.setPWM(3, 0, 0);
    pwm.setPWM(2, 0, 0);
    carStatus = 0;
    i2cUnlock();
}

void forward(int value) {
    i2cLock();
    pwm.setPWM(7, 0, value);
    pwm.setPWM(4, 0, value);
    pwm.setPWM(3, 0, value);
    pwm.setPWM(2, 0, value);

    pwm.setPWM(6, 4095, 0);
    pwm.setPWM(5, 4095, 0);
    pwm.setPWM(0, 4095, 0);
    pwm.setPWM(1, 4095, 0);
    carStatus = 1;
    i2cUnlock();
}

void reverse(int value) {
    i2cLock();
    pwm.setPWM(7, 0, value);
    pwm.setPWM(4, 0, value);
    pwm.setPWM(3, 0, value);
    pwm.setPWM(2, 0, value);

    pwm.setPWM(6, 0, 4095);
    pwm.setPWM(5, 0, 4095);
    pwm.setPWM(0, 0, 4095);
    pwm.setPWM(1, 0, 4095);
    carStatus = -1;
    i2cUnlock();
}

int mapThrottle(int val, bool reverseDir) {
    int minPWM = 300;
    int maxPWM = 4095;

    float inputMin = reverseDir ? 134.0 : 122.0;
    float inputMax = reverseDir ? 255.0 : 0.0;
    float raw = reverseDir ? (val - inputMin) / (inputMax - inputMin)
                           : (inputMin - val) / (inputMin - inputMax);

    raw = constrain(raw, 0.0f, 1.0f);

    float curve = pow(raw, 4);

    int pwm = minPWM + curve * (maxPWM - minPWM);
    return constrain(pwm, minPWM, maxPWM);
}

void parseAndApply(char *s) {
    int th = 128, hd = 128;
    char *p = strtok(s, ",\n\r ");
    while (p) {
        if (strncmp(p, "th=", 3) == 0)      th = atoi(p + 3);
        else if (strncmp(p, "hd=", 3) == 0) hd = atoi(p + 3);
        p = strtok(NULL, ",\n\r ");
    }
    th = constrain(th, 0, 255);
    hd = constrain(hd, 0, 255);

    threshold = th;
    handler   = hd;

    if (threshold > 133) {
        if (carStatus == 1) {
            carStop();
            delay(20);
        }
        reverse(mapThrottle(threshold, true));   
    }
    else if (threshold < 123) {
        if (carStatus == -1) {
            carStop();
            delay(20);
        }
        forward(mapThrottle(threshold, false));
    }
    else {
        carStop();
    }

    // 舵角（頭 8、尾 9）
    i2cLock();
    pwm.setPWM(8, 0, map(handler, 0, 255, head_handler_right_bound, head_handler_left_bound));
    // pwm.setPWM(9, 0, map(handler, 0, 255, tail_handler_right_bound, tail_handler_left_bound));
    i2cUnlock();
}
