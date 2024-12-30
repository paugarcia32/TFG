import os
import pandas as pd
import serial
import csv
import time
from threading import Thread

# Configuración de los puertos seriales
TX_PORT = 'COM3'  # Cambia esto por el puerto del transmisor
RX_PORT = 'COM6'  # Cambia esto por el puerto del receptor
BAUD_RATE = 115200

# Archivo CSV de salida
CSV_FILE = 'communication_log_SF8.csv'

if not os.path.exists(CSV_FILE):
    print(f"El archivo {CSV_FILE} no existe. Creando uno vacío...")
    df = pd.DataFrame(columns=['Timestamp', 'Source', 'Message'])
    df.to_csv(CSV_FILE, index=False)

# Función para leer desde un puerto serial y guardar en CSV
def read_serial(port, label):
    with serial.Serial(port, BAUD_RATE, timeout=1) as ser:
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    timestamp = time.time()
                    with open(CSV_FILE, mode='a', newline='') as file:
                        writer = csv.writer(file)
                        writer.writerow([timestamp, label, line])
                    print(f"[{label}] {line} @ {timestamp}")
            except Exception as e:
                print(f"Error en {label}: {e}")
                break

# Crear el archivo CSV y escribir los encabezados
with open(CSV_FILE, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["Timestamp", "Source", "Message"])

# Crear hilos para leer ambos puertos seriales
tx_thread = Thread(target=read_serial, args=(TX_PORT, 'TX'))
rx_thread = Thread(target=read_serial, args=(RX_PORT, 'RX'))

# Iniciar los hilos
tx_thread.start()
rx_thread.start()

# Esperar a que los hilos terminen (se pueden detener manualmente)
tx_thread.join()
rx_thread.join()
