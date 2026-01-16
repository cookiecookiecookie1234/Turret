#include <Servo.h>

Servo triggerServo;

const int xyStepper[4] = {2, 3, 4, 5};
const int yzStepper[4] = {6, 7, 8, 9};
int motorPin = 10;

float added = 0;

const int stepPattern[4][4] = {
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

void setup(){
  triggerServo.attach(11);

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



void shoot(){
  triggerState = 500;
}

void tick(float dt){
  float st1 = (xyStepperCounter /2048) * 360;
  float st2 = (yzStepperCounter /2048) * 360;

  float st1E = fmod(xyTarRot, 360.0f) - fmod(st1, 360.0f);
  float st2E = fmod(yzTarRot, 360.0f) - fmod(st2, 360.0f);

  if (st1E > 0.5){
    if (st1E < 180){
      xyStepperCounter += dt * 0.5;
    } else {
      xyStepperCounter -= dt * 0.5;
    }
  }
  
  if (st1E < -0.5){
    if (st1E < 180){
      xyStepperCounter -= dt;
    } else {
      xyStepperCounter += dt;
    }
  }

  if (st2E > 0.5){
    if (st2E < 180){
      yzStepperCounter += dt * 0.3;
    } else {
      yzStepperCounter -= dt * 0.3;
    }
  }
  
  if (st2E < -0.5){
    if (st2E < 180){
      yzStepperCounter -= dt * 0.3;
    } else {
      yzStepperCounter += dt * 0.3;
    }
  }

  



  triggerState = max(0, triggerState - dt);
  if (triggerState != 0){
    triggerServo.write (5);
  } else {
    triggerServo.write(180);
  }

  updStepper(xyStepper, xyStepperCounter);
  updStepper(yzStepper, yzStepperCounter);
}
void loop() {
  delay(1);
  tick(1);

  
  xyTarRot = sin(millis() * 0.0025) * 20;
  yzTarRot = sin(millis() * 0.0025) * 20;

  if (millis() % 200 < 5){
    if (added == 1){
      
    }



    added = 0;
  } else {
    added = 1;
  }


  if (millis() % 3000 < 5){
    shoot();
  }



}
