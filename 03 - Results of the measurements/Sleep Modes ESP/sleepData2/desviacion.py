import pandas as pd
import matplotlib.pyplot as plt

# Archivo CSV
csv_filename = "detailed_sleep_data.csv"
output_svg = "variacion_tiempos.svg"

# Leer el archivo CSV con la codificación adecuada
try:
    # Leer el archivo con codificación 'latin-1'
    data = pd.read_csv(csv_filename, encoding='latin-1')

    # Filtrar los tiempos asociados a 'Up'
    elapsed_times = data.loc[data['Message'] == 'Up', 'Elapsed Time (µs)'].dropna()
    elapsed_times = pd.to_numeric(elapsed_times)  # Asegurarse de que sean numéricos

    # Calcular estadísticas
    mean_time = elapsed_times.mean()
    std_time = elapsed_times.std()

    print(f"Media de los tiempos: {mean_time:.2f} µs")
    print(f"Desviación estándar de los tiempos: {std_time:.2f} µs")

    # Crear un gráfico de líneas para mostrar la variación de los tiempos
    plt.figure(figsize=(10, 6))
    plt.plot(elapsed_times.index, elapsed_times, marker='o', linestyle='-', label="Tiempo de Despertar (µs)")
    plt.axhline(mean_time, color='red', linestyle='--', label=f"Media: {mean_time:.2f} µs")
    plt.fill_between(elapsed_times.index,
                     mean_time - std_time,
                     mean_time + std_time,
                     color='red',
                     alpha=0.2,
                     label=f"±1 Desviación: {std_time:.2f} µs")
    plt.title("Variación de Tiempos de Despertar del ESP32")
    plt.xlabel("Muestras")
    plt.ylabel("Tiempo (µs)")
    plt.legend()
    plt.grid(True)

    # Guardar el gráfico como archivo SVG
    plt.savefig(output_svg, format='svg')
    print(f"Gráfico guardado como: {output_svg}")

    # Mostrar el gráfico
    plt.show()

except Exception as e:
    print(f"Error al procesar el archivo CSV: {e}")

