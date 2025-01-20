import pandas as pd

# Configuración de los archivos
INPUT_CSV = "deep_sleep_log.csv"  # Archivo generado por el logger
OUTPUT_EXCEL = "deep_sleep_analysis.xlsx"  # Archivo de salida con el análisis

def analyze_deep_sleep(input_csv, output_excel):
    """
    Analiza los datos de sueño profundo desde un archivo CSV y guarda los resultados en un Excel.
    """
    # Cargar los datos
    try:
        data = pd.read_csv(input_csv)
    except FileNotFoundError:
        print(f"El archivo {input_csv} no se encontró. Asegúrate de que exista.")
        return

    # Filtrar solo los eventos con duración válida
    valid_data = data[data["Sleep Duration (ms)"].notnull()]

    # Convertir la columna de duración a números
    valid_data["Sleep Duration (ms)"] = valid_data["Sleep Duration (ms)"].astype(float)

    # Calcular estadísticas
    stats = {
        "Total Sleep Events": len(valid_data),
        "Min Duration (ms)": valid_data["Sleep Duration (ms)"].min(),
        "Max Duration (ms)": valid_data["Sleep Duration (ms)"].max(),
        "Average Duration (ms)": valid_data["Sleep Duration (ms)"].mean(),
        "Total Sleep Time (ms)": valid_data["Sleep Duration (ms)"].sum(),
    }

    # Crear un DataFrame para las estadísticas
    stats_df = pd.DataFrame.from_dict(stats, orient="index", columns=["Value"])

    # Guardar los resultados en un Excel
    with pd.ExcelWriter(output_excel) as writer:
        valid_data.to_excel(writer, sheet_name="Raw Data", index=False)
        stats_df.to_excel(writer, sheet_name="Summary")

    print(f"Análisis completado. Los resultados se han guardado en {output_excel}")

# Ejecutar el análisis
if __name__ == "__main__":
    analyze_deep_sleep(INPUT_CSV, OUTPUT_EXCEL)
