// config.h — TEK doğruluk kaynağı.
// Projedeki HER pin, eşik ve zamanlama sabiti burada. Başka dosyada çıplak sayı YOK.
// Host'ta (g++) de derlenir: Arduino'ya bağımlı kısımlar #ifdef ARDUINO içinde.
#pragma once

#include <cstdint>

// ============================================================================
// PİN HARİTASI — Kişi A ile yapılan arayüz sözleşmesi. DEĞİŞTİRİLEMEZ.
// ============================================================================
constexpr uint8_t SOIL_PIN      = 34;  // Kapasitif toprak nem — analog, ADC1 (Wi-Fi uyumlu)
constexpr uint8_t DHT_PIN       = 4;   // DHT11 — dijital, one-wire
constexpr uint8_t LDR_PIN       = 35;  // LDR ışık — analog, ADC1 (Wi-Fi uyumlu)

constexpr uint8_t I2C_SDA_PIN   = 21;  // LCD I2C SDA (adres 0x27)
constexpr uint8_t I2C_SCL_PIN   = 22;  // LCD I2C SCL
constexpr uint8_t LCD_I2C_ADDR  = 0x27;

constexpr uint8_t PUMP_RELAY_PIN = 5;  // Röle Ch1 — su pompası. AKTİF-LOW.
constexpr uint8_t FAN_RELAY_PIN  = 18; // Röle Ch2 — tahliye fanı. AKTİF-LOW.

constexpr uint8_t LED_STRIP_PIN = 25;  // Şerit LED (MOSFET gate) — LEDC PWM
constexpr uint8_t BUZZER_PIN    = 19;  // Buzzer — dijital çıkış
constexpr uint8_t GREEN_LED_PIN = 2;   // Yeşil durum LED'i
constexpr uint8_t RED_LED_PIN   = 15;  // Kırmızı alarm LED'i

constexpr uint8_t BTN_WATER_PIN = 12;  // Buton 1 — manuel sulama. INPUT_PULLUP.
constexpr uint8_t BTN_PAGE_PIN  = 14;  // Buton 2 — LCD sayfa. INPUT_PULLUP.

// ============================================================================
// EŞİK DEĞERLERİ
// ============================================================================
constexpr int   SOIL_DRY_THRESHOLD  = 2200;   // ham ADC ÜSTÜnde -> toprak kuru -> sula
constexpr int   LIGHT_LOW_THRESHOLD = 1500;   // ham ADC ALTINDA -> karanlık -> LED yak
constexpr float TEMP_HIGH_THRESHOLD = 35.0f;  // C ÜSTÜnde -> alarm + fan
constexpr float HUM_HIGH_THRESHOLD  = 70.0f;  // % ÜSTÜnde -> fan aç
constexpr float HUM_LOW_THRESHOLD   = 60.0f;  // % ALTINDA -> fan kapat (histerezis alt bandı)
constexpr int   LED_BRIGHTNESS      = 200;    // 0-255, otomatik modda şerit LED parlaklığı

// Kapasitif toprak nem kalibrasyonu — ham değer TERS orantılı (kuru=yüksek, ıslak=düşük).
// Kişi A kalibrasyonda ölçtükten sonra güncellenecek.
constexpr int   SOIL_RAW_DRY = 3000;  // havada (kuru) ham okuma -> %0 nem
constexpr int   SOIL_RAW_WET = 1200;  // suda (ıslak) ham okuma -> %100 nem
constexpr int   DHT_MAX_FAILS = 5;    // ardışık bu kadar hatada sensör arızalı sayılır

// ============================================================================
// ZAMANLAMA SABİTLERİ (ms) — hepsi millis() tabanlı, delay() YASAK
// ============================================================================
constexpr unsigned long WATERING_DURATION    = 5000UL;   // pompa açık kalma süresi
constexpr unsigned long WATERING_COOLDOWN     = 60000UL;  // ardışık sulamalar arası min (güvenlik kilidi)
constexpr unsigned long WATERING_MIN_MS       = 2000UL;   // bitki/evre ölçeklemesi sonrası alt sınır
constexpr unsigned long WATERING_MAX_MS       = 15000UL;  // üst sınır — pompa güvenliği
constexpr unsigned long SENSOR_READ_INTERVAL  = 2000UL;   // DHT11 en fazla 0.5 Hz
constexpr unsigned long LCD_REFRESH_INTERVAL  = 500UL;
constexpr unsigned long CLOUD_INTERVAL        = 30000UL;  // ThingSpeak — API bütçesi, DÜŞÜRME
constexpr unsigned long TALKBACK_INTERVAL     = 30000UL;  // TalkBack — API bütçesi, DÜŞÜRME
constexpr unsigned long BUTTON_DEBOUNCE_MS    = 50UL;
constexpr unsigned long ALARM_BLINK_MS        = 500UL;   // alarm buzzer/LED yanıp sönme periyodu
constexpr unsigned long LCD_MESAJ_SURESI      = 2000UL;  // geçici LCD bildirimi ekranda kalma süresi

// ============================================================================
// LEDC PWM PARAMETRELERİ (şerit LED)
// ============================================================================
constexpr int LEDC_FREQ    = 5000;  // 5 kHz
constexpr int LEDC_RES     = 8;     // 8 bit -> 0-255
constexpr int LEDC_CHANNEL = 0;     // yalnızca core 2.x dalında kullanılır

// ============================================================================
// NTP SAAT + ZAMANLI SULAMA
// ============================================================================
constexpr char NTP_SUNUCU[]        = "pool.ntp.org";
constexpr long GMT_OFFSET_SN        = 3 * 3600;  // Türkiye UTC+3
constexpr int  YAZ_SAATI_OFFSET_SN  = 0;          // Türkiye yaz saati uygulamıyor
constexpr int  MAKS_SULAMA_ZAMANI   = 12;         // en fazla zamanlı sulama girdisi

// ============================================================================
// Arduino'ya bağımlı kısımlar — host derlemesinde (g++) atlanır.
// control_logic.h bu bloğa dokunmaz; host'ta test edilebilmesi için ayrık.
// ============================================================================
#ifdef ARDUINO

// --- Röle mantığı: AKTİF-LOW ---
// digitalWrite(pin, RELAY_ON) röleyi AÇAR. Modül kodunda çıplak LOW/HIGH YAZMA.
constexpr uint8_t RELAY_ON  = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

// --- LEDC sürüm koruması ---
// ESP32 Arduino core 3.0 ledcSetup()/ledcAttachPin()'i KALDIRDI; yerine
// ledcAttach(pin,freq,res) geldi ve ledcWrite() artık kanal değil PIN alıyor.
// Bu proje core 3.3.11 kullanıyor ama makro her iki sürümde de derlensin diye
// korunuyor. Kodun geri kalanı YALNIZCA LED_PWM_BEGIN()/LED_PWM_WRITE() çağırır.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define LED_PWM_BEGIN()   ledcAttach(LED_STRIP_PIN, LEDC_FREQ, LEDC_RES)
  #define LED_PWM_WRITE(d)  ledcWrite(LED_STRIP_PIN, (d))
#else
  #define LED_PWM_BEGIN()   do { ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RES); \
                                 ledcAttachPin(LED_STRIP_PIN, LEDC_CHANNEL); } while (0)
  #define LED_PWM_WRITE(d)  ledcWrite(LEDC_CHANNEL, (d))
#endif

#endif  // ARDUINO

// ============================================================================
// Derleme zamanı kontrolleri
// ============================================================================
static_assert(HUM_LOW_THRESHOLD < HUM_HIGH_THRESHOLD,
              "Histerezis bandi ters tanimlanmis: HUM_LOW >= HUM_HIGH");
static_assert(LED_BRIGHTNESS >= 0 && LED_BRIGHTNESS <= 255,
              "LED_BRIGHTNESS 0-255 araliginda olmali");
// ThingSpeak ücretsiz hesap günde ~8000 istek. İki görev (cloud + talkback)
// CLOUD_INTERVAL/TALKBACK_INTERVAL periyoduyla çalışır. Bütçeyi derlemede koru.
static_assert(2 * (86400000UL / CLOUD_INTERVAL) <= 8000UL,
              "CLOUD_INTERVAL cok dusuk: gunluk ThingSpeak istek limiti asilir");
