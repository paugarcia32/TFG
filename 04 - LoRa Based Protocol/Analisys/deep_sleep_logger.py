import serial
import csv
from datetime import datetime

# Configuración del puerto serie
SERIAL_PORT = "COM3"  # Cambia al puerto donde esté conectado tu ESP32
BAUD_RATE = 115200
OUTPUT_CSV = "deep_sleep_log.csv"

def main():
    try:
        # Abrir puerto serie
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Escuchando el puerto serie: {SERIAL_PORT} a {BAUD_RATE} bps")
    except Exception as e:
        print(f"Error abriendo el puerto serie: {e}")
        return

    # Crear/abrir archivo CSV para guardar datos
    with open(OUTPUT_CSV, mode="a", newline='', encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)

        # Escribir encabezados si el archivo está vacío
        if csv_file.tell() == 0:
            writer.writerow(["Timestamp", "Event", "Sleep Duration (ms)"])

        last_sleep_time = None
        print("Capturando datos... Presiona Ctrl+C para detener.")
        try:
            while True:
                # Leer línea desde el puerto serie
                if ser.in_waiting > 0:
                    line = ser.readline().decode(errors='ignore').strip()
                    if line in ["D", "U"]:
                        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

                        if line == "D":
                            # Registrar el inicio del sueño
                            last_sleep_time = datetime.now()
                            writer.writerow([timestamp, "D", None])
                            print(f"[{timestamp}] Nodo entrando a deep sleep (D)")

                        elif line == "U" and last_sleep_time:
                            # Calcular la duración del sueño
                            wake_time = datetime.now()
                            sleep_duration = (wake_time - last_sleep_time).total_seconds() * 1000
                            writer.writerow([timestamp, "U", round(sleep_duration)])
                            print(f"[{timestamp}] Nodo despertando (U) - Tiempo dormido: {round(sleep_duration)} ms")
                            last_sleep_time = None  # Reiniciar para la próxima iteración
                        else:
                            writer.writerow([timestamp, line, None])

        except KeyboardInterrupt:
            print("\nCaptura interrumpida por el usuario (Ctrl+C).")
        finally:
            ser.close()
            print("Puerto serie cerrado.")

if __name__ == "__main__":
    main()
