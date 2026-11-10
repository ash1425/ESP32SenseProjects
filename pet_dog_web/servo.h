#pragma once
#include <Arduino.h>

#define SERVO_FL_PIN  3
#define SERVO_FR_PIN  4
#define SERVO_BL_PIN  5
#define SERVO_BR_PIN  6

#define SERVO_MIN_US   500
#define SERVO_MAX_US   2500
#define SERVO_FREQ_HZ  50
#define SERVO_NEUTRAL  90

#define LED_PIN 21

void servo_init();
void servo_set_angle(uint8_t pin, int degrees);
void servo_all_neutral();

void anim_good_boy();
void anim_sit_down();
void anim_lie_down();
void anim_stretch();
void anim_walk();
void anim_dance();
void anim_wiggle();
void anim_test();
void anim_solo(int leg);
