# CLAUDE.md — ESP32 Akıllı Sera Firmware

> Bu dosyayı proje köküne kopyalayın. Claude Code her oturumda otomatik okur.

## Proje

ESP32 IoT Akıllı Sera & İklim Kontrol İstasyonu. **Firmware'in tamamı Kişi B'nin (benim) sorumluluğumda.** Donanım montajı Kişi A'da; devreye hiç dokunmam, yalnızca aşağıdaki pin eşleme tablosuna göre kod yazarım. Pin tablosu Kişi A ile yapılmış bir **arayüz sözleşmesidir** ve tek taraflı değiştirilemez.

## Komutlar

```bash
# Derleme (her değişiklikten sonra)
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --build-property build.partitions=huge_app --build-property upload.maximum_size=3145728 --output-dir build --warnings all akillisera/

# Host birim testleri
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests

# Yükleme
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:nodemcu-32s akillisera/

# Seri monitör
arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
```

## Pin haritası (DEĞİŞTİRİLEMEZ)

| Bileşen | Pin | Tip / Not |
|:---|:---|:---|
| Kapasitif toprak nem | GPIO34 | Analog giriş — **ADC1**, Wi-Fi ile uyumlu |
| DHT11 | GPIO4 | Dijital, one-wire |
| LDR | GPIO35 | Analog giriş — **ADC1** |
| LCD SDA / SCL | GPIO21 / GPIO22 | I2C, adres 0x27 |
| Röle Ch1 — su pompası | GPIO5 | Dijital çıkış, **AKTİF-LOW** |
| Röle Ch2 — tahliye fanı | GPIO18 | Dijital çıkış, **AKTİF-LOW** |
| Şerit LED (MOSFET gate) | GPIO25 | LEDC PWM, 5 kHz, 8 bit |
| Buzzer | GPIO19 | Dijital çıkış |
| Yeşil LED / Kırmızı LED | GPIO2 / GPIO15 | Dijital çıkış |
| Buton 1 (sulama) / Buton 2 (sayfa) | GPIO12 / GPIO14 | INPUT_PULLUP |

## Eşik değerleri

```
SOIL_DRY_THRESHOLD   = 2200     # üstünde toprak kuru -> sula
LIGHT_LOW_THRESHOLD  = 1500     # altında karanlık -> LED yak
TEMP_HIGH_THRESHOLD  = 35.0     # C, üstünde alarm + fan
HUM_HIGH_THRESHOLD   = 70.0     # %, üstünde fan aç
HUM_LOW_THRESHOLD    = 60.0     # %, altında fan kapat (histerezis)
WATERING_DURATION    = 5000     # ms
WATERING_COOLDOWN    = 60000    # ms, ardışık sulamalar arası minimum
LED_BRIGHTNESS       = 200      # 0-255
SENSOR_READ_INTERVAL = 2000     # ms (DHT11 en fazla 0.5 Hz)
LCD_REFRESH_INTERVAL = 500      # ms
CLOUD_INTERVAL       = 30000    # ms (API bütçesi — düşürme!)
TALKBACK_INTERVAL    = 30000    # ms (API bütçesi — düşürme!)
BUTTON_DEBOUNCE_MS   = 50       # ms
```

## Mutlak kurallar

1. **`loop()` içinde asla `delay()` kullanma.** Tüm zamanlama `millis()` tabanlı. Tek istisna: `setup()` içindeki bir defalık açılış mesajı beklemesi.
2. **Röleler AKTİF-LOW.** `digitalWrite(pin, LOW)` röleyi **açar**. Kodda `RELAY_ON` / `RELAY_OFF` sabitlerini kullan, çıplak `LOW`/`HIGH` yazma.
3. **Wi-Fi açıkken yalnızca ADC1 pinleri** (GPIO32-39) analog okunabilir. ADC2 pinleri çöp değer döndürür.
4. **Tüm sabitler yalnızca `config.h`'da.** Başka hiçbir dosyada çıplak sayı olmayacak.
5. **Saf mantık `control_logic.h`'da**, Arduino başlıklarına bağımlı olmadan — host'ta test edilebilmeli.
6. **Sırlar yalnızca `secrets.h`'da**, bu dosya `.gitignore`'da. `secrets.h.example` repoda kalır.
7. **Web arayüzü PROGMEM'de**, `server.send_P()` ile servis edilir (`send` değil).
8. **Doğrulamadan "tamamlandı" deme.** Her fazın sonunda derleme + testler yeşil olmalı ve çıktı gösterilmeli.

## ESP32 core sürüm farkı — LEDC

ESP32 Arduino core **3.0 ile `ledcSetup()` ve `ledcAttachPin()` kaldırıldı**. Yerine `ledcAttach(pin, freq, resolution)` geldi ve `ledcWrite()` artık kanal değil **pin** numarası alıyor. `config.h` içindeki `LED_PWM_BEGIN()` / `LED_PWM_WRITE()` makroları her iki sürümü de destekler. **Kodun geri kalanında ham `ledc*` fonksiyonlarını çağırma, yalnızca bu makroları kullan.**

## Modül bağımlılık sırası

```
config.h  ->  control_logic.h  ->  sensors.h / actuators.h  ->  display.h / buttons.h
                                                             ->  network.h / web_ui.h / cloud.h
                                                             ->  akillisera.ino
```

Alt katman üst katmanı include etmez. `control_logic.h` hiçbir donanım modülünü tanımaz.

## Mimari kararlar

- **Neden `.h` dosyaları, `.cpp` değil:** Arduino IDE her `.cpp`'yi ayrı derleme birimi yapar; `inline` fonksiyonlarla dolu `.h` dosyaları tek derleme birimi davranışını korur ve linker sorunlarını engeller.
- **Neden histerezis:** Tek eşikle fan, nem eşiğin etrafında salınırken saniyede birkaç kez açılıp kapanır (röle çatırdaması). 10 puanlık band bunu engeller.
- **Neden 60 sn sulama soğuması:** Toprak nem sensörü arızalanıp sürekli "kuru" derse bitki boğulur. Bu bir güvenlik kilididir.
- **Neden 30 sn bulut aralığı:** Ücretsiz ThingSpeak günde ~8000 istek kabul eder. İki görev 30 sn'de bir çalışınca 5760 istek olur; 15 sn'de 11520 olur ve limit aşılır. `config.h`'daki `static_assert` bunu derleme anında korur.
