
import serial
import csv
import time

# Configuración del puerto serial
port = "COM4"  # Cambia a tu puerto serial
baudrate = 115200
timeout = 1

# Archivo CSV donde se guardarán los datos
csv_filename = "sensor_data.csv"

try:
    # Abrir conexión serial
    ser = serial.Serial(port, baudrate, timeout=timeout)
    print(f"Conectado al puerto {port}")

    # Crear o abrir el archivo CSV
    with open(csv_filename, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(["Timestamp", "Sensor Value"])  # Cabecera del archivo

        while True:
            # Leer línea desde el puerto serial
            line = ser.readline().decode('utf-8').strip()
            if line:  # Si hay datos
                # Guardar el dato con marca de tiempo en el CSV
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                csv_writer.writerow([timestamp, line])
                print(f"Datos guardados: {timestamp}, {line}")
except KeyboardInterrupt:
    print("\nFinalizando programa...")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Conexión serial cerrada.")
