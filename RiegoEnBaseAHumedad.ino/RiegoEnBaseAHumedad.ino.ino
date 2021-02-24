
int humValue = 0;
int humidity;
int solePin = 7;
int sensorhumidity = A1;

void setup() {
  pinMode(solePin, OUTPUT);
  Serial.begin(9600); 
}

void loop() {

  humValue = analogRead(sensorhumidity);
  
  delay(1500);

  humidity = (100.0 * humValue) / 1024;
  Serial.print("humidity:");
  Serial.println(humidity);
  Serial.println("=======================================================================");
  
  // ----------------------------------------------------
  // Chequeo si debo regar
  // ----------------------------------------------------
  
  if( humidity > 70 ) {
    
    Serial.println("Humedad baja, se activa manguera de riego.");
    digitalWrite(solePin, HIGH);
    
  } else {
    digitalWrite(solePin, LOW);
  }
  
  Serial.println("=======================================================================");

}
