/*
  Wild Animal Detection System - Tinkercad (Arduino Uno) simulation
  Tinkercad has no ESP32, so an Uno stands in for it. Wi-Fi and the web
  dashboard cannot be simulated; the status packet is printed on the
  Serial Monitor instead.

  Wiring (Tinkercad)
    Sensor 1 (4 ft): VCC 5V, GND, TRIG -> D9,  ECHO -> D10
    Sensor 2 (6 ft): VCC 5V, GND, TRIG -> D11, ECHO -> D12
    Piezo buzzer:    + -> D6, - -> GND   (stands in for L298N + buzzer)
    Push button:     one leg -> D2, other leg -> GND  (manual override)
    Green LED (SAFE):  D4 -> 220 ohm -> LED -> GND
    Red LED (ACTIVE):  D5 -> 220 ohm -> LED -> GND

  How to test
    - Both sensors far (over 100 cm): SAFE
    - Only sensor 1 close:           likely animal -> buzzer + red LED
    - Both sensors close:            likely human  -> no deterrent
    - Press the button:              manual override on/off
*/

const int TRIG1 = 9,  ECHO1 = 10;   // 4 ft sensor (presence)
const int TRIG2 = 11, ECHO2 = 12;   // 6 ft sensor (height)
const int BUZZER = 6;
const int BTN = 2;
const int LED_SAFE = 4;
const int LED_ALERT = 5;

const int DETECT_CM = 100;                 // object counts as present below this
const unsigned long DETERRENT_MS = 15000;  // report logged a 15 s run; lower it for quick demos
const unsigned long STATUS_MS = 1000;

bool override = false;
unsigned long deterrentUntil = 0;
unsigned long lastStatus = 0;
unsigned long lastPress = 0;
int lastBtn = HIGH;

long readCm(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long us = pulseIn(echo, HIGH, 30000);    // 30 ms timeout
  if (us == 0) return -1;                  // no echo
  return us / 58;
}

void setup() {
  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_SAFE, OUTPUT);
  pinMode(LED_ALERT, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Wild Animal Detection System - Block A (simulation)");
}

void loop() {
  long d4 = readCm(TRIG1, ECHO1);
  delay(30);
  long d6 = readCm(TRIG2, ECHO2);

  bool near4 = (d4 > 0 && d4 < DETECT_CM);
  bool near6 = (d6 > 0 && d6 < DETECT_CM);

  const char* result = "SAFE";
  if (near4 && !near6) {
    result = "LIKELY ANIMAL";
    deterrentUntil = millis() + DETERRENT_MS;
  } else if (near4 && near6) {
    result = "LIKELY HUMAN";
  } else if (!near4 && near6) {
    result = "UNUSUAL READING";            // not treated as an intrusion
  }

  // Manual override button (toggle, debounced)
  int b = digitalRead(BTN);
  if (b == LOW && lastBtn == HIGH && millis() - lastPress > 250) {
    override = !override;
    lastPress = millis();
  }
  lastBtn = b;

  bool active = override || (millis() < deterrentUntil);
  if (active) tone(BUZZER, 2000); else noTone(BUZZER);
  digitalWrite(LED_ALERT, active ? HIGH : LOW);
  digitalWrite(LED_SAFE, active ? LOW : HIGH);

  // Status packet (what the ESP32 would send over Wi-Fi)
  if (millis() - lastStatus >= STATUS_MS) {
    lastStatus = millis();
    Serial.print("zone=Block A, t="); Serial.print(millis() / 1000);
    Serial.print("s, d6ft="); if (d6 > 0) Serial.print(d6); else Serial.print("--");
    Serial.print("cm, d4ft="); if (d4 > 0) Serial.print(d4); else Serial.print("--");
    Serial.print("cm, result="); Serial.print(result);
    Serial.print(", deterrent="); Serial.print(active ? "ACTIVE" : "STANDBY");
    Serial.println(override ? " (OVERRIDE)" : "");
  }
}
