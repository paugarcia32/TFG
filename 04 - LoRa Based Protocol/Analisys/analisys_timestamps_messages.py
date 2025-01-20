import pandas as pd

def load_and_prepare_data(file_path):
    """
    Carga el archivo CSV con los datos y convierte la columna de timestamp a datetime con microsegundos.
    """
    # Cargar el archivo CSV
    data = pd.read_csv(file_path)

    # Convertir la columna de timestamp a formato datetime
    data['Timestamp'] = pd.to_datetime(data['Timestamp'], format='%Y-%m-%d %H:%M:%S.%f')

    # Asegurarse de que los datos estén ordenados por tiempo
    data = data.sort_values(by="Timestamp").reset_index(drop=True)

    return data

def compute_intervals(df, eventA, eventB, emitterA=None, emitterB=None):
    """
    Calcula los intervalos de tiempo entre eventos A y B, con opción de filtrar por emisor.
    """
    intervals = []
    for i, row in df.iterrows():
        if row['Message'] == eventA and (emitterA is None or row['Device'] == emitterA):
            # Buscar el siguiente evento B que ocurra después del evento A
            next_eventB = df[
                (df['Message'] == eventB) &
                (df['Timestamp'] > row['Timestamp']) &
                (emitterB is None or df['Device'] == emitterB)
            ]
            if not next_eventB.empty:
                interval = (next_eventB.iloc[0]['Timestamp'] - row['Timestamp']).total_seconds()
                intervals.append(interval)
    return intervals

def analyze_lora_protocol(file_path, output_path):
    """
    Analiza el protocolo LoRa con base en un archivo de logs y genera un Excel con los resultados.
    """
    # Cargar y preparar los datos
    data = load_and_prepare_data(file_path)

    # Definir los pares de eventos a analizar
    analysis_params = [
        ("BRX", "RTX", "Nodo", "Nodo"),  # Nodo: tiempo entre BRX y RTX
        ("SRX", "DTX", "Nodo", "Nodo"),  # Nodo: tiempo entre SRX y DTX
        ("DRX", "BTX", "Gateway", "Gateway"),  # Gateway: tiempo entre último DRX y siguiente BTX
        ("BTX", "BTX", "Gateway", "Gateway"),  # Gateway: tiempo entre beacons consecutivos
    ]

    # Analizar intervalos y calcular estadísticas
    results = {}
    for eventA, eventB, emitterA, emitterB in analysis_params:
        intervals = compute_intervals(data, eventA, eventB, emitterA, emitterB)
        if intervals:
            results[f"{eventA} -> {eventB} ({emitterA})"] = {
                "min": min(intervals),
                "max": max(intervals),
                "avg": sum(intervals) / len(intervals),
                "count": len(intervals)
            }
        else:
            results[f"{eventA} -> {eventB} ({emitterA})"] = {
                "min": None,
                "max": None,
                "avg": None,
                "count": 0
            }

    # Convertir resultados a DataFrame
    results_df = pd.DataFrame.from_dict(results, orient="index")
    results_df.index.name = "Event Pair"

    # Guardar los resultados en un archivo Excel
    results_df.to_excel(output_path, sheet_name="Analysis Results")

    print(f"Análisis completado. Los resultados se han guardado en {output_path}")

# Configura la ruta del archivo de entrada y salida
input_file = "log_lora_microseconds.csv"  # Cambia a la ruta de tu archivo
output_file = "lora_protocol_analysis.xlsx"  # Archivo de salida

# Ejecuta el análisis
analyze_lora_protocol(input_file, output_file)
