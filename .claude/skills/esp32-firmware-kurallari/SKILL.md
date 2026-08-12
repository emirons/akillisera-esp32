---
name: esp32-firmware-kurallari
description: ESP32 Arduino firmware yazarken uyulacak zorunlu kalıplar. Millis tabanlı bloklamayan zamanlama, aktif-LOW röle sürme, ESP32 core 3.x LEDC API, ADC1/ADC2 Wi-Fi çakışması, DHT11 NaN kontrolü, PROGMEM web servisi, F() flash string. Bu projede (akillisera / ESP32 / Arduino / sera firmware) her kod yazımından önce oku.
---

# ESP32 Firmware Kuralları — Akıllı Sera

Bu skill, `akillisera` ESP32 firmware'inde **her modül için geçerli** zorunlu kalıpları içerir. Kod yazmadan önce bu kalıpları uygula.

## 1. millis() tabanlı bloklamayan zamanlama — `delay()` YASAK

`loop()` içinde `delay()` web server, mDNS ve bulut zamanlayıcılarını dondurur. Her periyodik iş kendi zamanlayıcısını tutar.

```cpp
static unsigned long lastRead = 0;
if (millis() - lastRead >= SENSOR_READ_INTERVAL) {
  lastRead = millis();
  // periyodik iş burada
}
```

Bloklamayan "pompa 5 sn açık" kalıbı (delay yok):

```cpp
if (pumpOn && millis() - pumpStart >= WATERING_DURATION) {
  setPump(false);           // süre doldu, kapat
  pumpOn = false;
}
```

Tek istisna: `setup()` içindeki bir defalık açılış beklemesi.

## 2. Aktif-LOW röle sürme — çıplak LOW/HIGH yazma

Röleler aktif-LOW: `digitalWrite(pin, LOW)` röleyi **açar**. Kafa karışıklığını `config.h` sabitleriyle önle:

```cpp
// config.h
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// kullanım
digitalWrite(PUMP_RELAY_PIN, RELAY_ON);   // pompa AÇIK
```

Asla modül kodunda çıplak `LOW`/`HIGH` yazma.

## 3. ESP32 core 3.x LEDC API — sürüm korumalı makro

Core 3.0 ile `ledcSetup()` + `ledcAttachPin()` **kaldırıldı**. Yerine `ledcAttach(pin, freq, res)` geldi ve `ledcWrite()` artık **pin** alıyor (kanal değil). Bu proje core **3.3.11** kullanıyor. `config.h`'daki makroları kullan, ham `ledc*` çağırma:

```cpp
// config.h — her iki sürümü destekler
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #define LED_PWM_BEGIN() ledcAttach(STRIP_LED_PIN, LED_PWM_FREQ, LED_PWM_RES)
  #define LED_PWM_WRITE(v) ledcWrite(STRIP_LED_PIN, (v))
#else
  #define LED_PWM_BEGIN() do { ledcSetup(LED_PWM_CH, LED_PWM_FREQ, LED_PWM_RES); \
                               ledcAttachPin(STRIP_LED_PIN, LED_PWM_CH); } while (0)
  #define LED_PWM_WRITE(v) ledcWrite(LED_PWM_CH, (v))
#endif
```

## 4. ADC1/ADC2 + Wi-Fi çakışması

Wi-Fi aktifken ADC2 pinleri `analogRead()`'te çöp döner. Sadece **ADC1** (GPIO32-39) okunabilir. Proje pinleri GPIO34/35 ADC1 — güvenli. Yeni analog pin gerekirse yalnızca GPIO32-39 seç.

## 5. DHT11 NaN kontrolü

DHT11 okuması başarısız olunca `NaN` döner. Her okuma `isnan()` ile kontrol edilir; NaN ise son geçerli değer korunur, sayaç arttırılır.

```cpp
float t = dht.readTemperature();
float h = dht.readHumidity();
if (isnan(t) || isnan(h)) {
  dhtErrorCount++;
  return;                   // eski değeri koru, çöp yayma
}
```

DHT11 en fazla 0.5 Hz — okuma aralığı >= 2000 ms olmalı.

## 6. PROGMEM'den web servisi

Web arayüzü HTML/CSS/JS `PROGMEM`'de tutulur, RAM'e kopyalanmadan `send_P` ile servis edilir:

```cpp
const char INDEX_HTML[] PROGMEM = R"HTML(...)HTML";
server.send_P(200, "text/html", INDEX_HTML);   // send DEĞİL, send_P
```

## 7. Seri çıktıda F() makrosu

Sabit string'ler flash'ta kalsın, RAM yemesin:

```cpp
Serial.println(F("Sensor okuma hatasi"));
```

## Genel

- Tüm sabitler yalnızca `config.h`'da — modüllerde magic number yok.
- Saf mantık `control_logic.h`'da, Arduino başlığı include etmeden — host'ta g++ ile test edilir.
- Her değişiklikten sonra derle: `arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/`
