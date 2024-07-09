#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "VIGNESH";  // SSID for the ESP32 Access Point
const char* password = "12345678";  // Password for the ESP32 Access Point

#define IR_SENSOR_PIN 15 // Define the IR sensor pin (GPIO 15 on ESP32)
#define ALARM_PIN 2// Define the alarm pin (GPIO 0 on ESP32)
#define THRESHOLD_TIME 1600 // Threshold time in milliseconds (2 seconds)

WebServer server(80);  // Initialize WebServer on port 80

String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Driver Alert System</title>
<style>
/* CSS Styles */
body {
    font-family: Arial, sans-serif;
    background-color: #f8f9fa; /* Light grey background */
    margin: 0;
    padding: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh; /* Full height */
}

.container {
    text-align: center;
}

.status-box {
    width: 200px;              /* Width of the status box */
    height: 100px;             /* Height of the status box */
    margin: auto;              /* Center horizontally */
    border-radius: 10px;       /* Rounded corners */
    font-size: 24px;           /* Font size */
    font-weight: bold;         /* Bold font */
    color: white;              /* Text color */
    line-height: 100px;        /* Center text vertically */
    background-color: #007bff; /* Blue background */
    transition: background-color 0.3s ease; /* Smooth color transition */
}

/* Red background for sleeping status */
.status-box.sleeping {
    background-color: #FF5733;
}

/* Green background for awake status */
.status-box.awake {
    background-color: #4CAF50;
}

/* Responsive design for smaller screens */
@media only screen and (max-width: 600px) {
    .status-box {
        width: 150px; /* Adjusted width */
        height: 75px; /* Adjusted height */
        font-size: 18px; /* Adjusted font size */
        line-height: 75px; /* Adjusted line height */
    }
}
</style>
</head>
<body>
<div class="container">
    <h1>Driver Alert System</h1>
    <!-- Status box to display driver status -->
    <div class="status-box" id="status-box">Connecting...</div>
</div>
<!-- JavaScript to update status and apply styles -->
<script>
function updateStatus() {
    var xhttp = new XMLHttpRequest(); // Create XMLHttpRequest object
    xhttp.onreadystatechange = function() { // Define callback function
        if (this.readyState == 4 && this.status == 200) { // Check if request is complete and successful
            var statusBox = document.getElementById("status-box"); // Get status box element
            statusBox.innerHTML = this.responseText; // Update status text
            // Apply appropriate class based on status response
            if (this.responseText.trim() === "Sleeping") {
                statusBox.classList.remove("awake");
                statusBox.classList.add("sleeping");
            } else {
                statusBox.classList.remove("sleeping");
                statusBox.classList.add("awake");
            }
        }
    };
    xhttp.open("GET", "/status", true); // Initialize request
    xhttp.send(); // Send request
}
setInterval(updateStatus, 1000); // Update status every second
</script>
</body>
</html>
)";

unsigned long lastDetectionTime = 0; // Variable to store the time of last detection

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  delay(1000);  // Allow time for the serial monitor to start
  
  // Set up ESP32 as an access point
  WiFi.softAP(ssid, password);

  IPAddress apIP = WiFi.softAPIP();  // Get IP address of the access point
  Serial.print("Access Point IP address: ");
  Serial.println(apIP);

  pinMode(IR_SENSOR_PIN, INPUT); // Set IR sensor pin as input
  pinMode(ALARM_PIN, OUTPUT); // Set alarm pin as output

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", html);
  });

  server.on("/status", HTTP_GET, []() {
    server.send(200, "text/plain", getDriverStatus());
  });

  server.begin();  // Start server
}

void loop() {
  server.handleClient();  // Handle incoming client requests

  // Check driver status and trigger alarm if necessary
  checkDriverStatus();
}

String getDriverStatus() {
  int irSensorValue = digitalRead(IR_SENSOR_PIN); // Read the IR sensor value
  if (irSensorValue == HIGH) {
    return "Awake";
  } else {
    if (millis() - lastDetectionTime >= THRESHOLD_TIME) {
      return "Sleeping";
    } else {
      return "Awake";
    }
  }
}

void checkDriverStatus() {
  int irSensorValue = digitalRead(IR_SENSOR_PIN); // Read IR sensor value
  
  if (irSensorValue == HIGH) {
    // IR sensor detects presence (no sleep)
    digitalWrite(ALARM_PIN, LOW); // Turn off buzzer
    lastDetectionTime = millis(); // Update last presence time
  } else {
    // IR sensor doesn't detect presence (sleep)
    // Check if the time since last presence detection exceeds the threshold
    if (millis() - lastDetectionTime >= THRESHOLD_TIME) {
      digitalWrite(ALARM_PIN, HIGH); // Turn on buzzer
    } else {
      digitalWrite(ALARM_PIN, LOW); // Turn off buzzer
    }
  }
}
