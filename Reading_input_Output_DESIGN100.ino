// Basic demo for accelerometer readings from Adafruit MPU6050

/* STEP 1: Connect to the sensor
   mpu.begin() opens the I2C connection (the ESP32's SDA/SCL pins) and
   checks that the sensor answers. It returns false if the sensor is not
   found (usually a wiring or address problem). If that happens, the
   program prints an error and stops, since there is nothing to read.

   STEP 2: Accelerometer range
   mpu.setAccelerometerRange(MPU6050_RANGE_8_G)
   Sets the largest acceleration the sensor can measure: +-8 g.
   (1 g = gravity, about 9.8 m/s^2.) Options: 2, 4, 8, or 16 g.
     - Smaller range = finer detail, but readings max out on hard motion.
     - Larger range  = handles sudden/strong motion, but less detail.
   Readings are reported in m/s^2.

   STEP 3: Gyroscope range
   mpu.setGyroRange(MPU6050_RANGE_500_DEG)
   Sets the fastest rotation the sensor can measure: +-500 degrees/second.
   Options: 250, 500, 1000, or 2000 deg/s. Same tradeoff as above:
   small range = more precise for slow movement, large range = handles
   fast spins. Note: the library reports gyro values in rad/s, not deg/s.

   STEP 4: Filter bandwidth
   mpu.setFilterBandwidth(MPU6050_BAND_21_HZ)
   Sets how much the sensor smooths out electrical noise and tiny
   vibrations. Options range from 5 Hz to 260 Hz.
     - Lower Hz  = smoother data, but slower to react to quick changes. 
     - Higher Hz = reacts Faster, but the data is noisier. */
     
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!"); // the stuff below configures the sensor

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}


void loop() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp); // INPUT: these are my variables Sherwin that u need to use, but disergard temp. For some reason i need it there but we dont really need it

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");



  Serial.println("");
  delay(100);
}