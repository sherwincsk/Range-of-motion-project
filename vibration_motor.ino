const int motorPin = 1;
const unsigned long testDuration = 10000;
bool buzz = false;
unsigned long startTime;


void setup() {

pinMode(motorPin, OUTPUT);
digitalWrite(motorPin, LOW);
bool buzz = false;
startTime = millis();
}

void loop() {
 
if( millis() - startTime >= testDuration && buzz !=true){
buzz = true;
digitalWrite(motorPin, HIGH);
delay(500);
digitalWrite(motorPin, LOW);
}
}

