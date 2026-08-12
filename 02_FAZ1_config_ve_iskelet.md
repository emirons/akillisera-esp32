# Faz 1 — `config.h`: Tek Doğruluk Kaynağı

## 🎯 Amaç

Projedeki **her** pin numarasını, eşiği ve zamanlama sabitini tek bir dosyada toplamak; ESP32 core 2.x/3.x LEDC API farkını sürüm korumalı makrolarla kapatmak. Bu dosya, Kişi A ile yapılan pin eşleme sözleşmesinin koddaki karşılığıdır.

## 📋 Ön koşul

Faz 0 tamamlanmış, boş proje derleniyor, core sürümü biliniyor.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `.claude/skills/esp32-firmware-kurallari` | Otomatik tetiklenir |
| **WebSearch / WebFetch** | ESP32 core 3.x LEDC geçiş dokümanı |

## 📁 Dosyalar

`akillisera/config.h` (yeni) · `akillisera/secrets.h.example` (yeni) · `akillisera/akillisera.ino` (güncellenir)

---

## 💬 PROMPT 1.1 — config.h

```text
akillisera/config.h dosyasını yaz. Bu dosya projedeki TEK sabit kaynağı olacak;
başka hiçbir dosyada çıplak pin numarası veya eşik değeri bulunmayacak.

İÇERİK:

1. Include guard (#pragma once).

2. PIN TANIMLARI — CLAUDE.md'deki pin tablosunun birebir aynısı. Her satırın
   yanına hangi bileşen olduğunu ve varsa özel notunu yorum olarak yaz
   (örn. "ADC1 - Wi-Fi ile uyumlu", "AKTİF-LOW").
   constexpr uint8_t kullan, #define kullanma (tip güvenliği için).
   İstisna: LEDC makroları #define olacak.

3. EŞİK DEĞERLERİ — CLAUDE.md'deki tüm eşikler. constexpr, doğru tiplerle
   (int/float). Her birinin yanına hangi yönde tetiklediğini yaz.

4. ZAMANLAMA SABİTLERİ — WATERING_DURATION, CLOUD_INTERVAL, TALKBACK_INTERVAL,
   SENSOR_READ_INTERVAL (2000 ms), LCD_REFRESH_INTERVAL (500 ms),
   BUTTON_DEBOUNCE_MS (50 ms). Hepsi constexpr unsigned long.

5. RÖLE MANTIĞI — röleler aktif-LOW olduğu için okunabilirlik adına:
   constexpr uint8_t RELAY_ON  = LOW;
   constexpr uint8_t RELAY_OFF = HIGH;
   Böylece kodun geri kalanında digitalWrite(RELAY_PUMP, RELAY_ON) yazılır ve
   kimse LOW/HIGH karıştırmaz.

6. LEDC SÜRÜM KORUMASI — kritik bölüm. ESP32 Arduino core 3.0 ile ledcSetup()
   ve ledcAttachPin() kaldırıldı; yerine ledcAttach(pin, freq, resolution) geldi
   ve ledcWrite() artık kanal yerine pin numarası alıyor. Her iki sürümde de
   derlenen bir soyutlama yaz:

   #if ESP_ARDUINO_VERSION_MAJOR >= 3
     #define LED_PWM_BEGIN()   ledcAttach(LED_STRIP_PIN, LEDC_FREQ, LEDC_RES)
     #define LED_PWM_WRITE(d)  ledcWrite(LED_STRIP_PIN, (d))
   #else
     #define LED_PWM_BEGIN()   do { ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RES); \
                                    ledcAttachPin(LED_STRIP_PIN, LEDC_CHANNEL); } while (0)
     #define LED_PWM_WRITE(d)  ledcWrite(LEDC_CHANNEL, (d))
   #endif

   Kodun geri kalanı YALNIZCA LED_PWM_BEGIN() ve LED_PWM_WRITE() kullanacak.
   Bu makroların üstüne, neden var olduklarını açıklayan 3-4 satırlık bir yorum
   bloğu yaz — 6 ay sonra bu dosyaya bakan kişi anlasın.

   Yazmadan önce ESP32 core 3.x LEDC geçiş dokümanını web'den doğrula; API
   imzalarının doğru olduğundan emin ol.

7. Dosyanın en altına derleme zamanı kontrolleri ekle:
   static_assert(HUM_LOW_THRESHOLD < HUM_HIGH_THRESHOLD,
                 "Histerezis bandi ters tanimlanmis");
   static_assert(LED_BRIGHTNESS >= 0 && LED_BRIGHTNESS <= 255,
                 "LED_BRIGHTNESS 0-255 araliginda olmali");
   Bu, birinin eşikleri yanlışlıkla ters yazmasını derleme anında yakalar.

Sonra akillisera.ino'ya #include "config.h" ekle ve derle. Derleme çıktısını göster.
```

---

## 💬 PROMPT 1.2 — secrets ayrımı

```text
akillisera/secrets.h.example dosyasını oluştur. İçinde gerçek değerler DEĞİL,
şu şablon olsun:

#pragma once
// Bu dosyayi secrets.h olarak kopyalayip kendi degerlerinizi girin.
// secrets.h .gitignore icindedir, repoya yuklenmez.

constexpr char WIFI_SSID[]     = "AGINIZIN_ADI";
constexpr char WIFI_PASSWORD[] = "AG_PAROLASI";

constexpr char TS_WRITE_API_KEY[] = "THINGSPEAK_WRITE_API_KEY";
constexpr char TS_CHANNEL_ID[]    = "THINGSPEAK_CHANNEL_ID";

constexpr char TB_ID[]  = "TALKBACK_ID";
constexpr char TB_KEY[] = "TALKBACK_API_KEY";

Sonra bu dosyayı secrets.h olarak kopyala (placeholder değerlerle kalabilir,
gerçek anahtarları Faz 8'de gireceğim) ve config.h'ın secrets.h'ı include
etmediğinden emin ol — secrets.h'ı yalnızca ona ihtiyaç duyan modüller
(network.h, cloud.h) include edecek.

.gitignore'da secrets.h olduğunu doğrula ve `git status` çıktısıyla secrets.h'ın
takip edilmediğini bana göster.
```

---

## ✅ Doğrulama komutları

```bash
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
git status --short                      # secrets.h listede OLMAMALI
grep -rn "GPIO\|[0-9]\{4\}" akillisera/akillisera.ino   # config.h dışında sabit olmamalı
```

**Sürüm koruması testi** — her iki dalın da derlendiğini kanıtlamak için:

```bash
# Mevcut sürümle derle (yukarıdaki komut)
# Sonra makronun diğer dalını geçici olarak zorla test et:
#   config.h'daki #if satırını geçici olarak #if 0 yapıp derleyin, sonra geri alın.
```

## 🎓 Kabul kriterleri

- [ ] `config.h` tüm pinleri, eşikleri ve zamanlama sabitlerini içeriyor
- [ ] Hiçbir pin numarası `config.h` dışında geçmiyor
- [ ] `RELAY_ON` / `RELAY_OFF` sabitleri tanımlı ve aktif-LOW mantığı yorumlanmış
- [ ] `LED_PWM_BEGIN()` / `LED_PWM_WRITE()` makroları her iki core sürümü için yazıldı
- [ ] `static_assert` kontrolleri eklendi ve derleniyor
- [ ] `secrets.h` `.gitignore`'da, `secrets.h.example` repoda
- [ ] `--warnings all` ile uyarısız derleniyor

## ⚠️ Bilinen tuzaklar

**`constexpr uint8_t` yerine `#define` kullanmak** yaygın Arduino alışkanlığıdır ama tip kontrolü yapılmadığı için `digitalWrite(SOIL_PIN, HIGH)` gibi anlamsız çağrılar derlenip geçer. `constexpr` bunu engellemez ama en azından hata mesajlarını okunabilir kılar ve hata ayıklayıcıda sembol olarak görünür.

**`static_assert` içinde float karşılaştırması** bazı derleyicilerde sorun çıkarabilir; `HUM_LOW_THRESHOLD < HUM_HIGH_THRESHOLD` gibi basit karşılaştırmalar sorunsuzdur, karmaşık ifadelerden kaçının.

**Claude Code core sürümünü varsayabilir.** Faz 0'da not aldığınız sürümü prompt'ta açıkça belirtin; "core 3.0.7 kullanıyoruz" demek, yanlış API seçilmesini baştan engeller.
