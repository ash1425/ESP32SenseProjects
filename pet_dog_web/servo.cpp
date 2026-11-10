#include "servo.h"

typedef struct {
    int fl, fr, bl, br;
} legs_t;

static legs_t cur = {90, 90, 90, 90};

static uint32_t angle_to_duty(int degrees) {
    int pulse_us = SERVO_MIN_US + (degrees * (SERVO_MAX_US - SERVO_MIN_US)) / 180;
    return (uint32_t)((pulse_us * 16383) / 20000);
}

static void set_fl(int deg) {
    cur.fl = deg;
    servo_set_angle(SERVO_FL_PIN, 180 - deg);
}
static void set_fr(int deg) {
    cur.fr = deg;
    servo_set_angle(SERVO_FR_PIN, deg);
}
static void set_bl(int deg) {
    cur.bl = deg;
    servo_set_angle(SERVO_BL_PIN, 180 - deg);
}
static void set_br(int deg) {
    cur.br = deg;
    servo_set_angle(SERVO_BR_PIN, deg);
}

static void pwm_off(uint8_t pin) {
    ledcWrite(pin, 0);
}

static void pwm_off_all() {
    pwm_off(SERVO_FL_PIN);
    pwm_off(SERVO_FR_PIN);
    pwm_off(SERVO_BL_PIN);
    pwm_off(SERVO_BR_PIN);
}

static void ease_to(legs_t to, int duration_ms) {
    legs_t from = cur;
    int steps = 20;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float e = t * t * (3.0f - 2.0f * t);
        set_fl(from.fl + (int)((to.fl - from.fl) * e));
        set_fr(from.fr + (int)((to.fr - from.fr) * e));
        set_bl(from.bl + (int)((to.bl - from.bl) * e));
        set_br(from.br + (int)((to.br - from.br) * e));
        delay(duration_ms / steps);
    }
}

static void reset_to_neutral() {
    legs_t neutral = {90, 90, 90, 90};
    ease_to(neutral, 500);
    delay(150);
}

// --- Public API ---

void servo_init() {
    ledcAttach(SERVO_FL_PIN, SERVO_FREQ_HZ, 14);
    ledcAttach(SERVO_FR_PIN, SERVO_FREQ_HZ, 14);
    ledcAttach(SERVO_BL_PIN, SERVO_FREQ_HZ, 14);
    ledcAttach(SERVO_BR_PIN, SERVO_FREQ_HZ, 14);
}

void servo_set_angle(uint8_t pin, int degrees) {
    if (degrees < 0) degrees = 0;
    if (degrees > 180) degrees = 180;
    ledcWrite(pin, angle_to_duty(degrees));
}

void servo_all_neutral() {
    reset_to_neutral();
    delay(200);
    pwm_off_all();
}

// --- Animations ---

void anim_good_boy() {
    reset_to_neutral();
    int wag_angles[] = {75, 105, 70, 110, 75, 105, 82, 98, 90};
    int wag_times[]  = {120, 120, 110, 110, 100, 100, 90, 90, 80};
    int n = sizeof(wag_angles) / sizeof(wag_angles[0]);
    for (int i = 0; i < n; i++) {
        set_bl(wag_angles[i]);
        set_br(180 - wag_angles[i]);
        delay(wag_times[i]);
    }
    ease_to((legs_t){90, 90, 90, 90}, 300);
    delay(100);
    pwm_off_all();
}

void anim_sit_down() {
    reset_to_neutral();
    legs_t lean    = {75, 75, 90, 90};
    legs_t sit     = {90, 90, 110, 110};
    legs_t bounce  = {75, 75, 120, 120};
    legs_t settle  = {90, 90, 135, 135};
    ease_to(lean, 600);   delay(200);
    ease_to(sit, 700);    delay(200);
    ease_to(bounce, 150); delay(80);
    ease_to(settle, 150); delay(400);
    pwm_off_all();
}

void anim_lie_down() {
    reset_to_neutral();
    legs_t front     = {60, 60, 90, 90};
    legs_t all_down  = {80, 80, 60, 60};
    legs_t overshoot = {120, 120, 50, 50};
    legs_t settled   = {180, 180, 0, 0};
    ease_to(front, 500);     delay(150);
    ease_to(all_down, 500);  delay(100);
    ease_to(overshoot, 200); delay(80);
    ease_to(settled, 600);   delay(300);
    pwm_off_all();
}

void anim_stretch() {
    reset_to_neutral();
    ease_to((legs_t){160, 160, 90, 90}, 1000); delay(600);
    ease_to((legs_t){90, 90, 90, 90}, 800);    delay(300);
    ease_to((legs_t){90, 90, 30, 30}, 1000);   delay(600);
    ease_to((legs_t){90, 90, 90, 90}, 800);    delay(200);
    pwm_off_all();
}

void anim_walk() {
    reset_to_neutral();
    legs_t stance = {100, 100, 80, 80};
    ease_to(stance, 600); delay(300);
    for (int i = 0; i < 8; i++) {
        ease_to((legs_t){130, 130, 80, 80}, 250);
        ease_to((legs_t){130, 130, 60, 60}, 250);
        delay(80);
        ease_to((legs_t){70, 70, 60, 60}, 250);
        ease_to((legs_t){70, 70, 110, 110}, 250);
        delay(80);
    }
    ease_to(stance, 400);
    ease_to((legs_t){90, 90, 90, 90}, 500);
    delay(200);
    pwm_off_all();
}

void anim_dance() {
    reset_to_neutral();

    legs_t swagger_a = {60, 90, 90, 60};
    legs_t swagger_b = {90, 60, 60, 90};
    for (int i = 0; i < 4; i++) {
        ease_to(swagger_a, 350); delay(100);
        ease_to(swagger_b, 350); delay(100);
    }
    ease_to((legs_t){90, 90, 90, 90}, 400); delay(200);

    legs_t lean_fwd  = {120, 120, 120, 120};
    legs_t lean_back = {60, 60, 60, 60};
    ease_to(lean_fwd, 600);  delay(300);
    ease_to(lean_back, 600); delay(300);
    ease_to(lean_fwd, 500);  delay(200);
    ease_to(lean_back, 500); delay(200);
    ease_to(lean_fwd, 400);  delay(150);
    ease_to(lean_back, 400); delay(150);
    ease_to((legs_t){90, 90, 90, 90}, 500); delay(300);

    legs_t front    = {60, 60, 90, 90};
    legs_t all_down = {80, 80, 60, 60};
    legs_t flat     = {180, 180, 0, 0};
    ease_to(front, 500);    delay(150);
    ease_to(all_down, 500); delay(100);
    ease_to(flat, 700);     delay(800);

    legs_t shimmy_a = {170, 180, 0, 10};
    legs_t shimmy_b = {180, 170, 10, 0};
    for (int i = 0; i < 5; i++) {
        ease_to(shimmy_a, 250); delay(50);
        ease_to(shimmy_b, 250); delay(50);
    }
    ease_to(flat, 300); delay(400);

    ease_to(all_down, 600); delay(150);
    ease_to(front, 600);    delay(150);
    ease_to((legs_t){90, 90, 90, 90}, 600); delay(300);
    pwm_off_all();
}

void anim_wiggle() {
    reset_to_neutral();
    legs_t left  = {60, 120, 60, 120};
    legs_t right = {120, 60, 120, 60};
    for (int i = 0; i < 2; i++) {
        ease_to(left, 200);
        ease_to(right, 200);
    }
    ease_to((legs_t){90, 90, 90, 90}, 300);
    delay(100);
    pwm_off_all();
}

void anim_test() {
    reset_to_neutral();
    delay(500);
    for (int i = 0; i < 10; i++) {
        Serial.printf("TEST cycle %d/10 — FL\n", i + 1);
        set_fl(30);  delay(250);
        set_fl(150); delay(250);
        set_fl(90);  delay(100);

        Serial.printf("TEST cycle %d/10 — FR\n", i + 1);
        set_fr(30);  delay(250);
        set_fr(150); delay(250);
        set_fr(90);  delay(100);

        Serial.printf("TEST cycle %d/10 — BL\n", i + 1);
        set_bl(30);  delay(250);
        set_bl(150); delay(250);
        set_bl(90);  delay(100);

        Serial.printf("TEST cycle %d/10 — BR\n", i + 1);
        set_br(30);  delay(250);
        set_br(150); delay(250);
        set_br(90);  delay(100);
    }
    ease_to((legs_t){90, 90, 90, 90}, 300);
    pwm_off_all();
}

void anim_solo(int leg) {
    void (*setter)(int) = NULL;
    const char *name = "";
    switch (leg) {
        case 0: setter = set_fl; name = "FL"; break;
        case 1: setter = set_fr; name = "FR"; break;
        case 2: setter = set_bl; name = "BL"; break;
        case 3: setter = set_br; name = "BR"; break;
        default: return;
    }
    Serial.printf("SOLO %s — sweep 30>150>90\n", name);
    setter(90); delay(300);
    for (int i = 0; i < 3; i++) {
        setter(30);  delay(400);
        setter(150); delay(400);
    }
    setter(90); delay(200);
    pwm_off_all();
}
