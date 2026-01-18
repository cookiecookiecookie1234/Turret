#include <Servo.h>
#include <SoftwareSerial.h>

#define RX_PIN 12  // New pin
#define TX_PIN 13  // New pin

SoftwareSerial espSerial(RX_PIN, TX_PIN);

Servo triggerServo;



const int xyStepper[4] = {2, 3, 4, 5};
const int yzStepper[4] = {6, 7, 8, 9};
int motorPin = 10;

float added = 0;

const int stepPattern[4][4] = {
  {1, 0, 0, 0},
  {0, 1, 0, 0},
  {0, 0, 1, 0},
  {0, 0, 0, 1}
};

const int stepPatternStrong[4][4] = {
  {1, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 1},
  {1, 0, 0, 1}
};

float triggerState = 0;
float xyTarRot = 0;
float xyStepperCounter = 0;

float yzRot = 0;
float yzTarRot = 0;
float yzStepperCounter = 0;

float lastMovement = 0;

float lastTick = 0;
int read = 00;



void setup(){
  triggerServo.attach(11);

  Serial.begin(9600);  

  //Esp 32 data communication (UART)
  espSerial.begin(115200); 





  for (int i = 0; i<4; i++){
    pinMode(xyStepper[i], OUTPUT);
  }
  
  for (int i = 0; i<4; i++){
    pinMode(yzStepper[i], OUTPUT);
  }

  //analogWrite(motorPin, 20);  

}

void updStepper(int pins[4], float step){
  int state = int(step) % 4;
  if (state < 0) state += 4;
  
  for (int i = 0; i<4; i++){
    digitalWrite(pins[i], stepPattern[state][i]);
  }
}

void updStepperFast(int pins[4], float step){
  int state = int(step) % 4;
  if (state < 0) state += 4;
  
  for (int i = 0; i<4; i++){
    digitalWrite(pins[i], stepPatternStrong[state][i]);
  }
}

void disableStepper(int pins[4]){
  for (int i = 0; i<4; i++){
    digitalWrite(pins[i], 0);
  }
}

void shoot(){
  triggerState = 30000;
}

void tick(){
  Serial.println(triggerState);


  float dt = (micros() - lastTick) / 1.0;
  lastTick = micros();

  float st1 = (xyStepperCounter /2048) * 360;
  float st2 = (yzStepperCounter /2048) * 360;

  float st1E = fmod(xyTarRot, 360.0f) - fmod(st1, 360.0f);
  float st2E = fmod(yzTarRot, 360.0f) - fmod(st2, 360.0f);


  //Smooth stepper controller
  if (abs(st1E) > 0.5){
    float speed = constrain(st1E * 0.08, -1, 1);
    speed += constrain(st1E * 100.0, -1, 1) * 0.15;
    speed *= 0.03;


    if (abs(st1E) < 180){
      xyStepperCounter += constrain(dt * speed, -1.0, 1.0);
    } else {
      xyStepperCounter -= constrain(dt * speed, -1.0, 1.0);
    }

    if (speed <= 0.1){
      updStepper(xyStepper, xyStepperCounter);
    } else{
      updStepperFast(xyStepper, xyStepperCounter);
    }

    lastMovement = millis();
  } else {
    if (millis() - lastMovement > 150){
      disableStepper(xyStepper);
    } else {
      updStepper(xyStepper, xyStepperCounter);
    }
  }
  
  if (abs(st2E) > 0.5){
    float speed = constrain(st2E * 0.08, -1, 1);
    speed += constrain(st2E * 100.0, -1, 1) * 0.15;
    speed *= 0.04;


    if (abs(st2E) < 180){
      yzStepperCounter += constrain(dt * speed, -1.0, 1.0);
    } else {
      yzStepperCounter -= constrain(dt * speed, -1.0, 1.0);
    }



    if (speed <= 0.1){
      updStepper(yzStepper, yzStepperCounter);
    } else{
      updStepperFast(yzStepper, yzStepperCounter);
    }

    
    lastMovement = millis();
  } else {
    if (millis() - lastMovement > 150){
      disableStepper(yzStepper);
    } else {
      updStepper(yzStepper, yzStepperCounter);
    }
  }
  

  //Asinc servo write
  triggerState = max(0, triggerState - (0.1 * dt));
  if (triggerState != 0){
    triggerServo.write (5);
  } else {
    triggerServo.write(180);
  }
}

uint8_t receiveBuffer[16];  // Fixed from int8_t
uint8_t bufferIndex = 0;

void handleSerial(){
  // Read available bytes without blocking
  while (espSerial.available() > 0 && bufferIndex < 16) {
    receiveBuffer[bufferIndex++] = espSerial.read();
  }
  
  // Process complete message
  if (bufferIndex == 16) {
    int32_t value1, value2, value3, value4;
    
    // Copy bytes to integers
    memcpy(&value1, &receiveBuffer[0], 4);
    memcpy(&value2, &receiveBuffer[4], 4);
    memcpy(&value3, &receiveBuffer[8], 4);
    memcpy(&value4, &receiveBuffer[12], 4);
    
    // Use the values for your turret control
    xyTarRot = value1;  // Set target rotation from ESP32
    yzTarRot = value2;  // Set target rotation from ESP32
    
    // value3 and value4 available for other commands
    // For example: if (value3 == 1) shoot();
    
    // Debug output (optional - remove if not needed)
    Serial.print("Received: ");
    Serial.print(value1); Serial.print(", ");
    Serial.print(value2); Serial.print(", ");
    Serial.print(value3); Serial.print(", ");
    Serial.println(value4);
    
    // Reset for next message
    bufferIndex = 0;
  }
}



void loop() {



  
  handleSerial();
  
  for (int i = 0; i < 5; i++){

  if (millis() % 6000 < 5){
    shoot();
  }


    tick();
  }
}


  





