import pandas as pd
import matplotlib.pyplot as plt

file_path = 'CAD_error_rate_10s.csv'
data = pd.read_csv(file_path)

sent_messages = data[data['Message'].str.contains(r'\[TX\] Enviando ->')]
received_messages = data[data['Message'].str.contains(r'\[RX\] Paquete recibido ->')]

sent_messages['ID'] = sent_messages['Message'].str.extract(r'Hola mundo #(\d+)').astype(float)
received_messages['ID'] = received_messages['Message'].str.extract(r'Hola mundo #(\d+)').astype(float)

total_sent = len(sent_messages)
total_received = len(received_messages)

error_rate = ((total_sent - total_received) / total_sent) * 100

print(f"Total de mensajes enviados: {total_sent}")
print(f"Total de mensajes recibidos: {total_received}")
print(f"Tasa de error: {error_rate:.2f}%")

plt.figure(figsize=(8, 6))
plt.bar(['Enviados', 'Recibidos'], [total_sent, total_received], color=['blue', 'green'])
plt.title('Análisis de transmisión de mensajes')
plt.ylabel('Número de mensajes')
plt.xlabel('Estado del mensaje')
plt.text(0, total_sent, f'{total_sent}', ha='center', va='bottom', fontsize=12)
plt.text(1, total_received, f'{total_received}', ha='center', va='bottom', fontsize=12)
plt.show()
