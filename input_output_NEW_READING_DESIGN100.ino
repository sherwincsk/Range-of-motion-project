#include "LSM6DS3.h"
#include "Wire.h"
#include <cmath>

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
float aY;
float aZ;
float gyroY;
float gyroZ;
float gyro_Yangle;
float gyro_Zangle;
float old_Y;
float old_Z;
float accel_Yangle;
float final_Yangle, final_Zangle;
float t0 {0};
float dt;
float mag_x, mag_y, mag_z, mag_zangle;
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
	dt = (micros() - t0)/1000000;
    gyro_Yangle += (old_Y + gyroY)*dt/2;
    old_Y = gyroY;
    gyro_Zangle += (old_Z + gyroZ)*dt/2;
	t0 = micros();
    old_Z = gyroZ;
    if(abs(aY)>=1){
      aY= 0.99999;
    }
    accelPitch = (acos(aY)*180/3.141-90);
	mag_Zangle = atan2(mag_y, mag_x)*180/3.141;
    final_Yangle = gyro_Yangle*0.80 + accelPitch*0.20;
	final_Zangle = gyro_Zangle*0.80 + mag_Zangle*0.20;
    Serial.println(final_Yangle);
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
