#include <Servo.h>
#include <SoftwareSerial.h>

#define RX_PIN 12
#define TX_PIN 13
const uint8_t DataLenght = 16;

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

float triggerState = 90000;
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

  Serial.begin(19200);  

  //Esp 32 data communication (UART)
  espSerial.begin(115200); 

  pinMode(motorPin, OUTPUT);





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
  triggerState = 0;
}

void tick(){


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
  triggerState = min(90000, triggerState + (0.1 * dt));
  if (triggerState < 45000){
    if(triggerState > 200){
      triggerServo.write (5);
    }
    
    digitalWrite(motorPin, HIGH);
  } else {
    if (triggerState > 80000 && triggerState < 89000){
      triggerServo.write (5);
    } else {
      triggerServo.write(180);
    }
    digitalWrite(motorPin, LOW);
  }
}

uint8_t receiveBuffer[(DataLenght + 3) * 2]; 
uint8_t bufferPacket[(DataLenght + 3)]; 
uint8_t packet[(DataLenght + 3)]; 
uint8_t bufferIndex = 0;
bool bufferFilled = false;

void handleSerial(){
  // Read available bytes without blocking
  bool updated = false;
  while (espSerial.available() > 0) {
    updated = true;

    if(bufferIndex == ((DataLenght + 3) * 2)){
      bufferFilled = true;
    }

    bufferIndex = bufferIndex % ((DataLenght + 3) * 2);
    
    uint8_t byte = espSerial.read();
    receiveBuffer[bufferIndex] = byte;

    bufferIndex++;
  }

  
  if (updated && bufferFilled){
    uint8_t lastByte = receiveBuffer[(bufferIndex) % ((DataLenght + 3) * 2)];
    bool foundHeader = false;
    uint8_t packetCounter = 0;



    for(int i = 0; i < (DataLenght + 3) * 2; i++){
      int mi = (i + bufferIndex + 1) % ((DataLenght + 3) * 2);
      uint8_t byte = receiveBuffer[mi];

      if (foundHeader && packetCounter < (DataLenght + 3)){
        bufferPacket[packetCounter] = byte;
        
        packetCounter++;
      }

      if (byte == 255 && lastByte == 255 && !foundHeader){
        foundHeader =  true;

        bufferPacket[0] = lastByte;
        bufferPacket[1] = byte;

        packetCounter = 2;
      }

      lastByte = byte;
    }
  

    uint8_t sum = 0;
    for(int i = 2; i < DataLenght + 2; i++){
      uint8_t byte = bufferPacket[i];
      sum += byte;
    }

    if (sum == bufferPacket[DataLenght + 2]){
      for(int i = 0; i < DataLenght + 3; i++){
        packet[i] = bufferPacket[i];
      }
    } else {
      Serial.print("sum: ");
      Serial.print(sum);
      Serial.print(" expected: ");
      Serial.print(bufferPacket[DataLenght + 2]);
    }
  }
}


void printPacket(){
  Serial.println("----- Packet ------");

  for(int i = 0; i < DataLenght + 3; i++){

    if(i < 2){
      Serial.print("Header ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(packet[i]);
    } else if(i < DataLenght + 2) {
      Serial.print("Byte ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(packet[i]);
    } else {
      Serial.print("Sum: ");
      Serial.println(packet[i]);
    }

  }

}



void loop() {
  
  handleSerial();
  
  for (int i = 0; i < 5; i++){

    delay(2);


    tick();
  }

  if (millis() % 600 < 11){
    printPacket();
  }
}


  





