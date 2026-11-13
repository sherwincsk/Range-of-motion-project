


#include "LSM6DS3.h"
#include "Wire.h"

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
float aX;
float aZ;
float gyroX;
float gyroZ;
float Xangle;
float Zangle;
float old_X;
float old_Z;
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
    aX = myIMU.readFloatAccelX();
    aZ = myIMU.readFloatAccelZ();
    //Gyroscope
    Serial.print("\nGyroscope:\n");
    Serial.print(" X1 = ");
    gyroX = myIMU.readFloatGyroX();
    gyroZ = myIMU.readFloatGyroZ();
    Serial.print(" Z1 = ");
    Serial.println(myIMU.readFloatGyroZ(), 4);
    Xangle += (old_X + gyroX)/2 * 0.01;
    old_X= gyroX;
    Zangle += (old_Z + gyroZ)/2 * 0.01;
    old_Z= gyroZ;
    Serial.print("Pitch angle: ");
    Serial.println(Xangle);
    Serial.print("Yaw angle: ");
    Serial.println(Zangle);
    delay(10);
}
