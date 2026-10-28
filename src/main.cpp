#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu;
BleMouse bleMouse("taoAirMouse", "ESP32", 100);

// Button pins
#define LEFT_BUTTON_PIN  5   // Left click
#define RIGHT_BUTTON_PIN 10  // Right click

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Boot OK");

  // MPU6050 init (SDA=21, SCL=20)
  Wire.begin(21, 20);
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  // BLE Mouse init
  bleMouse.begin();

  // Button setup (internal pull-ups)
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (bleMouse.isConnected()) {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Scale down sensitivity
    int dx = gx / 800;
    int dy = -gy / 800;

    // Deadzone filter
    if (abs(dx) < 3) dx = 0;
    if (abs(dy) < 3) dy = 0;

    // Smoothing (simple moving average)
    static int avgDx = 0, avgDy = 0;
    avgDx = (avgDx * 4 + dx) / 5;
    avgDy = (avgDy * 4 + dy) / 5;

    // Move mouse
    bleMouse.move(avgDx, avgDy);

    // --- Button handling ---
    // Left click
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      bleMouse.press(MOUSE_LEFT);
    } else {
      bleMouse.release(MOUSE_LEFT);
    }

    // Right click
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      bleMouse.press(MOUSE_RIGHT);
    } else {
      bleMouse.release(MOUSE_RIGHT);
    }

    // Debugging
    Serial.print("dx: "); Serial.print(avgDx);
    Serial.print(" dy: "); Serial.print(avgDy);
    Serial.print(" L: "); Serial.print(digitalRead(LEFT_BUTTON_PIN));
    Serial.print(" R: "); Serial.println(digitalRead(RIGHT_BUTTON_PIN));
  }

  delay(10);
}