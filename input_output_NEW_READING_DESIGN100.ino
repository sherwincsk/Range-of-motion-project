#include "LSM6DS3.h"
#include "Wire.h"
#include <cmath>

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
float aY;
float aZ;
float gyroY;
float gyroZ;
float Yangle;
float Zangle;
float old_Y;
float old_Z;
float accelPitch;
float finalAnswer;
void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    while (!Serial);
    //Call .begin() to configure the IMUs
    if (myIMU.begin() != 0) {
        Serial.println("Device error");
    } else {
        Serial.println("Device OK!");
    }
}

void loop() {
    //Accelerometer
    aY = myIMU.readFloatAccelX();
    aZ = myIMU.readFloatAccelZ();
    //Gyroscope
    gyroY = myIMU.readFloatGyroY();
    gyroZ = myIMU.readFloatGyroZ();
    Yangle += (old_Y + gyroY)*0.1/2;
    old_Y = gyroY;
    Zangle += (old_Z + gyroZ)*0.01/2;
    old_Z= gyroZ;
    if(abs(aY)>=1){
      aY= 0.99999;
    }
    accelPitch = (acos(aY)*180/3.141-90);
    finalAnswer = Yangle*0.20 + accelPitch*0.80;
    Serial.println(finalAnswer);
    delay(100);
}

/*
float Y = (acos(aY)*180/3.141)-90;
    if(aY==0){
      Y=0;
    }
    float Y2 = Yangle*0.8 + Y*0.2;
   if (millis()%20==0){
        
    Serial.println(Y2);
    }
    float MIL = millis();
    delay(10-MIL);*/
