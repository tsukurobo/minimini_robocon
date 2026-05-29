#include <Arduino.h>
#include <Servo.h>

#define PIN_SERVO_RIGHT  1
#define PIN_SERVO_LEFT   0
#define PIN_ADC_RIGHT   26
#define PIN_ADC_CENTER  27
#define PIN_ADC_LEFT    28
#define PIN_TRIG         4
#define PIN_ECHO         5

const int THRESHOLD          = 800;
const float STOP_DISTANCE_CM = 3.0;
const int PULSE_STOP         = 1500;
const int PULSE_FWD_NORMAL   = 1080;
const int PULSE_FWD_BOOST    = 1000;
const int PULSE_SLOW         = 1350;
const int PULSE_BACK         = 1700;
const int STRAIGHT_THRESHOLD = 8;

inline int mirrorPulse(int pulse) { return 3000 - pulse; }

// ★ enum を先に定義
enum Action {
    ACTION_FORWARD,
    ACTION_CURVE_RIGHT,
    ACTION_CURVE_LEFT,
    ACTION_SPIN_RIGHT,
    ACTION_SPIN_LEFT
};

// ★ enum の後にグローバル変数を宣言
Action lastAction      = ACTION_FORWARD;
int    sameActionCount = 0;
int    straightCount   = 0;
int    obstacleCount   = 0;

Servo servoRight;
Servo servoLeft;

void setServos(int pulseR, int pulseL) {
    servoRight.writeMicroseconds(pulseR);
    servoLeft.writeMicroseconds(pulseL);
}

void driveStop()      { setServos(PULSE_STOP, PULSE_STOP); }
void driveSpinRight() { setServos(PULSE_BACK, mirrorPulse(PULSE_FWD_NORMAL)); }
void driveSpinLeft()  { setServos(PULSE_FWD_NORMAL, mirrorPulse(PULSE_BACK)); }
void driveSpinCW()    { setServos(PULSE_BACK, mirrorPulse(PULSE_FWD_NORMAL)); }

void driveForward() {
    int pulse = (straightCount >= STRAIGHT_THRESHOLD) ? PULSE_FWD_BOOST : PULSE_FWD_NORMAL;
    setServos(pulse, mirrorPulse(pulse));
}

void driveCurve(int innerPulse, bool turnRight) {
    if (turnRight) setServos(innerPulse, mirrorPulse(PULSE_FWD_NORMAL));
    else           setServos(PULSE_FWD_NORMAL, mirrorPulse(innerPulse));
}

struct SensorState { bool onRight, onCenter, onLeft; };

SensorState readSensors() {
    return {
        analogRead(PIN_ADC_RIGHT)  <= THRESHOLD,
        analogRead(PIN_ADC_CENTER) <= THRESHOLD,
        analogRead(PIN_ADC_LEFT)   <= THRESHOLD
    };
}

float measureDistance() {
    digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    long duration = pulseIn(PIN_ECHO, HIGH, 30000);
    if (duration == 0) return 999.0;
    return (duration / 2.0) / 29.1;
}

Action decideAction(const SensorState& s) {
    if (s.onRight && !s.onLeft)
        return s.onCenter ? ACTION_CURVE_RIGHT : ACTION_SPIN_RIGHT;
    if (s.onLeft && !s.onRight)
        return s.onCenter ? ACTION_CURVE_LEFT : ACTION_SPIN_LEFT;
    return ACTION_FORWARD;
}

int smoothedInnerPulse(int targetPulse) {
    if (sameActionCount < 2) return (PULSE_FWD_NORMAL + targetPulse) / 2;
    return targetPulse;
}

void setup() {
    Serial.begin(115200);
    servoRight.attach(PIN_SERVO_RIGHT, 1000, 2000);
    servoLeft.attach(PIN_SERVO_LEFT,   1000, 2000);
    driveStop();
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    Serial.println("起動");
    delay(1000);
}

void loop() {
    float dist = measureDistance();

    if (dist <= STOP_DISTANCE_CM) {
        obstacleCount++;
        straightCount = 0;
        Serial.print("障害物検知 ("); Serial.print(obstacleCount); Serial.print("回目): ");
        Serial.print(dist); Serial.println("cm");

        if (obstacleCount == 1) {
            Serial.println("時計回り回転 1.2秒");
            driveSpinCW();
            delay(1200);
            Serial.println("前進 1秒");
            driveForward();
            delay(1000);
        } else if (obstacleCount >= 2) {
            driveStop();
            Serial.println("2回目の障害物検知 → 終了");
            while (true) { delay(1000); }
        }
        lastAction      = ACTION_FORWARD;
        sameActionCount = 0;
        return;
    }

    SensorState state  = readSensors();
    Action      action = decideAction(state);

    if (action == ACTION_FORWARD) {
        if (straightCount < STRAIGHT_THRESHOLD + 1) straightCount++;
    } else {
        straightCount = 0;
    }

    if (action == lastAction) sameActionCount++;
    else                      sameActionCount = 0;
    lastAction = action;

    switch (action) {
        case ACTION_FORWARD:
            driveForward();
            break;
        case ACTION_CURVE_RIGHT:
            driveCurve(smoothedInnerPulse(PULSE_SLOW), true);
            break;
        case ACTION_CURVE_LEFT:
            driveCurve(smoothedInnerPulse(PULSE_SLOW), false);
            break;
        case ACTION_SPIN_RIGHT:
            driveSpinRight();
            break;
        case ACTION_SPIN_LEFT:
            driveSpinLeft();
            break;
    }

    delay(10);
}