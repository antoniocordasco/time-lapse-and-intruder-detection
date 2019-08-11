
#define sensorPin 8
#define ledPin 7

String serialInput;

void setup() {
  // Define inputs and outputs:
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  
  //Begin Serial communication at a baudrate of 9600:
  Serial.begin(9600);
}
void loop() {
  digitalWrite(ledPin, LOW);

  
  int sensorValue = digitalRead(sensorPin);
  if (sensorValue == 1) {
    Serial.write("intruder");    
    digitalWrite(ledPin, HIGH); // The Relay Input works Inversly
  }

  serialInput = Serial.readString(); //Read the serial data and store in var
  Serial.println(serialInput);    
  
  delay(1000);
}
