#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <RF24.h>

// ---------- Wi‑Fi Configuration ----------
const char *ssid = "JammerControl";
const char *password = "";  // open network

// ---------- Hardware ----------
SPIClass vspi(VSPI);
SPIClass hspi(HSPI);

RF24 radioH(16, 15);  // HSPI: CE=16, CSN=15
RF24 radioV(22, 21);  // VSPI: CE=22, CSN=21

WebServer server(80);
volatile bool armed = false;

// ---------- Web Handlers ----------
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Jammer Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #222; color: #eee; }
    .btn { 
      font-size: 32px; padding: 20px 40px; 
      background-color: #cc0000; color: white; border: none; 
      border-radius: 10px; cursor: pointer; box-shadow: 0 4px 8px rgba(255,0,0,0.5);
    }
    .btn:hover { background-color: #ff0000; }
  </style>
</head>
<body>
  <h1>Jammer Control Panel</h1>
  <p>Press the button to arm the jammer.<br>Wi‑Fi will disconnect and jamming begins immediately.</p>
  <button class="btn" onclick="arm()">ARM JAMMER</button>
  <script>
    function arm() {
      if (confirm('This can only be stopped by a hardware reset. Proceed?')) {
        fetch('/arm').then(response => response.text()).then(text => {
          document.body.innerHTML = '<h1>' + text + '</h1>';
        });
      }
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleArm() {
  server.send(200, "text/plain", "ARMED. Wi‑Fi will shut down in 2 seconds.");
  delay(2000);
  armed = true;
}

// Dummy handler to stop favicon errors
void handleFavicon() {
  server.send(204);  // No Content
}

// ---------- Jamming Routine ----------
void jamForever() {
  const uint8_t firstChannel = 2;
  const uint8_t lastChannel  = 80;
  const int dwellMs = 1;                // 1 ms per channel
  unsigned long sweepCount = 0;

  uint8_t chEven = firstChannel;
  uint8_t chOdd  = firstChannel + 1;

  // Start initial carriers
  radioH.startConstCarrier(RF24_PA_MAX, chEven);
  radioV.startConstCarrier(RF24_PA_MAX, chOdd);

  while (true) {
    // Stop carriers
    radioH.powerDown();
    radioV.powerDown();

    // Advance channels
    chEven = (chEven + 2 > lastChannel) ? firstChannel : chEven + 2;
    chOdd  = (chOdd  + 2 > lastChannel) ? firstChannel + 1 : chOdd + 2;

    // Restart on new channels
    radioH.startConstCarrier(RF24_PA_MAX, chEven);
    radioV.startConstCarrier(RF24_PA_MAX, chOdd);

    delay(dwellMs);

    // Debug print every 100 sweeps
    sweepCount++;
    if (sweepCount % 100 == 0) {
      Serial.printf("Sweep #%lu – channels %d & %d\n", sweepCount, chEven, chOdd);
    }
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Dual nRF24L01 Jammer (Educational) ===");

  // SPI and radio init
  vspi.begin();
  hspi.begin();

  if (!radioH.begin(&hspi)) {
    Serial.println("HSPI radio init failed!");
  } else {
    Serial.println("HSPI radio OK.");
    radioH.setPALevel(RF24_PA_MAX);
    radioH.setAutoAck(false);
    radioH.stopListening();
  }

  if (!radioV.begin(&vspi)) {
    Serial.println("VSPI radio init failed!");
  } else {
    Serial.println("VSPI radio OK.");
    radioV.setPALevel(RF24_PA_MAX);
    radioV.setAutoAck(false);
    radioV.stopListening();
  }

  // Wi‑Fi AP (visible)
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/favicon.ico", handleFavicon);   // <-- silences the error
  server.begin();
  Serial.println("Web server ready. Connect to \"" + String(ssid) + "\"");
}

// ---------- Main Loop ----------
void loop() {
  if (!armed) {
    server.handleClient();
  } else {
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("Jamming started. Only a hardware reset will stop it.");
    delay(100);
    jamForever();
  }
}