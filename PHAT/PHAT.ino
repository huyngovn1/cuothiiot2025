#include <Wire.h>
#include <DHT.h>
#include <BH1750FVI.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <LoRa.h>

// Khai báo ID thiết bị
#define DEVICE_ID "ID1"

// Khai báo chân kết nối
#define DHTPIN 15          // Chân DHT11
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN 32  

#define DRY_VALUE 3000  // Giá trị ADC khi đất khô
#define WET_VALUE 1000  // Giá trị ADC khi đất bão hòa

// Ngưỡng lỗi cảm biến ánh sáng
#define LIGHT_SENSOR_ERROR_THRESHOLD 54612  

// Chân LoRa
#define SS 5
#define RST 14
#define DIO0 2

// Khai báo cảm biến
DHT dht(DHTPIN, DHTTYPE);
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
Adafruit_BMP280 bmp;
bool bmpAvailable = true;  // Biến kiểm tra trạng thái BMP280

void setup() {
    Serial.begin(115200);
    dht.begin();
    LightSensor.begin();
    pinMode(SOIL_MOISTURE_PIN, INPUT);

    // Khởi động BMP280
    if (!bmp.begin(0x76)) {  // Địa chỉ I2C có thể là 0x76 hoặc 0x77
        Serial.println("Lỗi! Không tìm thấy BMP280.");
        bmpAvailable = false;  // Đánh dấu cảm biến không khả dụng
    }

    // Khởi động LoRa
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("Khởi tạo LoRa thất bại!");
        while (1);
    }
    Serial.println("LoRa Sender khởi động!");
}

void loop() {
    // Đọc dữ liệu từ DHT11
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Đọc độ ẩm đất
    int soilADC = analogRead(SOIL_MOISTURE_PIN);
    int soilMoisturePercent = map(soilADC, DRY_VALUE, WET_VALUE, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    // Đọc cường độ ánh sáng (Lux)
    uint16_t lux = LightSensor.GetLightIntensity();

    // Đọc áp suất từ BMP280 nếu khả dụng
    float pressure = 0;
    if (bmpAvailable) {
        pressure = bmp.readPressure() / 100.0F;  // Đổi từ Pa → hPa
    } else {
        Serial.println("Lỗi! Không đọc được BMP280.");
    }

    // Kiểm tra dữ liệu DHT11
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Lỗi! Không đọc được từ DHT11.");
    }

    // Kiểm tra lỗi cảm biến ánh sáng
    String lightStatus = (lux > LIGHT_SENSOR_ERROR_THRESHOLD) ? "Lỗi" : String(lux) + " Lux";

    // In dữ liệu lên Serial
    Serial.print(DEVICE_ID); Serial.print(" | ");
    Serial.print("Nhiệt độ DHT11: "); Serial.print(isnan(temperature) ? "Lỗi" : String(temperature)); Serial.print(" °C | ");
    Serial.print("Độ ẩm không khí: "); Serial.print(isnan(humidity) ? "Lỗi" : String(humidity)); Serial.print(" % | ");
    Serial.print("Độ ẩm đất: "); Serial.print(soilMoisturePercent); Serial.print(" % | ");
    Serial.print("Light: "); Serial.print(lightStatus); Serial.print(" | ");
    Serial.print("Áp suất: "); Serial.print(bmpAvailable ? String(pressure) : "Lỗi"); Serial.println(" hPa");
    Serial.println("--------------------------");

    // **Gửi dữ liệu qua LoRa**
    LoRa.beginPacket();
    LoRa.print(DEVICE_ID); LoRa.print(" | ");
    LoRa.print("Nhiệt độ DHT11: "); LoRa.print(isnan(temperature) ? "Lỗi" : String(temperature)); LoRa.print(" °C | ");
    LoRa.print("Độ ẩm không khí: "); LoRa.print(isnan(humidity) ? "Lỗi" : String(humidity)); LoRa.print(" % | ");
    LoRa.print("Độ ẩm đất: "); LoRa.print(soilMoisturePercent); LoRa.print(" % | ");
    LoRa.print("Light: "); LoRa.print(lightStatus); LoRa.print(" | ");
    LoRa.print("Áp suất: "); LoRa.print(bmpAvailable ? String(pressure) : "Lỗi"); LoRa.print(" hPa");
    LoRa.endPacket();

    delay(3000);
}
