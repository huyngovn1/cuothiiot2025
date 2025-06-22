#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <SPI.h>
#include <LoRa.h>

// WiFi setup
WiFiMulti wifiMulti;
const char *ssid = "Xiaomi 13";
const char *pass = "123456788";

// Chân LoRa
#define SS 5
#define RST 14
#define DIO0 2

// Chân I/O
#define CONTROL_PIN 4       // D4 - nút nhấn
#define OUTPUT_PIN_25 25    // D25 - điều khiển LED

// WebSocket
WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
       Serial.println(" WebSocket connected!");
      break;
    case WStype_DISCONNECTED:
       Serial.println(" WebSocket disconnected!");
      break;
    case WStype_TEXT:
      Serial.printf(" WebSocket message: %s\n", payload);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  // WiFi connect
  wifiMulti.addAP(ssid, pass);
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println(" WiFi connected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  // LoRa init
  SPI.begin();
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println(" LoRa init failed!");
    while (1);
  }
  Serial.println(" LoRa Receiver started!");

  // Pin config
  pinMode(CONTROL_PIN, INPUT_PULLUP);     // Nút nhấn (LOW khi nhấn)
  pinMode(OUTPUT_PIN_25, OUTPUT);         // LED
  digitalWrite(OUTPUT_PIN_25, HIGH);      // Mặc định LED tắt (HIGH)
  
  // WebSocket config
  webSocket.beginSSL("api.cungcapdichvunchts.id.vn", 443, "/");

  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  // Đọc trạng thái nút và điều khiển LED tương ứng
  bool buttonState = digitalRead(CONTROL_PIN);

  if (buttonState == LOW) {
    // Nút đang được nhấn
    digitalWrite(OUTPUT_PIN_25, LOW);   // Bật LED
  } else {
    // Nút thả ra
    digitalWrite(OUTPUT_PIN_25, HIGH);  // Tắt LED
  }

  // Nhận dữ liệu từ LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedText = "";
    while (LoRa.available()) {
      receivedText += (char)LoRa.read();
    }

    Serial.println("------ Received from LoRa ------");
    Serial.println(receivedText);
    Serial.println("--------------------------------");

    if (receivedText.indexOf("ID1") != -1 || receivedText.indexOf("ID2") != -1) {
      // Serial.println("Gửi dữ liệu WebSocket:");
      webSocket.sendTXT(receivedText.c_str());
      delay(100);
    }
  }
}
