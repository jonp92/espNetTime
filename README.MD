# espNetTime: ESP32 GPS NTP Server with Real-Time Web Dashboard

## Overview
espNetTime combines an ESP32 microcontroller, a GPS module, and various software components to create a GPS-enabled NTP server with a real-time web-based dashboard. The system synchronizes local time with GPS time and serves this accurate time over the network using NTP (Network Time Protocol). It also provides a web interface displaying GPS data, system time, and network information.

## Features
- **GPS Integration**: Retrieves accurate time and location data from a GPS module.
- **NTP Server**: Serves precise time to network clients using NTP.
- **Web Dashboard**: Real-time web interface displaying GPS information, system time, NTP server status, and network details.
- **Responsive Design**: Ensures usability across various devices and screen sizes.

## Hardware Requirements
- **ESP32 Development Board**
- **GPS Module** (e.g., NEO-6M)
- **Wires and Breadboard** for connections
- **LED** for status indication (optional)

## Software Requirements
- **Arduino IDE** with ESP32 Board Manager
- **Libraries**:
  - `TinyGPS++`
  - `ESPAsyncUDP`
  - `ESPAsyncWebServer`
  - `Ticker`
  - `TimeLib`
  - `LittleFS`

## Circuit Diagram
![Circuit Diagram](path/to/your/circuit-diagram.png)

## Installation

1. **Clone the Repository**
    ```sh
    git clone https://github.com/jonp92/espNetTime.git
    cd espNetTime
    ```

2. **Install Arduino Libraries**
   Open the Arduino IDE and install the following libraries via Library Manager or manually:
   - TinyGPS++
   - ESPAsyncUDP
   - ESPAsyncWebServer
   - Ticker
   - TimeLib
   - LittleFS

3. **Upload the Code**
   - Open the project in the Arduino IDE.
   - Select the appropriate board and port.
   - Upload the code to the ESP32.

## Usage

1. **Connect to Wi-Fi**
   The ESP32 will connect to the specified Wi-Fi network upon startup.

2. **Access the Web Dashboard**
   Open a web browser and navigate to the IP address of the ESP32. The dashboard displays GPS data, system time, and network information.

3. **NTP Server**
   Configure NTP clients on the network to use the ESP32 IP address as the NTP server for precise time synchronization.

## Web Dashboard Features
- **GPS Information**: Displays latitude, longitude, and the number of satellites.
- **Time Information**: Shows GPS time, local system time, and the offset between them.
- **NTP Server Status**: Indicates whether the NTP server is running and its IP address.


## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your improvements.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements
- [TinyGPS++ Library](https://github.com/mikalhart/TinyGPSPlus)
- [ESPAsyncUDP Library](https://github.com/me-no-dev/ESPAsyncUDP)
- [ESPAsyncWebServer Library](https://github.com/me-no-dev/ESPAsyncWebServer)

## Contact
For any inquiries, please contact [your-email@example.com](mailto:your-email@example.com).

---

### Screenshots
![Dashboard Screenshot](path/to/your/dashboard-screenshot.png)

---

Happy coding!
