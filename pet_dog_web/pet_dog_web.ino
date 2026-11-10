#include <WiFi.h>
#include <WebServer.h>
#include "servo.h"
#include "web_page.h"

#define CMD_GOOD_BOY 0
#define CMD_SIT_DOWN 1
#define CMD_LIE_DOWN 2
#define CMD_STRETCH  3
#define CMD_WALK     4
#define CMD_DANCE    5
#define CMD_WIGGLE   99
#define CMD_TEST     98
#define CMD_SOLO_FL  90
#define CMD_SOLO_FR  91
#define CMD_SOLO_BL  92
#define CMD_SOLO_BR  93

const char *AP_SSID = "PetDog";
WebServer server(80);

void run_command(int id) {
    Serial.printf("Running command: %d\n", id);
    digitalWrite(LED_PIN, HIGH);

    switch (id) {
        case CMD_GOOD_BOY: anim_good_boy(); break;
        case CMD_SIT_DOWN: anim_sit_down(); break;
        case CMD_LIE_DOWN: anim_lie_down(); break;
        case CMD_STRETCH:  anim_stretch();  break;
        case CMD_WALK:     anim_walk();     break;
        case CMD_DANCE:    anim_dance();    break;
        case CMD_WIGGLE:   anim_wiggle();   break;
        case CMD_TEST:     anim_test();     break;
        case CMD_SOLO_FL:  anim_solo(0);    break;
        case CMD_SOLO_FR:  anim_solo(1);    break;
        case CMD_SOLO_BL:  anim_solo(2);    break;
        case CMD_SOLO_BR:  anim_solo(3);    break;
        default:
            Serial.printf("Unknown command: %d\n", id);
            break;
    }

    digitalWrite(LED_PIN, LOW);
}

void handle_root() {
    server.send(200, "text/html", HTML_PAGE);
}

void handle_cmd() {
    if (!server.hasArg("id")) {
        server.send(400, "text/plain", "Missing id");
        return;
    }
    int id = server.arg("id").toInt();
    server.send(200, "text/plain", "OK");
    run_command(id);
}

void handle_serial() {
    if (Serial.available() <= 0) return;

    int c = Serial.read();
    if (c >= '0' && c <= '5') {
        run_command(c - '0');
    } else if (c == '8') {
        run_command(CMD_TEST);
    } else if (c == '9') {
        run_command(CMD_WIGGLE);
    } else if (c == 'a' || c == 'A') {
        run_command(CMD_SOLO_FL);
    } else if (c == 'b' || c == 'B') {
        run_command(CMD_SOLO_FR);
    } else if (c == 'c' || c == 'C') {
        run_command(CMD_SOLO_BL);
    } else if (c == 'd' || c == 'D') {
        run_command(CMD_SOLO_BR);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("=== Pet Dog Web Control ===");

    servo_init();
    Serial.println("Servos initialized");
    servo_all_neutral();
    Serial.println("Servos at neutral");

    WiFi.softAP(AP_SSID);
    Serial.printf("WiFi AP started: %s\n", AP_SSID);
    Serial.printf("Connect and open: http://%s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", handle_root);
    server.on("/cmd", handle_cmd);
    server.begin();
    Serial.println("Web server started");

    Serial.println("Serial: 0-5=anims  8=test  9=wiggle  a=FL  b=FR  c=BL  d=BR");
    Serial.println("Ready!");
}

void loop() {
    server.handleClient();
    handle_serial();
}
