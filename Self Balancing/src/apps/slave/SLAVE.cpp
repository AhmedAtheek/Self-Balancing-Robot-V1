// Libraries
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <PID_v1.h>
#include <PS4Controller.h>

const char* PS4_MAC = "4C:5F:70:CE:F9:31";

// L298N motor driver pins
const int enA = 19; // Enable pin for motor A - PWM
const int enB = 18; // Enable pin for motor B - PWM
const int in1 = 25; // Input 1 for motor A - Direction
const int in2 = 26; // Input 2 for motor A - Direction
const int in3 = 32; // Input 3 for motor B - Direction
const int in4 = 33; // Input 4 for motor B - Direction

// PWM properties - Increased to 12-bit resolution
const int pwmFreq = 7000; // Increased from 5000 to 7000 Hz for better motor control
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmRes = 12; // 12-bit resolution (0-4095) instead of 8-bit

// MPU6050 object
MPU6050 mpu(Wire);

// PID variables for balancing
double setPoint = 0;  // Desired angle of 0 degrees
double input, output;

// PID gain values
double Kp = 150.0;
double Ki = 10.0;
double Kd = 15.0;
int activeParameter = 0;

// Button states
bool triangle_old = false;
bool circle_old = false;
bool cross_old = false;
bool up_old = false;
bool down_old = false;
bool r1_old = false;

bool balanceMode = true;

// PID controller object
PID myPID(&input, &output, &setPoint, Kp, Ki, Kd, DIRECT);

// Timing variables
unsigned long lastPIDUpdate = 0;
const unsigned long PIDInterval = 4; // PID runs every 8ms

// Motor variables
int motorSpeed = 0;
int Lspeed = 0;
int Rspeed = 0;

// Deadband threshold - ignore small corrections
const int DEADBAND = 10;

// Task handles
TaskHandle_t pidTaskHandle;

// Function prototypes
void pidLoop(void *parameter);
void motorControl();
void testMotors();

void setup() {
  Serial.begin(115200);
  
  Serial.println("Self-balancing robot initialization...");
  
  // PS4 controller setup
  PS4.begin(PS4_MAC);
  Serial.println("Waiting for controller to connect...");
  
  // Initialize L298N motor driver pins
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  
  // Configure PWM channels with 12-bit resolution
  ledcSetup(pwmChannelA, pwmFreq, pwmRes);
  ledcSetup(pwmChannelB, pwmFreq, pwmRes);
  ledcAttachPin(enA, pwmChannelA);
  ledcAttachPin(enB, pwmChannelB);
  
  // Stop motors initially
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);

  // Initialize MPU6050
  Wire.begin();
  byte status = mpu.begin();
  if (status != 0) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }
  Serial.println("MPU6050 initialized successfully");
  
  // Test motors first to verify connections
  testMotors();
  delay(1000);
  
  // Calibrate MPU6050 with original method
  Serial.println("Calibrating gyroscope - keep the robot still...");
  mpu.calcOffsets(true, true);
  Serial.println("Calibration complete");
  
  // Initialize PID controller
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-4000, 4000); // Increased to 12-bit range (±4000)
  
  // Create a task for the PID loop on core 0
  xTaskCreatePinnedToCore(pidLoop, "PID Task", 10000, NULL, 2, &pidTaskHandle, 0);
}

void gainControl() {
  if (PS4.isConnected()) {
    // Get joystick values
    int leftX = PS4.data.analog.stick.lx;
    int leftY = -PS4.data.analog.stick.ly; // Invert Y axis so positive is forward
    
    // Current button states
    bool triangle_current = PS4.data.button.triangle;
    bool circle_current = PS4.data.button.circle;
    bool cross_current = PS4.data.button.cross;
    bool up_current = PS4.data.button.up;
    bool down_current = PS4.data.button.down;
    bool r1_current = PS4.data.button.r1;
    
    // R1 button toggle for balance mode
    if (r1_current && !r1_old) {
      balanceMode = !balanceMode;
      Serial.print("Balance mode: ");
      Serial.println(balanceMode ? "ON" : "OFF");
      
      // Reset PID integrator when toggling to prevent windup
      if (balanceMode) {
        myPID.SetMode(AUTOMATIC);
      } else {
        myPID.SetMode(MANUAL);
      }
    }
    
    // Switch between parameters with triangle, circle, and x buttons
    // Only trigger on button press (rising edge)
    if (triangle_current && !triangle_old) {
      activeParameter = 0; // Kp
      Serial.print("Active parameter: Kp = ");
      Serial.println(Kp, 3); // Print with 3 decimal places
    }
    
    if (circle_current && !circle_old) {
      activeParameter = 1; // Ki
      Serial.print("Active parameter: Ki = ");
      Serial.println(Ki, 3); // Print with 3 decimal places
    }
    
    if (cross_current && !cross_old) {
      activeParameter = 2; // Kd
      Serial.print("Active parameter: Kd = ");
      Serial.println(Kd, 3); // Print with 3 decimal places
    }
    
    // Determine increment size
    double increment;
    if (PS4.data.button.l2) {
      // L2 pressed - decimal changes
      increment = 0.1;
      if (PS4.data.button.l1) {
        // L1 also pressed - smaller decimal changes
        increment = 0.01;
      }
    } else {
      // Normal mode - integer changes
      increment = PS4.data.button.l1 ? 1.0 : 10.0; // L1 pressed = increment by 1, otherwise by 10
    }
    
    if (up_current && !up_old) {
      // Increase the active parameter
      switch (activeParameter) {
        case 0:
          Kp += increment;
          Serial.print("Kp = ");
          Serial.println(Kp, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
        case 1:
          Ki += increment;
          Serial.print("Ki = ");
          Serial.println(Ki, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
        case 2:
          Kd += increment;
          Serial.print("Kd = ");
          Serial.println(Kd, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
      }
    }
    
    if (down_current && !down_old) {
      // Decrease the active parameter
      switch (activeParameter) {
        case 0:
          Kp -= increment;
          Kp = max(0.0, Kp); // Prevent negative values
          Serial.print("Kp = ");
          Serial.println(Kp, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
        case 1:
          Ki -= increment;
          Ki = max(0.0, Ki); // Prevent negative values
          Serial.print("Ki = ");
          Serial.println(Ki, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
        case 2:
          Kd -= increment;
          Kd = max(0.0, Kd); // Prevent negative values
          Serial.print("Kd = ");
          Serial.println(Kd, 3);
          myPID.SetTunings(Kp, Ki, Kd);
          break;
      }
    }
    
    // Update previous button states for the next loop iteration
    triangle_old = triangle_current;
    circle_old = circle_current;
    cross_old = cross_current;
    up_old = up_current;
    down_old = down_current;
    r1_old = r1_current; 
  }
}

void loop() {
  // Motor control logic runs on core 1
  gainControl();
  
  // Only run motor control when balance mode is on
  if (balanceMode) {
    motorControl();
  } else {
    // Stop motors when not in balance mode
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    ledcWrite(pwmChannelA, 0);
    ledcWrite(pwmChannelB, 0);
  }
  
  // Small delay to prevent watchdog timer issues
  delay(1);
}

void testMotors() {
  Serial.println("Testing motors...");
  
  // Test Left Motor Forward
  Serial.println("Left motor forward");
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  ledcWrite(pwmChannelA, 2000); // ~50% of 12-bit range
  delay(1000);
  ledcWrite(pwmChannelA, 0);
  
  // Test Right Motor Forward
  Serial.println("Right motor forward");
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  ledcWrite(pwmChannelB, 2000); // ~50% of 12-bit range
  delay(1000);
  ledcWrite(pwmChannelB, 0);
  
  // Test both motors backward
  Serial.println("Both motors backward");
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  ledcWrite(pwmChannelA, 2000);
  ledcWrite(pwmChannelB, 2000);
  delay(1000);
  
  // Stop motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);
  
  Serial.println("Motor test complete");
}

void pidLoop(void *parameter) {
  unsigned long heartbeat = 0;
  
  while (true) {
    // Check if balance mode is on
    if (balanceMode) {
      unsigned long currentMillis = millis();
      
      if (currentMillis - lastPIDUpdate >= PIDInterval) {
        lastPIDUpdate = currentMillis;
        
        // Update MPU6050 angle - using the library's built-in methods
        mpu.update();
        double angle = mpu.getAngleX();  // Read the X-axis angle for balancing
        
        // Apply gentle smoothing filter to prevent wild readings
        // static double smoothedAngle = 0.0;
        // smoothedAngle = smoothedAngle * 0.9 + angle * 0.1; // 90% old value, 10% new value
        
        // Update PID controller with filtered angle
        input = angle;
        bool pidComputed = myPID.Compute();
        
        // Scale PID output - with 12-bit resolution
        motorSpeed = (int)output;
        
        // Log angles every 500ms for debugging
        if (currentMillis - heartbeat > 500) {
          heartbeat = currentMillis;
          Serial.print("Raw Angle: ");
          Serial.print(angle);
          // Serial.print(", Filtered Angle: ");
          // Serial.print(smoothedAngle);
          Serial.print(", Motor: ");
          Serial.println(motorSpeed);
        }
      }
    } else {
      // Reset PID integrator when not in balance mode to prevent windup
      myPID.SetMode(MANUAL);
      myPID.SetMode(AUTOMATIC);
      
      // Reset motor speed
      motorSpeed = 0;
    }
    
    // Always have a small delay to prevent watchdog timer issues
    delay(1);
  }
}

void motorControl() {
  // Apply deadband - ignore small corrections
  if (abs(motorSpeed) < DEADBAND) {
    // Stop motors if correction is small
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    ledcWrite(pwmChannelA, 0);
    ledcWrite(pwmChannelB, 0);
    return;
  }
  
  // Calculate motor speeds with 12-bit range
  // TT motors can handle full 12-bit range (max 4095)
  int baseSpeed = map(abs(motorSpeed), DEADBAND, 4000, 700, 4095);
  baseSpeed = constrain(baseSpeed, 1000, 4095);
  
  Lspeed = Rspeed = baseSpeed;
  
  // Adjust speed based on PS4 controller input if connected
  if (PS4.isConnected()) {
    int leftX = PS4.data.analog.stick.lx;
    
    // Apply simple differential steering when turning
    if (abs(leftX) > 20) {
      // Scale down one side for turning, max reduction of 30%
      float turnFactor = map(abs(leftX), 20, 127, 0, 30) / 100.0;
      
      if (leftX > 0) {
        // Turn right - reduce right motor speed
        Rspeed = baseSpeed * (1.0 - turnFactor);
      } else {
        // Turn left - reduce left motor speed
        Lspeed = baseSpeed * (1.0 - turnFactor);
      }
    }
  }
  
  // Set motor directions based on PID output
  if (motorSpeed > 0) {
    // Forward motion
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  } else {
    // Backward motion
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  }
  
  // Apply speed to motors with 12-bit resolution
  ledcWrite(pwmChannelA, Lspeed);
  ledcWrite(pwmChannelB, Rspeed);
}