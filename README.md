# ecen5823-f22-assignments
Starter code based on Gecko SDK 3.2.3
# Smart Vitals 
## Shall have ADPS9960 Gesture sensor along with MAX Bio Hub Pulse oximeter & Heartrate sensor as peripherals. And one blue gecko as a client and other as server functionality. For temperature monitoring we are using on board SI7021 Temperature and Humidity sensor.
•	Gesture Recognition: The Smart Vital shall reliably detect hand gestures using the interfaced gesture sensor and trigger corresponding actions such as for right gesture detection respond with oximeter reading on the LCD of client, likewise for left with temperature reading and up with heartrate reading.
•	Sensor Activation: The Smart Vital shall activate the appropriate health sensor (Pulse Oximeter, Heart Rate, or Temperature sensor) based on the gesture input.
•	BLE Communication: Bluetooth Low Energy (BLE) for efficient wireless communication between the server and client boards. The server board, equipped with a gesture sensor, activates specific health sensors (Pulse Oximeter, Heart Rate, or Temperature) based on the user's hand gestures. Each sensor collects vital health data which is then sent to the client board via BLE. This communication protocol was chosen for its low power consumption and suitability for transmitting small amounts of data over short distances, enhancing the system’s battery life and reliability. The client board displays this health data on an LCD and provides alerts if anomalies are detected, offering a comprehensive monitoring solution. This system promises to be a valuable tool in settings ranging from personal fitness tracking to clinical health monitoring.

Project Demo video: https://drive.google.com/file/d/1vkBvkXxIwrtu6cqnhG1cDkQOUY0x6p-Q/view?usp=drive_link
