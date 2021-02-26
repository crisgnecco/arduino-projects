
// -----------------------------------------------------------------
/*** cada 12hs chequea si la tierra esta humeda y se debe regar ***/
// -----------------------------------------------------------------


int humValue = 0;
int humidity;
int solePin = 7;
int sensorhumidity = A1;
int msecs;

void setup() {
  pinMode(solePin, OUTPUT);
  Serial.begin(9600);

  int hours = 12;
  msecs = hours * 3600000;
}

void loop() {

  humValue = analogRead(sensorhumidity);

  humidity = (100.0 * humValue) / 1024;
  Serial.print("humidity:");
  Serial.println(humidity);
  Serial.println("=======================================================================");

  if ( humidity > 70 ) {

    Serial.println("Humedad baja, se activa manguera de riego.");
    digitalWrite(solePin, HIGH);

    /* espero tiempo de riego, 10 secs */
    delay(10000);
    digitalWrite(solePin, LOW);

  }

  /*** espero n hs  ***/
  delay(msecs);

  Serial.println("=======================================================================");

}
