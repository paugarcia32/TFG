import serial
import csv
import time

RX_PORT = 'COM6'
BAUD_RATE = 115200

CSV_FILE = 'cad_operational_data.csv'

with open(CSV_FILE, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["Timestamp", "Event", "Value"])

try:
    with serial.Serial(RX_PORT, BAUD_RATE, timeout=1) as ser:
        print(f"Escuchando en el puerto {RX_PORT}...")
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                timestamp = time.time()

                if "[TIMESTAMP] Recibido en:" in line:
                    value = line.split(":")[-1].strip()
                    event = "Mensaje recibido"
                elif "[TIMESTAMP] CAD operativo en:" in line:
                    value = line.split(":")[-1].strip()
                    event = "CAD operativo"
                else:
                    continue

                with open(CSV_FILE, mode='a', newline='') as file:
                    writer = csv.writer(file)
                    writer.writerow([timestamp, event, value])

                print(f"[{event}] {value} registrado en {timestamp}")
except KeyboardInterrupt:
    print("Detenido por el usuario.")
except Exception as e:
    print(f"Error: {e}")
