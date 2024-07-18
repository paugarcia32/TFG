

// Definición de los manejadores de tareas
TaskHandle_t Task1;
TaskHandle_t Task2;

void setup() {
  // Inicializa la comunicación serie
  Serial.begin(115200);

  // Crear la tarea que se ejecutará en la función Task1code() con prioridad 1 y
  // en el núcleo 0
  xTaskCreatePinnedToCore(
      Task1code, /* Función de la tarea */
      "Task1",   /* Nombre de la tarea */
      10000,     /* Tamaño de la pila de la tarea */
      NULL,      /* Parámetro de la tarea */
      1,         /* Prioridad de la tarea */
      &Task1,    /* Manejador de la tarea para llevar un seguimiento */
      0);        /* Fijar la tarea al núcleo 0 */
  delay(500);

  // Crear la tarea que se ejecutará en la función Task2code() con prioridad 1 y
  // en el núcleo 1
  xTaskCreatePinnedToCore(
      Task2code, /* Función de la tarea */
      "Task2",   /* Nombre de la tarea */
      10000,     /* Tamaño de la pila de la tarea */
      NULL,      /* Parámetro de la tarea */
      1,         /* Prioridad de la tarea */
      &Task2,    /* Manejador de la tarea para llevar un seguimiento */
      1);        /* Fijar la tarea al núcleo 1 */
  delay(500);
}

// Task1code: Imprime un mensaje en el Serial Monitor cada 1000 ms
void Task1code(void *pvParameters) {
  Serial.print("Task1 corriendo en el núcleo ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    Serial.println("Task1 ejecutándose");
    delay(1000);
  }
}

// Task2code: Imprime un mensaje en el Serial Monitor cada 700 ms
void Task2code(void *pvParameters) {
  Serial.print("Task2 corriendo en el núcleo ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    Serial.println("Task2 ejecutándose");
    delay(700);
  }
}

void loop() {
  // No hay necesidad de código en el loop() ya que las tareas se están
  // ejecutando en paralelo
}
