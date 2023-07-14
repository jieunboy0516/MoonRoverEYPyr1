#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>

// Wireless variables
char ssid[] = "Nik";
char pass[] = "xyz55555";
unsigned int localPort = 2390;
char packetBuffer[255];
WiFiUDP Udp;

const char *server = "192.168.82.119";
IPAddress samsam;

// Signal detection variables
const byte radioPin = A0;           // Radio sensor output
const byte irPin = A1;              // IR sensor output
const byte magneticPin = A2;        // Hall sensor output
StaticJsonDocument<64> SensorData;  // Sensor data to be transmitted 64 bytes large

// Steering/movement constants
const float TURNING_FACTOR = 0.5;  // Number between 0 and 1 to control maximum sharpness of turns
const float RESPONSE_SPEED = 0.01;  // Controls how quickly a wheel reaches the target velocity
const float SWEEP_PERIOD = 3000;   // Period of sweeping cycle in ms
const int SWEEP_WIDTH = 0xb0;      // How wide the sweep range should be

// General useful values
unsigned long prevTime = 0;
int delta = 0;                                          // Time since last loop() call
Adafruit_NeoPixel pixels(1, 40, NEO_GRB + NEO_KHZ800);  // Enable built in NeoPixel

// Wheel state
bool F, B, L, R = false;
int leftWheelVelocity = 0;
int rightWheelVelocity = 0;
int sweepTime = 0;
bool boosting = false;

// For debugging
int pos = 0;
char inputString[5];

int exponential_steering(int current, int target) {
    int difference = abs(current - target);
    if (current < target) {
        current += difference * RESPONSE_SPEED;
        if (current > target) {
            current = target;
        }
    } else if (current > target) {
        current -= difference * RESPONSE_SPEED;
        if (current < target) {
            current = target;
        }
    }
    return current;
}

void boost() {
    leftWheelVelocity = 0xff;
    rightWheelVelocity = 0xff;
}

// Calculate left wheel velocity
void left_velocity() {
    int leftTargetSpeed;
    if (F == B && L == R) {
        leftTargetSpeed = 0;
    } else if (F != B && L && !R) {
        leftTargetSpeed = TURNING_FACTOR * 0xff;
    } else {
        leftTargetSpeed = 0xff;
    }
    if ((!F && B) || (F == B && L && !R)) {
        leftWheelVelocity = exponential_steering(leftWheelVelocity, -leftTargetSpeed);
    } else {
        leftWheelVelocity = exponential_steering(leftWheelVelocity, leftTargetSpeed);
    }
}

// Calculate right wheel velocity
void right_velocity() {
    int rightTargetSpeed;
    if (F == B && L == R) {
        rightTargetSpeed = 0;
    } else if (F != B && !L && R) {
        rightTargetSpeed = TURNING_FACTOR * 0xff;
    } else {
        rightTargetSpeed = 0xff;
    }
    if ((!F && B) || (F == B && !L && R)) {
        rightWheelVelocity = exponential_steering(rightWheelVelocity, -rightTargetSpeed);
    } else {
        rightWheelVelocity = exponential_steering(rightWheelVelocity, rightTargetSpeed);
    }
}

bool connectToWireless() {
    // Error out if no WiFi Shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi module not present");
        return false;
    }
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        pixels.setPixelColor(0, pixels.Color(0, 0, 255));
        pixels.show();
        WiFi.begin(ssid, pass);
        delay(5000);
    }
    Serial.println("Connected to wifi");
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
    printWiFiStatus();
    Udp.begin(localPort);
    broadcastIP();
    return true;
}

void setup() {
    samsam.fromString(server);

    // Initialise serial output
    Serial.begin(9600);
    // Initialise NeoPixel
    pixels.begin();
    pixels.setBrightness(255);

    // Sensors
    pinMode(radioPin, INPUT_PULLDOWN);
    pinMode(irPin, INPUT_PULLDOWN);
    pinMode(magneticPin, INPUT_PULLDOWN);
    // pinMode(2, OUTPUT);  // Controls radio frequency tuning
    //  Motors
    pinMode(9, OUTPUT);  // Left PWM
    pinMode(8, OUTPUT);  // Left DIR
    pinMode(6, OUTPUT);  // Right PWM
    pinMode(3, OUTPUT);  // Right DIR

    if (!connectToWireless()) {
        // Error out if wireless cannot be connected on boot
        while (true) {
            pixels.setPixelColor(0, pixels.Color(255, 0, 0));
            pixels.show();
            while (true) {
                pixels.setBrightness(0);
                pixels.show();
                delay(500);
                pixels.setBrightness(255);
                pixels.show();
                delay(500);
            }
        }
    }
}

void broadcastIP() {
    Udp.beginPacket("255.255.255.255", localPort);
    Udp.write("ZDB");
    IPAddress ip = WiFi.localIP();
    ip.printTo(Udp);
    Udp.endPacket();
}

void processPacket(bool sensing) {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        // Read incoming control packet into buffer.
        int len = Udp.read(packetBuffer, 255);
        if (len > 0) {
            packetBuffer[len] = 0;
        }
        if (packetBuffer[0] == 'Z' && packetBuffer[1] == 'D' && packetBuffer[2] == 'B') {
            if (packetBuffer[3] == '1' && !sensing) {
                CaptureSensorData();
                delay(50);
                SendSensorData();
                delay(100);
            }
            boosting = packetBuffer[8] == '1';
            F = packetBuffer[4] == '1';
            B = packetBuffer[5] == '1';
            L = packetBuffer[6] == '1';
            R = packetBuffer[7] == '1';
        }
    }
    Udp.flush();
}

void loop() {
    // Update delta
    delta = millis() - prevTime;
    prevTime = millis();

    // Check connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("CONNECTION LOST!!");
        boosting = F = B = L = R = false;  // Make sure rover comes to a stop when it loses connection
        WiFi.end();
        connectToWireless();
    }

    processPacket(false);
    move();
}

void move() {
    if (boosting) {  // Note that boosting overrides all other inputs
        boost();
    } else {
        sweepTime = 0;
        left_velocity();
        right_velocity();
    }

    // PWM outputs
    analogWrite(9, abs(leftWheelVelocity));
    analogWrite(6, abs(rightWheelVelocity));

    // DIR outputs
    digitalWrite(8, leftWheelVelocity >= 0 ? LOW : HIGH);
    digitalWrite(3, rightWheelVelocity >= 0 ? LOW : HIGH);
}

void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

double frequencyDetector(byte pin, int lowerThreshold, int upperThreshold, int sampleTime = 50, bool radio = false) {
    long t = millis();
    bool rising = false;
    int i = 0;
    double freq = 0;
    long signalTime;
    bool trig = true;
    do {
        int sample = analogRead(pin);
        if (sample > upperThreshold && rising) {
            if (radio) {
                delay(1); // Skip past initial noise spike with 1ms delay
            }
            if (trig) {
                trig = false;
            } else {
                long d = micros() - signalTime + 43.7;  // Delay compensation calculated at 500Hz reference frequency
                i++;
                freq += 1.0 / (d)*1000000;
            }
            signalTime = micros();
            rising = false;
        }
        if (sample < lowerThreshold && !rising) {
            rising = true;
        }
        processPacket(true);  // Process incoming packets while waiting for signal change
        move();               // Ensure movement is unaffected while sensing
    } while (millis() - t < sampleTime);
    return freq / i;
}

int voltToADC(float val) {
    int maxVal;
    switch (ADC->CTRLB.bit.RESSEL) {
        case 0x0:
            maxVal = 4095;
            break;
        case 0x1:
            maxVal = 65535;
            break;
        case 0x2:
            maxVal = 1023;
            break;
        case 0x3:
            maxVal = 255;
            break;
        default:
            maxVal = 1023;
    }
    return (val / 3.3) * maxVal;
}

void CaptureSensorData() {
    pixels.setPixelColor(0, pixels.Color(255, 255, 0));
    pixels.show();
    // Radio sensor
    SensorData["radio61k"] = frequencyDetector(radioPin, voltToADC(1.8), voltToADC(3.2), 100, true);  // 100ms will collect 15 samples at 151Hz
    // IR sensor
    SensorData["infrared"] = frequencyDetector(irPin, 800, 900, 40, false);  // 40ms sample time will collect 15 samples at 353Hz
    // Hall sensor
    SensorData["magnetic"] = analogRead(magneticPin);
}

void SendSensorData() {
    //Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.beginPacket(samsam, 2390);
    serializeJson(SensorData, Udp);
    // serializeJson(SensorData,Serial);
    Udp.endPacket();
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
}