void setup() {
  Serial.begin(115200); // Inicializamos la comunicación serial a 9600 baudios
}

void loop() {
  int sensorValue = random(0, 101); // Generar un valor aleatorio entre 0 y 100
  Serial.println(sensorValue);     // Enviar el valor por el puerto serial
  delay(1000);                     // Esperar 1 segundo
}
