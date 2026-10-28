int threshold = 100;
const int LED = 5;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int light = analogRead(A0);
  Serial.println(light);
  int rotaryAngle = analogRead(A1);
  int brightness = map(rotaryAngle, 0, 1023, 0,255);
  if (light < threshold){
      analogWrite(LED,brightness);
  }
}
