void setup()
{
  Serial.begin(115200);
}

void loop()
{
  int sensorValue = random(0, 101);
  Serial.println(sensorValue);
  delay(1000);
}
