#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import datetime
import csv
import time

# Configuración de los puertos serial
GATEWAY_SERIAL_PORT = 'COM8'  # Cambiar según corresponda
NODE_SERIAL_PORT = 'COM3'  # Cambiar según corresponda
BAUD_RATE = 115200

# Mensajes que queremos capturar
MESSAGE_TAGS = ["BTX", "BRX", "RTX", "RRX", "STX", "SRX", "DTX", "DRX"]

def main():
    # Intentar abrir los puertos serial
    try:
        ser_gateway = serial.Serial(GATEWAY_SERIAL_PORT, BAUD_RATE, timeout=0.1)
        ser_node = serial.Serial(NODE_SERIAL_PORT, BAUD_RATE, timeout=0.1)
    except Exception as e:
        print(f"Error abriendo puertos serial: {e}")
        return

    # Crear/abrir archivo CSV en modo append
    with open("log_lora_microseconds.csv", mode="a", newline='', encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        # Escribir encabezados si es la primera vez
        if csv_file.tell() == 0:
            writer.writerow(["Timestamp", "Device", "Message"])

        print("Comenzando lectura de puertos. Presiona Ctrl+C para terminar.")
        try:
            while True:
                # Leer Gateway
                if ser_gateway.in_waiting > 0:
                    line_gw = ser_gateway.readline().decode(errors='ignore').strip()
                    if line_gw:
                        for tag in MESSAGE_TAGS:
                            if tag in line_gw:
                                # Obtener timestamp con microsegundos
                                timestamp_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                                writer.writerow([timestamp_str, "Gateway", tag])
                                print(f"[{timestamp_str}] Gateway -> {tag}")
                                break

                # Leer Node
                if ser_node.in_waiting > 0:
                    line_node = ser_node.readline().decode(errors='ignore').strip()
                    if line_node:
                        for tag in MESSAGE_TAGS:
                            if tag in line_node:
                                timestamp_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                                writer.writerow([timestamp_str, "Nodo", tag])
                                print(f"[{timestamp_str}] Nodo -> {tag}")
                                break

                # Pausa para no saturar la CPU
                time.sleep(0.01)

        except KeyboardInterrupt:
            print("\nLectura interrumpida por el usuario (Ctrl+C).")

    # Cerrar los puertos serial
    ser_gateway.close()
    ser_node.close()

if __name__ == "__main__":
    main()
