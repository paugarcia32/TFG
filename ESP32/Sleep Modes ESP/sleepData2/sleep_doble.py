import serial
import csv
import time

port = "COM5"
baudrate = 115200
timeout = 1

csv_filename = "detailed_sleep_data_doble.csv"

last_down_time = None
last_up_time = None
previous_down_time = None
previous_up_time = None

try:
    ser = serial.Serial(port, baudrate, timeout=timeout)
    print(f"Conectado al puerto {port}")

    with open(csv_filename, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow([
            "Timestamp", 
            "Message", 
            "Elapsed Time (µs)", 
            "Time Between D-D (µs)", 
            "Time Between U-U (µs)"
        ])

        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                current_time = time.time_ns()
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

                if line == "D":
                    if last_down_time is not None:
                        time_between_downs = (current_time - last_down_time) // 1000
                    else:
                        time_between_downs = ""

                    previous_down_time = last_down_time
                    last_down_time = current_time

                    csv_writer.writerow([timestamp, "Down", "", time_between_downs, ""])
                    print(f"Datos guardados: {timestamp}, Down, Time Between D-D: {time_between_downs} µs")

                elif line == "U":
                    if last_up_time is not None:
                        time_between_ups = (current_time - last_up_time) // 1000
                    else:
                        time_between_ups = ""

                    if last_down_time is not None:
                        elapsed_time = (current_time - last_down_time) // 1000
                    else:
                        elapsed_time = ""

                    previous_up_time = last_up_time
                    last_up_time = current_time

                    csv_writer.writerow([timestamp, "Up", elapsed_time, "", time_between_ups])
                    print(f"Datos guardados: {timestamp}, Up, {elapsed_time} µs, Time Between U-U: {time_between_ups} µs")

except KeyboardInterrupt:
    print("\nFinalizando programa...")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Conexión serial cerrada.")

