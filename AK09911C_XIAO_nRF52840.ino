/*
  AK09911C 3-axis magnetometer  ->  Seeed Studio XIAO nRF52840 Sense
  ====================================================================
  Reads the magnetic field on the X, Y and Z axes over I2C and prints it in
  microtesla (uT). Open the Serial Monitor or Serial Plotter at 115200 baud.

  Board (Tools > Board): "Seeed XIAO nRF52840 Sense"
    - works with both "Seeed nRF52 Boards" and "Seeed nRF52 mbed-enabled Boards"
  Libraries: only Wire (built in) - nothing to install.

  WIRING  (AK09911C breakout pin  ->  XIAO pin)
    VDD   -> 3V3
    VID   -> 3V3   I/O supply - tie it to VDD
    GND   -> GND
    SDA   -> D4    (SDA)
    SCL   -> D5    (SCL)
    RSTN  -> 3V3   reset, active LOW. If it is left floating the chip can
                   stay in reset and never answer.
    CAD   -> GND   I2C address 0x0C (CAD -> 3V3 gives 0x0D; both are found)
    TST   -> leave unconnected
  * Power it from 3V3: the AK09911C runs on 2.4-3.6 V. Don't use the 5V pin.
  * If your breakout has no pull-up resistors on SDA/SCL, add 4.7 kOhm from
    SDA to 3V3 and from SCL to 3V3.

  Register map, timing and formulas follow the AKM AK09911 datasheet
  (MS1526-E-01).
*/

#include <Wire.h>
#include <math.h>

// ================================ Settings =================================
const int  SAMPLE_RATE_HZ = 10;     // 10, 20, 50 or 100 measurements per second
const bool PRINT_RAW      = false;  // true = also print the raw sensor counts
const int  RSTN_PIN       = -1;     // if RSTN goes to a XIAO pin instead of 3V3,
                                    // put that pin number here (e.g. 3 for D3)

// ========================= AK09911 registers & values ======================
const uint8_t AK_ADDR_CAD_GND = 0x0C;  // CAD pin tied to GND
const uint8_t AK_ADDR_CAD_VDD = 0x0D;  // CAD pin tied to 3V3

const uint8_t AK_REG_WIA1  = 0x00;  // company ID, always 0x48
const uint8_t AK_REG_WIA2  = 0x01;  // device ID, 0x05 = AK09911
const uint8_t AK_REG_ST1   = 0x10;  // status 1: bit0 DRDY, bit1 DOR
                                    // 0x11..0x16: HXL HXH HYL HYH HZL HZH
                                    // 0x17: TMPS (dummy)
const uint8_t AK_REG_ST2   = 0x18;  // status 2: bit3 HOFL (overflow)
const uint8_t AK_REG_CNTL2 = 0x31;  // operating mode
const uint8_t AK_REG_CNTL3 = 0x32;  // bit0 SRST = soft reset
const uint8_t AK_REG_ASAX  = 0x60;  // ASAX, ASAY, ASAZ: factory sensitivity trim

const uint8_t AK_WIA1_VALUE = 0x48;
const uint8_t AK_WIA2_VALUE = 0x05;

const uint8_t AK_ST1_DRDY = 0x01;   // a new sample is ready
const uint8_t AK_ST1_DOR  = 0x02;   // an older sample was skipped (not read in time)
const uint8_t AK_ST2_HOFL = 0x08;   // magnetic overflow - this sample is not valid

const uint8_t AK_MODE_POWER_DOWN = 0x00;
const uint8_t AK_MODE_CONT_10HZ  = 0x02;
const uint8_t AK_MODE_CONT_20HZ  = 0x04;
const uint8_t AK_MODE_CONT_50HZ  = 0x06;
const uint8_t AK_MODE_CONT_100HZ = 0x08;
const uint8_t AK_MODE_SELF_TEST  = 0x10;
const uint8_t AK_MODE_FUSE_ROM   = 0x1F;


const float AK_UT_PER_LSB = 0.6f;   // 0.6 uT per count (after the factory trim)

// One measurement from the sensor
struct MagReading {
  int16_t raw[3];    // raw counts straight from the chip: X, Y, Z
  float   uT[3];     // magnetic field in microtesla:      X, Y, Z
  bool    overflow;  // true = field too strong, sample is not valid
  bool    missed;    // true = an older sample was overwritten before we read it
};

// ================================ Globals ==================================
uint8_t  akAddress    = 0;                    // I2C address the sensor answered on
float    asaFactor[3] = {1.0f, 1.0f, 1.0f};   // per-axis factory trim
bool     sensorReady  = false;
uint32_t lastDataMs   = 0;   // when we last got a sample
uint32_t lastPollMs   = 0;   // when we last asked the sensor for new data
uint32_t lastRetryMs  = 0;   // when we last tried to find the sensor
float start_X, start_Y;
bool calibrated = false;

// ============================ Low-level I2C ================================
// Write one register. Returns true if the sensor acknowledged.
bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(akAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Read 'count' registers starting at 'reg' (the chip auto-increments the address).
bool readRegisters(uint8_t reg, uint8_t *buffer, uint8_t count) {
  Wire.beginTransmission(akAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {   // false = repeated START (no STOP)
    return false;
  }
  if (Wire.requestFrom(akAddress, (size_t)count) != count) {
    return false;
  }
  for (uint8_t i = 0; i < count; i++) {
    buffer[i] = (uint8_t)Wire.read();
  }
  return true;
}

void printHex2(uint8_t value) {
  Serial.print("0x");
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

// ============================ AK09911 driver ===============================
// Datasheet rule: always go to Power-down first and wait at least 100 us
// before switching to another mode.
bool setMode(uint8_t mode) {
  if (!writeRegister(AK_REG_CNTL2, AK_MODE_POWER_DOWN)) {
    return false;
  }
  delayMicroseconds(200);
  if (mode == AK_MODE_POWER_DOWN) {
    return true;
  }
  if (!writeRegister(AK_REG_CNTL2, mode)) {
    return false;
  }
  delayMicroseconds(200);
  return true;
}

uint8_t modeForRate(int hz) {
  switch (hz) {
    case 20:  return AK_MODE_CONT_20HZ;
    case 50:  return AK_MODE_CONT_50HZ;
    case 100: return AK_MODE_CONT_100HZ;
    default:  return AK_MODE_CONT_10HZ;
  }
}

// Look for the AK09911C at both possible addresses and check its ID bytes.
bool findSensor() {
  const uint8_t addresses[2] = {AK_ADDR_CAD_GND, AK_ADDR_CAD_VDD};
  for (uint8_t i = 0; i < 2; i++) {
    akAddress = addresses[i];
    uint8_t id[2];
    if (!readRegisters(AK_REG_WIA1, id, 2)) {
      continue;  // nothing answered at this address
    }
    if (id[0] == AK_WIA1_VALUE && id[1] == AK_WIA2_VALUE) {
      return true;
    }
    Serial.print("A device answered at ");
    printHex2(akAddress);
    Serial.print(" but its ID is ");
    printHex2(id[0]);
    Serial.print(' ');
    printHex2(id[1]);
    Serial.println(" (an AK09911 reads 0x48 0x05).");
  }
  akAddress = 0;
  return false;
}

// Read the factory sensitivity trim (ASA) from the chip's Fuse ROM.
// Datasheet 8.3.11:  Hadj = H * (ASA / 128 + 1)
bool readSensitivityTrim() {
  uint8_t asa[3];
  if (!setMode(AK_MODE_FUSE_ROM)) {
    return false;
  }
  bool ok = readRegisters(AK_REG_ASAX, asa, 3);
  if (!setMode(AK_MODE_POWER_DOWN) || !ok) {  // leave Fuse ROM mode via Power-down
    return false;
  }
  for (uint8_t axis = 0; axis < 3; axis++) {
    asaFactor[axis] = asa[axis] / 128.0f + 1.0f;
  }
  Serial.print("Factory trim ASA X/Y/Z = ");
  for (uint8_t axis = 0; axis < 3; axis++) {
    printHex2(asa[axis]);
    Serial.print(axis < 2 ? " / " : "  ->  gain x");
  }
  Serial.print(asaFactor[0], 3);
  Serial.print(" / x");
  Serial.print(asaFactor[1], 3);
  Serial.print(" / x");
  Serial.println(asaFactor[2], 3);
  return true;
}

// Wait until ST1 says a new sample is ready (or give up after timeoutMs).
bool waitForDataReady(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    uint8_t st1;
    if (readRegisters(AK_REG_ST1, &st1, 1) && (st1 & AK_ST1_DRDY)) {
      return true;
    }
    delay(1);
  }
  return false;
}

// Read one complete sample: ST1, X, Y, Z, TMPS and ST2 (registers 0x10..0x18)
// in a single burst. Reading ST2 at the end is REQUIRED: it tells the chip we
// are done. Until ST2 is read the chip keeps the data locked and won't update it.
bool readSample(MagReading &r) {
  uint8_t b[9];
  if (!readRegisters(AK_REG_ST1, b, 9)) {
    return false;
  }
  for (uint8_t axis = 0; axis < 3; axis++) {
    uint8_t low  = b[1 + 2 * axis];
    uint8_t high = b[2 + 2 * axis];
    r.raw[axis] = (int16_t)(((uint16_t)high << 8) | low);  // two's complement, low byte first
    r.uT[axis]  = r.raw[axis] * asaFactor[axis] * AK_UT_PER_LSB;
  }
  r.missed   = (b[0] & AK_ST1_DOR) != 0;
  r.overflow = (b[8] & AK_ST2_HOFL) != 0;
  return true;
}

// Built-in self-test (datasheet 6.4.4): the chip applies a known magnetic field
// to itself. If the result is inside AKM's limits, the sensor is working.
bool runSelfTest() {
  MagReading r;
  if (!setMode(AK_MODE_SELF_TEST) || !waitForDataReady(100) || !readSample(r)) {
    Serial.println("Self-test: no response from the sensor.");
    return false;
  }
  // The chip returns to Power-down by itself after the self-test.
  // AKM's limits are in trimmed counts (1 count = 0.6 uT).
  float hx = r.raw[0] * asaFactor[0];
  float hy = r.raw[1] * asaFactor[1];
  float hz = r.raw[2] * asaFactor[2];
  bool pass = !r.overflow &&
              hx >= -30.0f && hx <= 30.0f &&
              hy >= -30.0f && hy <= 30.0f &&
              hz >= -400.0f && hz <= -50.0f;
  Serial.print("Self-test (counts): X=");
  Serial.print(hx, 1);
  Serial.print(" Y=");
  Serial.print(hy, 1);
  Serial.print(" Z=");
  Serial.print(hz, 1);
  Serial.print("  [limits X,Y: -30..30, Z: -400..-50]  ->  ");
  Serial.println(pass ? "PASS" : "FAIL");
  if (!pass) {
    Serial.println("  Keep magnets, motors and steel away from the sensor, check it");
    Serial.println("  has a clean 3.3 V supply, then press RESET to run the test again.");
  }
  return pass;
}

// Full start-up sequence. Returns true once the sensor is measuring.
bool initSensor() {
  sensorReady = false;
  if (!findSensor()) {
    return false;
  }
  Serial.print("AK09911C found at I2C address ");
  printHex2(akAddress);
  Serial.println();

  // Soft reset: every register back to default, chip goes to Power-down.
  if (!writeRegister(AK_REG_CNTL3, 0x01)) {
    return false;
  }
  delay(10);

  if (!readSensitivityTrim()) {
    Serial.println("Could not read the factory trim (Fuse ROM).");
    return false;
  }
  runSelfTest();

  if (!setMode(modeForRate(SAMPLE_RATE_HZ))) {
    return false;
  }
  sensorReady = true;
  lastDataMs  = millis();
  Serial.println("Measuring... (X, Y, Z in microtesla; Total = field strength)");
  return true;
}

// Non-blocking: returns true only when a NEW sample was read into 'r'.
bool readMagnetometer(MagReading &r) {
  uint8_t st1;
  if (!readRegisters(AK_REG_ST1, &st1, 1)) {
    return false;
  }
  if ((st1 & AK_ST1_DRDY) == 0) {
    return false;  // no new sample yet
  }
  return readSample(r);
}

void calibration (const MagReading m){
  start_X = m.uT[0];
  start_Y = m.uT[1];
}
void printReading(const MagReading &m, float start_X, float start_Y) {
  if (m.overflow) {
    Serial.println("Magnetic overflow (strong magnet nearby?) - sample skipped");
    return;
  }
  float total = sqrtf(m.uT[0] * m.uT[0] + m.uT[1] * m.uT[1] + m.uT[2] * m.uT[2]);
  Serial.print("X_uT:");
  Serial.print(m.uT[0]-start_X, 2);
  Serial.print(",Y_uT:");
  Serial.print(m.uT[1]-start_Y, 2);
  Serial.print(",Z_uT:");
  Serial.print(m.uT[2], 2);
  Serial.print(",Total_uT:");
  Serial.print(total, 2);
  if (PRINT_RAW) {
    Serial.print(",X_raw:");
    Serial.print(m.raw[0]-start_X);
    Serial.print(",Y_raw:");
    Serial.print(m.raw[1]-start_Y);
    Serial.print(",Z_raw:");
    Serial.print(m.raw[2]);
  }
  Serial.println();
}

void printTroubleshooting() {
  Serial.println("AK09911C NOT found at 0x0C or 0x0D. Check:");
  Serial.println("  1. SDA -> D4 and SCL -> D5 (not swapped)");
  Serial.println("  2. VDD and VID -> 3V3, GND -> GND");
  Serial.println("  3. RSTN -> 3V3 (a floating RSTN holds the chip in reset)");
  Serial.println("  4. 4.7k pull-ups on SDA and SCL if your breakout has none");
  Serial.println("Retrying every 2 seconds...");
}

// ============================ Arduino entry points =========================
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);  // wait for the Serial Monitor (remove this loop to run on battery)
  }
  Serial.println();
  Serial.println("AK09911C magnetometer on XIAO nRF52840 Sense");

  if (RSTN_PIN >= 0) {  // optional hardware reset pulse (needs >= 5 us LOW)
    pinMode(RSTN_PIN, OUTPUT);
    digitalWrite(RSTN_PIN, LOW);
    delay(1);
    digitalWrite(RSTN_PIN, HIGH);
    delay(10);
  }

  Wire.begin();  // XIAO nRF52840: SDA = D4, SCL = D5, 100 kHz

  if (!initSensor()) {
    printTroubleshooting();
  }
  lastRetryMs = millis();
  
}

void loop() {
  uint32_t now = millis();

  // Sensor missing or lost: try to (re)connect every 2 seconds.
  if (!sensorReady) {
    if (now - lastRetryMs >= 2000) {
      lastRetryMs = now;
      if (!initSensor()) {
        Serial.println("AK09911C not found - retrying...");
      }
    }
    return;
  }

  // Ask for new data every few ms without blocking (keeps the I2C bus quiet).
  if (now - lastPollMs < 5) {
    return;
  }
  lastPollMs = now;

  MagReading m;
  if (readMagnetometer(m)) {
    lastDataMs = now;

    if (!calibrated && !m.overflow) {
        calibration(m);
        calibrated = true;
        Serial.println("Calibration complete!");
    }

    if (calibrated) {
        printReading(m, start_X, start_Y);
    }
  } else if (now - lastDataMs > 1000) {
    Serial.println("No data from the AK09911C for 1 s - re-initialising...");
    sensorReady = false;
    lastRetryMs = now - 2000;  // retry straight away
  }
}
