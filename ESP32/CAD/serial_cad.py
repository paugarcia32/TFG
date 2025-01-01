import os
import pandas as pd
import serial
import csv
import time
from threading import Thread

TX_PORT = 'COM3'
RX_PORT = 'COM7'
BAUD_RATE = 115200

CSV_FILE = 'CAD_error_rate_10s.csv'

if not os.path.exists(CSV_FILE):
    print(f"El archivo {CSV_FILE} no existe. Creando uno vacío...")
    df = pd.DataFrame(columns=['Timestamp', 'Source', 'Message'])
    df.to_csv(CSV_FILE, index=False)

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

with open(CSV_FILE, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["Timestamp", "Source", "Message"])

tx_thread = Thread(target=read_serial, args=(TX_PORT, 'TX'))
rx_thread = Thread(target=read_serial, args=(RX_PORT, 'RX'))

tx_thread.start()
rx_thread.start()

tx_thread.join()
rx_thread.join()
