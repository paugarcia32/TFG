# LoRa Gateway & Node comunication


The communication process follows these steps:

1. Beacon Transmission by Nodes: Each node starts by sending a beacon message using an initial Spreading Factor (SF) of 12.
2. ACK Reception at Gateway: The gateway receives the beacon and responds with an acknowledgment (ACK) to confirm receipt.
3. SF Negotiation Initiated by Nodes: Upon receiving the ACK, the node sends a message to negotiate the optimal SF for efficient communication.
4. Optimal SF Determination by Gateway: The gateway calculates the optimal SF based on signal metrics and sends this information back to the node.
5. Data Transmission by Nodes: The node adjusts its SF to the agreed value and sends the data using the new SF.

    

![Flow Diagram 2](https://github.com/user-attachments/assets/55dcff4e-d47f-40e9-96a5-a32c0c6b3029)
