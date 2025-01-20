import os
import pandas as pd
import matplotlib.pyplot as plt

CSV_FILE = 'communication_log_SF12.csv'

if not os.path.exists(CSV_FILE):
    print(f"El archivo {CSV_FILE} no existe. Creando uno vacío...")
    df = pd.DataFrame(columns=['Timestamp', 'Source', 'Message'])
    df.to_csv(CSV_FILE, index=False)

messages_df = pd.read_csv(CSV_FILE)

messages_df['Cleaned_Message'] = messages_df['Message'].str.extract(r'-> (.*)')

tx_messages = messages_df[messages_df['Source'] == 'TX']
rx_messages = messages_df[messages_df['Source'] == 'RX']

time_differences = []

for _, tx_row in tx_messages.iterrows():
    tx_timestamp = tx_row['Timestamp']
    tx_content = tx_row['Cleaned_Message']

    matching_rx = rx_messages[rx_messages['Cleaned_Message'] == tx_content]
    if not matching_rx.empty:
        rx_timestamp = matching_rx.iloc[0]['Timestamp']
        time_differences.append((tx_content, rx_timestamp - tx_timestamp))

if time_differences:
    average_time = sum(diff[1] for diff in time_differences) / len(time_differences)
    print(f"Se encontraron {len(time_differences)} pares de mensajes TX-RX.")
    print(f"Tiempo promedio de detección: {average_time:.4f} segundos\n")

    print("Diferencias de tiempo para cada mensaje:")
    for message, diff in time_differences:
        print(f"Mensaje: {message}, Diferencia de tiempo: {diff:.4f} segundos")

    plt.figure(figsize=(10, 6))
    plt.plot([diff[1] for diff in time_differences], marker='o', linestyle='-', label='Tiempo de detección')
    plt.title('Tiempos de detección (TX a RX)', fontsize=14)
    plt.xlabel('Número de mensaje', fontsize=12)
    plt.ylabel('Tiempo de detección (segundos)', fontsize=12)
    plt.grid(True)
    plt.legend(fontsize=12)
    plt.tight_layout()
    plt.show()
else:
    print("No se encontraron pares de mensajes TX-RX.")
