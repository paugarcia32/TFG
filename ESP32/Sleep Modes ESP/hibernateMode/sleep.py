import serial
import csv
import time


port = "COM4"
baudrate = 115200
timeout = 1


csv_filename = "hibernate_data.csv"


last_down_time = None

try:
    ser = serial.Serial(port, baudrate, timeout=timeout)
    print(f"Conectado al puerto {port}")

    with open(csv_filename, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(["Timestamp", "Message", "Elapsed Time (µs)"])

        while True:

            line = ser.readline().decode('utf-8').strip()
            if line:
                current_time = time.time_ns()
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

                if line == "D":
                    last_down_time = current_time
                    csv_writer.writerow([timestamp, "Down", ""])
                    print(f"Datos guardados: {timestamp}, Down")
                elif line == "U" and last_down_time is not None:
                    elapsed_time = (current_time - last_down_time) // 1000
                    csv_writer.writerow([timestamp, "Up", elapsed_time])
                    print(f"Datos guardados: {timestamp}, Up, {elapsed_time} µs")
except KeyboardInterrupt:
    print("\nFinalizando programa...")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Conexión serial cerrada.")

