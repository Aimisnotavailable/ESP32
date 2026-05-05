#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Replace with your WiFi credentials
const char* ssid = "Tenda_1EBFF0";
const char* password = "kasapulamaccount";

WebServer server(80);
const int ledPin = 2;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>Hello from ESP32!</h1>
  <p>LED is <span id="state">%STATE%</span></p>
  <button onclick="toggleLED()">Toggle LED</button>
  <script>
    function toggleLED() {
      fetch('/toggle')
        .then(response => response.text())
        .then(data => { location.reload(); });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  String html = index_html;
  html.replace("%STATE%", digitalRead(ledPin) ? "ON" : "OFF");
  server.send(200, "text/html", html);
}

void handleToggle() {
  digitalWrite(ledPin, !digitalRead(ledPin));
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}