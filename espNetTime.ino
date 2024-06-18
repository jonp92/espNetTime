#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <FS.h>

// Replace with your network credentials
const char* ssid = "jcl-wpa2";
const char* password = "Stealtheygrill";
// Change to your desired hostname otherwise leave as default
const char* hostname = "espTime";

const int localPort = 123; // Local port to listen for NTP packets

// GPS RX and TX pins (Set to the pins you are using)
#define GPS_RX_PIN D6
#define GPS_TX_PIN D5

// Set to true to enable debug output and optionally network debug (verbose NTP output)
bool debugMode = true;
bool networkDebug = false;

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

TinyGPSPlus gps;

// Create AsyncWebServer object on port 80 (can be changed to any port)
AsyncWebServer server(80);

AsyncEventSource events("/events");

// Create an AsyncUDP object to listen for NTP packets
AsyncUDP udp;

// Global variables
float latitude, longitude = 0.0; // Latitude and Longitude variables to store the GPS data
// unsigned long gpsTime = 0; // Date and Time variables to store the GPS data
time_t lastTime; // Last time the GPS data was updated
char serverStatus[50] = "stopped"; // Server status message


// Create a Ticker object to blink the LED
Ticker ledBlinker;

String timeToString(byte hours, byte minutes, byte seconds, byte hundredths) {
    char timeBuffer[12]; // Enough space for "HH:MM:SS.XX\0"
    sprintf(timeBuffer, "%02d:%02d:%02d.%02d", hours, minutes, seconds, hundredths);
    return String(timeBuffer);
}

String dateToString(uint16_t year, byte month, byte day) {
    char dateBuffer[11]; // Enough space for "MM/DD/YYYY\0"
    sprintf(dateBuffer, "%02d/%02d/%04d", month, day, year);
    return String(dateBuffer);
}

void blinkLed() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

time_t getGPSTime() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    if (gps.time.isValid()) {
        tmElements_t tm;
        tm.Hour = gps.time.hour();
        tm.Minute = gps.time.minute();
        tm.Second = gps.time.second();
        tm.Day = gps.date.day();
        tm.Month = gps.date.month() - 1;
        tm.Year = gps.date.year() - 1970;
        return makeTime(tm);
    }
    return 0;
}

void setupNTPServer() {
    if (udp.listen(localPort)) {
        strcpy(serverStatus, "running");
        Serial.println("UDP Listening on IP: " + WiFi.localIP().toString() + " Port: " + String(localPort));
        udp.onPacket([](AsyncUDPPacket packet) {
            byte packetBuffer[48] = {}; // Initialize buffer to zero

            // Extract T1 from the incoming packet
            unsigned long t1Seconds = (unsigned long)packet.data()[40] << 24 |
                                      (unsigned long)packet.data()[41] << 16 |
                                      (unsigned long)packet.data()[42] << 8 |
                                      (unsigned long)packet.data()[43];

            unsigned long t1Fraction = (unsigned long)packet.data()[44] << 24 |
                                       (unsigned long)packet.data()[45] << 16 |
                                       (unsigned long)packet.data()[46] << 8 |
                                       (unsigned long)packet.data()[47];

            Serial.println("Received NTP packet: " + packet.remoteIP().toString() + ":" + packet.remotePort());
            if (networkDebug) {
                Serial.print("T1: ");
                Serial.print(t1Seconds);
                Serial.print(".");
                Serial.println(t1Fraction);
                for (int i = 0; i < 48; i++) {
                    Serial.print(packet.data()[i], HEX);
                    Serial.print(", ");
                }
                Serial.println();
            }

            // Capture the current time as close as possible to the receive event
            time_t epochReceive = now();
            if (epochReceive == 0) {
                return; // If GPS time is invalid, do not respond
            }

            // NTP time starts on Jan 1 1900.
            const unsigned long seventyYears = 2208988800UL;
            unsigned long secondsSince1900Receive = epochReceive + seventyYears;
            unsigned long lastTimeNTP = lastTime + seventyYears;

            // Prepare NTP response
            packetBuffer[0] = 0b00100100; // LI, Version, Mode
            packetBuffer[1] = 1; // Stratum, or type of clock
            packetBuffer[2] = 6; // Polling Interval
            packetBuffer[3] = 0xEC; // Peer Clock Precision

            // Reference Timestamp (REF, time of last update)
            packetBuffer[16] = (lastTimeNTP >> 24) & 0xFF;
            packetBuffer[17] = (lastTimeNTP >> 16) & 0xFF;
            packetBuffer[18] = (lastTimeNTP >> 8) & 0xFF;
            packetBuffer[19] = lastTimeNTP & 0xFF;

            // Add Fractional Time
            packetBuffer[20] = 0;
            packetBuffer[21] = 0;
            packetBuffer[22] = 0;
            packetBuffer[23] = 0;

            // Originate Timestamp (T1 from client)
            packetBuffer[24] = (t1Seconds >> 24) & 0xFF;
            packetBuffer[25] = (t1Seconds >> 16) & 0xFF;
            packetBuffer[26] = (t1Seconds >> 8) & 0xFF;
            packetBuffer[27] = t1Seconds & 0xFF;
            packetBuffer[28] = (t1Fraction >> 24) & 0xFF;
            packetBuffer[29] = (t1Fraction >> 16) & 0xFF;
            packetBuffer[30] = (t1Fraction >> 8) & 0xFF;
            packetBuffer[31] = t1Fraction & 0xFF;

            // Receive Timestamp (T2, time request was received)
            packetBuffer[32] = (secondsSince1900Receive >> 24) & 0xFF;
            packetBuffer[33] = (secondsSince1900Receive >> 16) & 0xFF;
            packetBuffer[34] = (secondsSince1900Receive >> 8) & 0xFF;
            packetBuffer[35] = secondsSince1900Receive & 0xFF;

            // Transmit Timestamp (T3, time response is sent)
            time_t epochTransmit = now();
            unsigned long secondsSince1900Transmit = epochTransmit + seventyYears;

            packetBuffer[40] = (secondsSince1900Transmit >> 24) & 0xFF;
            packetBuffer[41] = (secondsSince1900Transmit >> 16) & 0xFF;
            packetBuffer[42] = (secondsSince1900Transmit >> 8) & 0xFF;
            packetBuffer[43] = secondsSince1900Transmit & 0xFF;

            if (networkDebug) {
                Serial.print("T2: ");
                Serial.print(secondsSince1900Receive);
                Serial.print(".");
                Serial.println(0);
                Serial.print("T3: ");
                Serial.print(secondsSince1900Transmit);
                Serial.print(".");
                Serial.println(0);
                Serial.println();
                Serial.println("Sending NTP packet");
                for (int i = 0; i < 48; i++) {
                    Serial.print(packetBuffer[i], HEX);
                    Serial.print(", ");
                }
            }

            // Send the response
            packet.write(packetBuffer, 48);
        });
    } else {
        strcpy(serverStatus, "failed");
        Serial.println("Failed to start UDP listener");
    }
}

char *ntpServerStatus() {
    return serverStatus;
}

// Example function to set system time from GPS
void setSystemTimeFromGPS(int year, int month, int day, int hours, int minutes, int seconds) {
    struct tm t;
    t.tm_year = year - 1900; // Year since 1900
    t.tm_mon = month - 1;    // Month, where 0 = jan
    t.tm_mday = day;         // Day of the month
    t.tm_hour = hours;
    t.tm_min = minutes;
    t.tm_sec = seconds;
    t.tm_isdst = 0;          // Is DST on? 1 = yes, 0 = no, -1 = unknown

    time_t timeSinceEpoch = mktime(&t);

    timeval tv = { timeSinceEpoch, 0 };
    settimeofday(&tv, NULL);
}

void connectToWiFi(const char* hostname = "", int retries = 10) {
    if (strlen(hostname) > 0){
        WiFi.setHostname(hostname);
    }
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    ledBlinker.attach(0.5, blinkLed);
    while (WiFi.status() != WL_CONNECTED && retries > 0) {
        delay(1000);
        Serial.print(".");
        retries--;
        if (retries % 2 == 0) {
            Serial.print("Attempting to reconnect to WiFi, retries left: ");
            Serial.println(retries);
        }
    }
    ledBlinker.detach();
    if (debugMode && WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        Serial.println("SSID: " + WiFi.SSID());
        Serial.println("IP Address: " + WiFi.localIP().toString());
    } else {
        Serial.println("Failed to connect to WiFi");
    }
}

String readFile(fs::FS &fs, const char * path) {
  String fileString = "";
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("Failed to open file for reading");
    return "";
  }

  while(file.available()){
    fileString += file.readString();
  }
  file.close();
  //Serial.println(fileString);
  return fileString;
}

String TemplateProcessor(String& htmlContent) {
    htmlContent.replace("{{LATITUDE}}", String(latitude, 6));
    htmlContent.replace("{{LONGITUDE}}", String(longitude, 6));
    htmlContent.replace("{{DATE}}", dateToString(gps.date.year(), gps.date.month(), gps.date.day()));
    htmlContent.replace("{{SYSTEM_TIME}}", timeToString(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond()));
    htmlContent.replace("{{SATELLITES}}", String(gps.satellites.value()));
    htmlContent.replace("{{GPS_TIME}}", timeToString(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond()));
    htmlContent.replace("{{NTP_STATUS}}", ntpServerStatus());
    htmlContent.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
    htmlContent.replace("{{RSSI}}", String(WiFi.RSSI()));
    htmlContent.replace("{{SSID}}", WiFi.SSID());
    return htmlContent;
}

void onUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File file;
  
  if (!index) {
    // Check if the file name is valid
    if (filename.endsWith(".html") || filename.endsWith(".css") || filename.endsWith(".js") || filename.endsWith(".png")) {
      // Open the file for writing
      file = LittleFS.open("/" + filename + ".tmp", "w");
      if (!file) {
        Serial.println("Failed to open file for writing");
        request->send(500, "text/plain", "Failed to open file for writing");
        return;
      }
    } else {
      request->send(400, "text/plain", "Invalid file type");
      return;
    }
  }


  // Write the received data to the file
  file.write(data, len);
  if (final) {
    // This is the end of the upload, close the file
    file.close();
    // Rename the temporary file to the final file
    LittleFS.rename("/" + filename + ".tmp", "/" + filename);
    Serial.println("File " + filename + " uploaded successfully");
    request->send(200, "text/plain", "File upload complete");
  }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);
    gpsSerial.begin(9600);
    connectToWiFi(hostname);
    //String htmlContent = "";
      // Load LittleFS and get web page data
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    } else {
        Serial.println("File system mounted");
        String htmlContent = readFile(LittleFS, "/index.html");
        if (htmlContent.length() == 0) {
        Serial.println("Failed to read index.html");
        }
    }
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String htmlContent = readFile(LittleFS, "/index.html");
        request->send(200, "text/html", TemplateProcessor(htmlContent));
        if (debugMode) {
        Serial.println("Request received from IP on /: " + request->client()->remoteIP().toString());
        }
    });
    // Route for CSS
    server.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.css", "text/css");
        if (debugMode) {
        Serial.println("Request received from IP on /index.css: " + request->client()->remoteIP().toString());
        }
    });
    // Route for JS
    server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.js", "text/javascript");
        if (debugMode) {
        Serial.println("Request received from IP on /index.js: " + request->client()->remoteIP().toString());
        }
    });
    server.on("/gps", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "Latitude: " + String(latitude, 6) + "\nLongitude: " + String(longitude, 6) + "\nDate: " + dateToString(gps.date.year(), gps.date.month(), gps.date.day())  + "\nTime: " + timeToString(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond() ) + "\nSatellites: " + String(gps.satellites.value()) + "\n";
        request->send(200, "text/plain", response);
        if (debugMode) {
        Serial.println("Request received from IP on /gps: " + request->client()->remoteIP().toString());
        }
    });
    server.on("/map", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("https://www.google.com/maps/search/?api=1&query=" + String(latitude, 6) + "," + String(longitude, 6));
        if (debugMode) {
        Serial.println("Request received from IP on /map: " + request->client()->remoteIP().toString());
        }
    });
    server.on("/ntpstatus", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", ntpServerStatus());
        if (debugMode) {
        Serial.println("Request received from IP on /ntpstatus: " + request->client()->remoteIP().toString());
        }
    });
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        // the request is handled in the onUpload callback
        if (debugMode) {
            Serial.println("Request received from IP on /upload: " + request->client()->remoteIP().toString());
        }
    }, onUpload);
    server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/upload.html", "text/html");
        if (debugMode) {
        Serial.println("Request received from IP on /upload: " + request->client()->remoteIP().toString());
        }
    });
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
        if (debugMode) {
        Serial.println("Request received from IP on unknown: " + request->client()->remoteIP().toString());
        }
    });
    events.onConnect([](AsyncEventSourceClient *client){
        if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        client->send("hello!", NULL, millis());
    });
    server.addHandler(&events); // Add an event source handler
    server.begin(); // Start the server
    setupNTPServer();
    ArduinoOTA.setHostname("espTime");
    // ArduinoOTA.setPassword("admin");
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (debugMode) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        }
     });
    ArduinoOTA.onError([](ota_error_t error) {
        if (debugMode) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
            } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
            } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
            } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
            } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
            }
        }
    });
    ArduinoOTA.begin();
    digitalWrite(LED_BUILTIN, HIGH); // Turn off the LED
}

void loop() {
    static unsigned long lastLogEvent = 0;
    ArduinoOTA.handle();
    digitalWrite(LED_BUILTIN, HIGH); // Turn off the LED

    while (gpsSerial.available() > 0) {
        if (gps.encode(gpsSerial.read())) {
            if (gps.location.isValid()) {
                digitalWrite(LED_BUILTIN, LOW); // Turn on the LED
                latitude = gps.location.lat();
                longitude = gps.location.lng();
            }
            if (gps.date.isValid() && gps.time.isValid()) {
                setSystemTimeFromGPS(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
                setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
                lastTime = now();
                char timeMessage[64];
                snprintf(timeMessage, sizeof(timeMessage), "%s", timeToString(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond()).c_str());
                events.send(timeMessage, "time", millis());
            }
        }
    }
    if (debugMode && millis() - lastLogEvent > 5000 || lastLogEvent == 0) {
        lastLogEvent = millis();
        Serial.println("System Time: " + timeToString(hour(), minute(), second(), 0));
        Serial.print("Satellites: ");
        Serial.println(gps.satellites.value());
        Serial.print("Latitude: ");
        Serial.println(latitude, 6);
        Serial.print("Longitude: ");
        Serial.println(longitude, 6);
        Serial.print("Date: ");
        String dateString = dateToString(gps.date.year(), gps.date.month(), gps.date.day());
        String time = timeToString(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
        Serial.println(dateString);
        Serial.println(gps.date.year());
        Serial.print("Time: ");
        Serial.println(time);
    }
}


