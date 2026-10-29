#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <BleMouse.h>
#include <math.h>

MPU6050 mpu;
BleMouse bleMouse("taoAirMouse", "ESP32", 100);

// Button pins
#define LEFT_BUTTON_PIN  5   // Left click
#define RIGHT_BUTTON_PIN 10  // Right click

// Gyro zero offsets, calibrated at startup (device must be still)
int16_t gxOff = 0, gyOff = 0, gzOff = 0;

// --- Tuning knobs ---
#define GYRO_LSB_PER_DPS 131.0f  // MPU6050 @ +-250 deg/s: 131 LSB per deg/s
#define DEADZONE_DPS     2.0f    // ignore rotation slower than this (deg/s)
#define SENSITIVITY      0.30f   // cursor pixels per deg/s (bigger = faster)
#define MAX_STEP         30      // clamp a single move (px)

void calibrateGyro() {
  const int N = 300;
  long sx = 0, sy = 0, sz = 0;
  int16_t gx, gy, gz;
  for (int i = 0; i < N; i++) {
    mpu.getRotation(&gx, &gy, &gz);
    sx += gx; sy += gy; sz += gz;
    delay(2);
  }
  gxOff = sx / N;
  gyOff = sy / N;
  gzOff = sz / N;
  Serial.printf("Gyro zero: gx=%d gy=%d gz=%d\n", gxOff, gyOff, gzOff);
}

// Convert gyro angular velocity (raw) to a cursor step (float px),
// applying deadzone and clamping. Returns fractional pixels so slow
// movement is preserved by the accumulator in loop().
float gyroToStep(int16_t raw, int16_t off) {
  float dps = (raw - off) / GYRO_LSB_PER_DPS;
  if (fabsf(dps) < DEADZONE_DPS) return 0.0f;
  float step = dps * SENSITIVITY;
  if (step > MAX_STEP) step = MAX_STEP;
  else if (step < -MAX_STEP) step = -MAX_STEP;
  return step;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Boot OK");

  // MPU6050 init (SDA=21, SCL=20)
  Wire.begin(21, 20);
  mpu.initialize();
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250); // +-250 deg/s = most precise

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  // Keep the device STILL during this ~0.6s calibration.
  calibrateGyro();

  // BLE Mouse init
  bleMouse.begin();

  // Button setup (internal pull-ups)
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (bleMouse.isConnected()) {
    int16_t gx, gy, gz;
    mpu.getRotation(&gx, &gy, &gz);

    // Pointing-remote style: yaw (gz) -> left/right, pitch (gx) -> up/down.
    // Signs match your working orientation; flip if a direction is inverted.
    float stepX = -gyroToStep(gz, gzOff);
    float stepY =  gyroToStep(gx, gxOff);

    // Fractional accumulator: emit integer pixels while keeping the remainder,
    // so slow movement stays smooth instead of truncating to zero.
    static float accX = 0, accY = 0;
    accX += stepX;
    accY += stepY;
    int dx = (int)accX;  accX -= dx;
    int dy = (int)accY;  accY -= dy;

    bleMouse.move(dx, dy);

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
  }

  delay(5);  // ~150 Hz update; lower = snappier
}
