# Faz 0 — Ortam Kurulumu & Proje İskeleti

## 🎯 Amaç

Claude Code'un kod yazmadan **önce** kendi kendini doğrulayabileceği altyapıyı kurmak: derleyici, kütüphaneler, test çatısı, Wokwi, proje hafızası ve otomatik derleme hook'u. Faz sonunda boş ama derlenen bir proje iskeleti olacak.

## 📋 Ön koşul

Yok — ilk faz.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `skill-creator` | Projeye özel firmware kuralları skill'i üretmek |
| `productivity:memory-management` | `CLAUDE.md` yapısını kurmak |
| **Bash** | Kurulum komutları |
| **GitHub MCP** *(opsiyonel)* | Repo oluşturma, ilk commit |

## 📁 Oluşturulacak dosyalar

`akillisera/akillisera.ino` · `CLAUDE.md` · `.gitignore` · `.claude/settings.json` · `.claude/skills/esp32-firmware-kurallari/SKILL.md` · `tests/doctest.h` · `wokwi/`

---

## 🔧 Adım 0.1 — Elle yapılacak kurulum (Claude Code'dan önce)

Bu komutları kendiniz çalıştırın; Claude Code'un `arduino-cli`'a ihtiyacı var.

```bash
# macOS
brew install arduino-cli

# Linux / WSL
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
export PATH=$PATH:~/bin

# Windows (PowerShell, yönetici)
winget install ArduinoSA.CLI
```

Doğrulama:

```bash
arduino-cli version
```

---

## 💬 PROMPT 0.1 — Ortam kurulumu ve doğrulama

```text
ESP32 tabanlı bir akıllı sera projesinin firmware'ini geliştireceğiz. Sen yalnızca
yazılım tarafından sorumlusun; donanım montajı başka biri tarafından yapılıyor.

İLK GÖREV: Geliştirme ortamını kur ve her adımı komut çıktısıyla doğrula.

1. `arduino-cli version` ile CLI'ın kurulu olduğunu doğrula. Kurulu değilse bana
   işletim sistemime uygun kurulum komutunu söyle ve dur.

2. ESP32 kart paketini kur:
   arduino-cli config init
   arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
   arduino-cli core update-index
   arduino-cli core install esp32:esp32

3. Kurulan core sürümünü `arduino-cli core list` ile yazdır ve bana SÜRÜM NUMARASINI
   açıkça bildir. Bu kritik: ESP32 core 3.x, LEDC PWM API'sini değiştirdi
   (ledcSetup/ledcAttachPin kaldırıldı, yerine ledcAttach geldi). Hangi sürümde
   olduğumuzu bilmem gerekiyor.

4. Gerekli kütüphaneleri kur ve sürümlerini yazdır:
   arduino-cli lib install "DHT sensor library"
   arduino-cli lib install "Adafruit Unified Sensor"
   arduino-cli lib install "LiquidCrystal I2C"
   arduino-cli lib install "ArduinoJson"

5. Kullanacağımız kartın tam FQBN'ini bul:
   arduino-cli board listall | grep -i "nodemcu\|esp32 dev"
   Muhtemel değer: esp32:esp32:nodemcu-32s

6. Aşağıdaki klasör yapısını oluştur ve akillisera/akillisera.ino içine yalnızca
   boş bir setup() ve loop() yaz:

   akillisera/akillisera.ino
   tests/
   wokwi/

7. Boş projeyi derle ve çıktıyı bana göster:
   arduino-cli compile --fqbn <bulduğun_fqbn> --warnings all akillisera/

Her adımın çıktısını göster. Bir adım başarısız olursa DURMA, hatayı analiz et ve
düzelt; düzeltemezsen bana sor. Hiçbir adımı "yaptım" diye geçme — komut çıktısı
olmadan başarı iddia etme.
```

---

## 💬 PROMPT 0.2 — Proje hafızası (CLAUDE.md)

```text
Şimdi projenin kalıcı hafızasını kur. Proje kökünde CLAUDE.md dosyası oluştur.
Bu dosyayı her oturumda otomatik okuyacaksın, o yüzden burada yazan kurallar
tüm fazlarda geçerli olacak.

CLAUDE.md şunları içermeli:

## Proje
ESP32 IoT Akıllı Sera & İklim Kontrol İstasyonu. Firmware'in tamamı benim
sorumluluğumda (Kişi B). Donanım montajı Kişi A'da; ben devreye hiç dokunmam,
yalnızca pin eşleme tablosuna göre kod yazarım.

## Derleme komutu (her değişiklikten sonra çalıştırılır)
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --warnings all akillisera/

## Host testleri
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests

## Pin haritası (DEĞİŞTİRİLEMEZ — Kişi A ile yapılan arayüz sözleşmesi)
| Bileşen | Pin | Tip |
| Kapasitif toprak nem | GPIO34 | Analog giriş (ADC1) |
| DHT11 | GPIO4 | Dijital (one-wire) |
| LDR | GPIO35 | Analog giriş (ADC1) |
| LCD SDA / SCL | GPIO21 / GPIO22 | I2C, adres 0x27 |
| Röle Ch1 — su pompası | GPIO5 | Dijital çıkış, AKTİF-LOW |
| Röle Ch2 — tahliye fanı | GPIO18 | Dijital çıkış, AKTİF-LOW |
| Şerit LED (MOSFET gate) | GPIO25 | LEDC PWM, 5 kHz, 8 bit |
| Buzzer | GPIO19 | Dijital çıkış |
| Yeşil LED / Kırmızı LED | GPIO2 / GPIO15 | Dijital çıkış |
| Buton 1 (sulama) / Buton 2 (sayfa) | GPIO12 / GPIO14 | INPUT_PULLUP |

## Eşik değerleri
SOIL_DRY_THRESHOLD = 2200 (üstünde toprak kuru -> sula)
LIGHT_LOW_THRESHOLD = 1500 (altında karanlık -> LED yak)
TEMP_HIGH_THRESHOLD = 35.0 C
HUM_HIGH_THRESHOLD = 70.0 % (üstünde fan aç)
HUM_LOW_THRESHOLD = 60.0 % (altında fan kapat — histerezis)
WATERING_DURATION = 5000 ms
LED_BRIGHTNESS = 200 (0-255)
CLOUD_INTERVAL = 30000 ms, TALKBACK_INTERVAL = 30000 ms

## Mutlak kurallar
1. loop() içinde ASLA delay() kullanma. Tüm zamanlama millis() tabanlı.
   Tek istisna: setup() içindeki bir defalık bekleme.
2. Röleler AKTİF-LOW: digitalWrite(pin, LOW) röleyi AÇAR.
3. Wi-Fi açıkken yalnızca ADC1 pinleri (GPIO32-39) analog okunabilir.
4. Tüm pin ve eşik sabitleri yalnızca config.h içinde tanımlanır. Başka hiçbir
   dosyada çıplak sayı (magic number) olmayacak.
5. Saf mantık fonksiyonları control_logic.h içinde, Arduino başlık dosyalarına
   bağımlı OLMADAN yazılır ki host'ta test edilebilsin.
6. Wi-Fi parolası ve API anahtarları yalnızca secrets.h içinde; bu dosya
   .gitignore'da olacak. secrets.h.example şablonu repoda kalır.
7. Web arayüzü HTML/CSS/JS'i PROGMEM içinde tutulur (RAM tasarrufu).
8. Her fazın sonunda derleme + testler yeşil olmadan "tamamlandı" deme.

Ayrıca .gitignore oluştur: secrets.h, build/, *.bin, *.elf, .DS_Store
```

---

## 💬 PROMPT 0.3 — Projeye özel skill ve otomatik derleme hook'u

```text
İki otomasyon kur:

A) skill-creator skill'ini kullanarak `.claude/skills/esp32-firmware-kurallari/`
   altında bir skill oluştur. Bu skill, ESP32 Arduino firmware'i yazarken
   uyulacak kuralları içersin:
   - millis() tabanlı bloklamayan zamanlama kalıbı (örnek kodla)
   - aktif-LOW röle sürme kalıbı
   - ESP32 core 2.x ve 3.x LEDC API farkı ve sürüm korumalı makro çözümü
   - ADC1/ADC2 + Wi-Fi çakışması
   - DHT11'in NaN dönebileceği ve her okumanın isnan() ile kontrol edilmesi
   - PROGMEM'den HTML servis etme kalıbı
   - Seri monitöre yazarken F() makrosu ile flash string kullanımı
   Skill'in description alanı, ESP32/Arduino/firmware kelimeleriyle tetiklenecek
   şekilde yazılsın.

B) `.claude/settings.json` içine bir PostToolUse hook'u ekle. Bu hook, akillisera/
   klasöründeki bir .ino veya .h dosyası her düzenlendiğinde otomatik derleme
   çalıştırsın ve hataları bana göstersin:

   {
     "hooks": {
       "PostToolUse": [
         {
           "matcher": "Edit|Write",
           "hooks": [
             {
               "type": "command",
               "command": "arduino-cli compile --fqbn esp32:esp32:nodemcu-32s akillisera/ 2>&1 | tail -25"
             }
           ]
         }
       ]
     }
   }

   Hook'u kurduktan sonra akillisera.ino'ya kasıtlı bir sözdizimi hatası ekle
   (örneğin bir noktalı virgülü sil), hook'un hatayı yakaladığını göster, sonra
   hatayı geri al.
```

---

## 💬 PROMPT 0.4 — Test çatısı ve Wokwi

```text
1. tests/ klasörüne doctest'in tek başlıklı sürümünü indir:
   curl -sL https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h -o tests/doctest.h

   İndirme başarısız olursa bana söyle; alternatif olarak bağımlılıksız, assert
   tabanlı minik bir test koşucusu yazarız.

2. tests/test_control_logic.cpp içine, henüz var olmayan control_logic.h'ı
   include eden ve tek bir "placeholder" testi bulunan bir dosya yaz. Şu an
   derlenmemesi normal — Faz 2'de control_logic.h'ı yazınca derlenecek.
   Test derleme komutunu bir yorum satırı olarak dosyanın başına ekle.

3. wokwi/diagram.json dosyasını oluştur. Sanal devrede şunlar olsun:
   - wokwi-esp32-devkit-v1
   - wokwi-dht22 (DHT11 yerine; Wokwi DHT11'i de destekler, hangisini
     kullandığını bana bildir) -> GPIO4
   - wokwi-lcd1602 (I2C modunda, SDA=GPIO21, SCL=GPIO22, adres 0x27)
   - iki adet wokwi-potentiometer -> GPIO34 (toprak nem) ve GPIO35 (LDR)
     (gerçek sensörlerin analog çıkışını taklit etmek için)
   - iki adet wokwi-pushbutton -> GPIO12 ve GPIO14 (INPUT_PULLUP, GND'ye)
   - üç adet wokwi-led -> GPIO2 (yeşil), GPIO15 (kırmızı), GPIO25 (şerit LED
     temsili), her biri 220 ohm direnç ile
   - wokwi-buzzer -> GPIO19
   - röleler yerine iki LED -> GPIO5 (pompa temsili), GPIO18 (fan temsili)

4. wokwi/wokwi.toml oluştur:
   [wokwi]
   version = 1
   firmware = '../build/akillisera.ino.bin'
   elf = '../build/akillisera.ino.elf'

5. Derleme komutunu --output-dir build ekleyerek güncelle ve CLAUDE.md'deki
   derleme komutunu da bu şekilde düzelt:
   arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
```

---

## ✅ Doğrulama komutları

```bash
arduino-cli version
arduino-cli core list                 # esp32:esp32 görünmeli, sürümü not al
arduino-cli lib list                  # 4 kütüphane görünmeli
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
ls build/akillisera.ino.bin           # dosya oluşmuş olmalı
ls .claude/skills/esp32-firmware-kurallari/SKILL.md
```

## 🎓 Kabul kriterleri

- [ ] `arduino-cli core list` çıktısında ESP32 core sürümü görülüyor ve **not alındı**
- [ ] Boş proje uyarısız derleniyor, `build/akillisera.ino.bin` üretiliyor
- [ ] `CLAUDE.md` pin tablosu, eşikler ve 8 mutlak kuralı içeriyor
- [ ] `.gitignore` içinde `secrets.h` var
- [ ] PostToolUse hook'u kasıtlı bir sözdizimi hatasını yakaladı
- [ ] `tests/doctest.h` indirildi
- [ ] `wokwi/diagram.json` ve `wokwi.toml` oluşturuldu
- [ ] İlk git commit atıldı

## ⚠️ Bilinen tuzaklar

**FQBN yanlış seçilirse** derleme başarılı olur ama karta yüklenen firmware çalışmaz. NodeMCU-32S ve ESP32 Dev Module farklı flash ayarlarına sahiptir. Kartın üzerindeki yazıyı Kişi A'ya sorup doğrulayın.

**`LiquidCrystal I2C` kütüphanesinin birden fazla sürümü var.** `arduino-cli lib search "LiquidCrystal I2C"` çıktısında birkaç seçenek çıkar; `Frank de Brabander` sürümü ESP32 ile en uyumlusudur. Claude Code yanlış olanı kurarsa `lcd.init()` derlenmez, `lcd.begin()` ister.

**Hook her düzenlemede derleme tetikler**; büyük dosyalarda bu yavaşlatabilir. Rahatsız ederse `matcher` alanını yalnızca `Write` olarak daraltın.
