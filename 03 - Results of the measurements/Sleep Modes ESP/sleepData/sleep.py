
import serial
import csv
import time

port = "COM4"
baudrate = 115200
timeout = 1

csv_filename = "sensor_data.csv"

try:
    ser = serial.Serial(port, baudrate, timeout=timeout)
    print(f"Conectado al puerto {port}")

    with open(csv_filename, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(["Timestamp", "Sensor Value"])

        while True:

            line = ser.readline().decode('utf-8').strip()
            if line:
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
