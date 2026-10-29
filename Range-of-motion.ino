void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  double reading; //calculating angular displacement
  double old_reading;
  angle += (reading + old_reading)/2 * 0.01;
  delay(10);
}
