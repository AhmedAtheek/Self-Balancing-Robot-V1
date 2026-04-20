#include <Arduino.h>
#include <Bluepad32.h>

// L298N motor driver pins
const int enA = 19; // Enable pin for motor A - PWM
const int enB = 18; // Enable pin for motor B - PWM
const int in1 = 25; // Input 1 for motor A - Direction
const int in2 = 26; // Input 2 for motor A - Direction
const int in3 = 32; // Input 3 for motor B - Direction
const int in4 = 33; // Input 4 for motor B - Direction

// PWM properties - 8-bit resolution
const int pwmFreq = 7000; // 7000 Hz for better motor control
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmRes = 8; // 8-bit resolution (0-255)

// PWM values for motor speed control
int pwmValue = 100; // Starting PWM value
const int pwmMin = 0;
const int pwmMax = 255; // Maximum for 8-bit resolution
const int pwmStep = 10; // Step size for PWM adjustments

// Button constants for PlayStation controllers
// These might need adjustment based on your specific controller
// We'll check the actual button mapping in the processGamepad function
const uint16_t BUTTON_SQUARE = 0x1000;    // Square is usually X for Xbox
const uint16_t BUTTON_CIRCLE = 0x2000;    // Circle is usually B for Xbox
const uint16_t BUTTON_L1 = 0x0100;        // L1/LB
const uint16_t BUTTON_L2 = 0x0400;        // L2/LT (might be a trigger instead)

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", 
                ctl->getModelName().c_str(), properties.vendor_id, properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not find empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

// Motor control functions
void driveLeftMotor() {
    // Set direction
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    
    // Set speed
    ledcWrite(pwmChannelA, pwmValue);
    Serial.println("Left motor running");
}

void driveRightMotor() {
    // Set direction
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    
    // Set speed
    ledcWrite(pwmChannelB, pwmValue);
    Serial.println("Right motor running");
}

void stopLeftMotor() {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(pwmChannelA, 0);
}

void stopRightMotor() {
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    ledcWrite(pwmChannelB, 0);
}

void stopMotors() {
    stopLeftMotor();
    stopRightMotor();
    Serial.println("All motors stopped");
}

void processGamepad(ControllerPtr ctl) {
    // Print the button state to help determine the correct button values
    if (ctl->hasData()) {
        static uint32_t lastPrintTime = 0;
        uint32_t currentTime = millis();
        
        // Only print every second to avoid flooding the console
        if (currentTime - lastPrintTime > 1000) {
            Serial.printf("Buttons: 0x%04x\n", ctl->buttons());
            lastPrintTime = currentTime;
        }
    }
    
    // Based on the example, we should use individual methods like a(), b(), etc.
    // Or check against button bitmasks from buttons()
    uint16_t buttons = ctl->buttons();
    
    // For PS controllers in Bluepad32, let's try both approaches
    // Approach 1: Check specific bits in the button bitmask
    
    // Check if Square is pressed (using button mapping for PS controllers)
    // For PS4, we need to first identify the correct button codes by printing them
    if (buttons & BUTTON_SQUARE) {
        driveLeftMotor();
    } else {
        stopLeftMotor();
    }
    
    // Check if Circle is pressed
    if (buttons & BUTTON_CIRCLE) {
        driveRightMotor();
    } else {
        stopRightMotor();
    }
    
    // Approach 2: Try using individual button methods if they exist
    // Let's try various methods based on common controller API patterns
    
    // Alternative approach for L1/L2 using trigger values (for controllers with analog triggers)
    if (ctl->l1()) {
        pwmValue = min(pwmValue + pwmStep, pwmMax);
        Serial.print("PWM increased to: ");
        Serial.println(pwmValue);
        delay(100); // Debounce
    }
    
    // L2 might be a trigger value
    if (ctl->brake() > 500) {  // Using brake() which usually corresponds to L2/LT
        pwmValue = max(pwmValue - pwmStep, pwmMin);
        Serial.print("PWM decreased to: ");
        Serial.println(pwmValue);
        delay(100); // Debounce
    }
    
    // As a fallback, also check buttons bitmask for L1/L2
    if (buttons & BUTTON_L1) {
        pwmValue = min(pwmValue + pwmStep, pwmMax);
        Serial.print("PWM increased to: ");
        Serial.println(pwmValue);
        delay(100); // Debounce
    }
    
    if (buttons & BUTTON_L2) {
        pwmValue = max(pwmValue - pwmStep, pwmMin);
        Serial.print("PWM decreased to: ");
        Serial.println(pwmValue);
        delay(100); // Debounce
    }
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            }
        }
    }
}

// Arduino setup function
void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    
    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
    // Configure motor control pins
    pinMode(enA, OUTPUT);
    pinMode(enB, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);
    
    // Configure PWM for motor speed control
    ledcSetup(pwmChannelA, pwmFreq, pwmRes);
    ledcSetup(pwmChannelB, pwmFreq, pwmRes);
    
    // Attach PWM channels to enable pins
    ledcAttachPin(enA, pwmChannelA);
    ledcAttachPin(enB, pwmChannelB);
    
    // Initially stop both motors
    stopMotors();
    
    // Forget Bluetooth keys to prevent reconnection issues
    BP32.forgetBluetoothKeys();
    
    Serial.println("Setup complete");
}

// Arduino loop function
void loop() {
    // This call fetches all the controllers' data
    bool dataUpdated = BP32.update();
    if (dataUpdated) {
        processControllers();
    }
    
    // Small delay to prevent CPU hogging
    delay(10);
}