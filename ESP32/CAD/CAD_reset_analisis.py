import pandas as pd
import matplotlib.pyplot as plt

CSV_FILE = 'cad_operational_data.csv'

data = pd.read_csv(CSV_FILE)

received_data = data[data['Event'] == 'Mensaje recibido']
cad_data = data[data['Event'] == 'CAD operativo']

time_differences = []

for _, received_row in received_data.iterrows():
    received_time = float(received_row['Value'])
    next_cad = cad_data[cad_data['Value'].astype(float) > received_time]

    if not next_cad.empty:
        cad_time = float(next_cad.iloc[0]['Value'])
        time_differences.append(cad_time - received_time)

if time_differences:
    average_time = sum(time_differences) / len(time_differences)
    print(f"Se calcularon {len(time_differences)} tiempos de reactivación del CAD.")
    print(f"Tiempo promedio de reactivación del CAD: {average_time:.4f} ms\n")

    print("Tiempos de reactivación del CAD:")
    for idx, diff in enumerate(time_differences):
        print(f"{idx + 1}: {diff:.4f} ms")

    plt.figure(figsize=(10, 6))
    plt.plot(time_differences, marker='o', linestyle='-', label='Tiempo de reactivación del CAD')
    plt.title('Tiempos de reactivación del CAD', fontsize=14)
    plt.xlabel('Número de evento', fontsize=12)
    plt.ylabel('Tiempo de reactivación (ms)', fontsize=12)
    plt.grid(True)
    plt.legend(fontsize=12)
    plt.tight_layout()
    plt.show()
else:
    print("No se encontraron datos suficientes para calcular tiempos de reactivación del CAD.")
