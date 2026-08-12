# 🤖 Kişi B Firmware Geliştirme Planı — Claude Code ile

> **Kapsam:** `proje_detay_raporu.md` içindeki **Kişi B** sorumluluğundaki tüm yazılım işleri.
> Kişi A'nın donanım işleri bu planın dışındadır; bu plan yalnızca kod üretir.
>
> **Karar verilen kurulum:** `arduino-cli` ile kendi kendine derleme · modüler `.ino + .h` yapısı · Wokwi simülasyonu + host birim testleri.

---

## 1. Temel Çalışma Prensibi

Claude Code'un gömülü sistemlerde en büyük zaafı **geri bildirim alamamasıdır**: kodu yazar ama çalışıp çalışmadığını göremez. Bu planın tamamı bu zaafı kapatmak üzerine kuruludur. Üç katmanlı bir doğrulama zinciri kurulur:

| Katman | Ne doğrular | Komut | Hız |
|:---|:---|:---|:---|
| **1. Derleme** | Sözdizimi, tip hataları, eksik kütüphane | `arduino-cli compile` | ~15 sn |
| **2. Host birim testi** | Saf mantık: histerezis, eşik, debounce, komut ayrıştırma | `g++ … && ./tests` | ~2 sn |
| **3. Wokwi simülasyonu** | Gerçek davranış: LCD çıktısı, Wi-Fi, HTTP, ThingSpeak | Wokwi VS Code eklentisi | ~30 sn |

**Kural:** Claude Code hiçbir fazı, o fazın doğrulama komutu yeşil olmadan "bitti" diye işaretlemez. Bu kuralı `superpowers:verification-before-completion` skill'i zorunlu kılar.

---

## 2. Faz Haritası

| # | Faz | Dosya | Süre | Çıktı |
|:--|:---|:---|:---|:---|
| 0 | Ortam kurulumu & proje iskeleti | `01_FAZ0_ortam_kurulumu.md` | ~40 dk | Derlenen boş proje, CLAUDE.md, hook |
| 1 | `config.h` — tek doğruluk kaynağı | `02_FAZ1_config_ve_iskelet.md` | ~25 dk | Pin & eşik tanımları, sürüm koruması |
| 2 | Saf kontrol mantığı (TDD) | `03_FAZ2_kontrol_mantigi_tdd.md` | ~60 dk | `control_logic.h` + geçen testler |
| 3 | Sensör katmanı | `04_FAZ3_sensor_katmani.md` | ~40 dk | `sensors.h`, medyan filtre, hata yönetimi |
| 4 | Eyleyici katmanı | `05_FAZ4_eyleyici_katmani.md` | ~40 dk | `actuators.h`, LEDC PWM, bloklamayan pompa |
| 5 | LCD & butonlar | `06_FAZ5_lcd_ve_butonlar.md` | ~45 dk | `display.h`, `buttons.h`, 3 sayfa, debounce |
| 6 | Wi-Fi, mDNS, Web Server | `07_FAZ6_wifi_mdns_webserver.md` | ~50 dk | `network.h`, JSON API, yeniden bağlanma |
| 7 | Mobil Web App (PWA) | `08_FAZ7_mobil_web_app.md` | ~60 dk | `web_ui.h` (PROGMEM), slider, canlı gösterge |
| 8 | ThingSpeak, NTP, TalkBack | `09_FAZ8_cloud_thingspeak_talkback.md` | ~50 dk | `cloud.h`, 30 sn zamanlayıcılar, komut işleyici |
| 9 | Entegrasyon, review, teslim | `10_FAZ9_entegrasyon_ve_teslim.md` | ~60 dk | Wokwi testi, güvenlik taraması, lab raporu |

**Toplam:** ~8 saat aktif çalışma. Fazlar sırayla yürütülmelidir; tek istisna Faz 7 ile Faz 8'in paralel çalıştırılabilmesidir (bkz. Faz 8, "Paralel yürütme" bölümü).

---

## 3. Kullanılacak Skill / Plugin / MCP Haritası

### Skill'ler

| Skill | Hangi fazda | Ne için |
|:---|:---|:---|
| `superpowers:brainstorming` | Faz 0 öncesi | Belirsiz kalan tasarım kararlarını netleştirmek |
| `superpowers:writing-plans` | Faz 0 | Bu planı repoya özel `plan.md`'ye dönüştürmek |
| `superpowers:test-driven-development` | **Faz 2** | Kontrol mantığını önce test yazarak geliştirmek |
| `superpowers:systematic-debugging` | Her fazda hata çıkınca | Tahmin yerine sistematik izolasyon |
| `superpowers:verification-before-completion` | **Her fazın sonu** | "Bitti" demeden önce komut çıktısıyla kanıt |
| `superpowers:requesting-code-review` | Faz 2, 6, 9 | Kritik modüllerde bağımsız gözle inceleme |
| `superpowers:using-git-worktrees` | Faz 7 & 8 paralel | İki fazı izole çalışma alanlarında yürütmek |
| `superpowers:subagent-driven-development` | Faz 7 & 8 | Web App ve Cloud modüllerini eşzamanlı yazdırmak |
| `engineering:code-review` | Faz 9 | Güvenlik, performans, sınır durumları |
| `engineering:debug` | Gerektiğinde | Stack trace / panic çözümlemesi |
| `engineering:testing-strategy` | Faz 2 başında | Neyin test edilebilir olduğunu belirlemek |
| `engineering:documentation` | Faz 9 | README ve kod içi dokümantasyon |
| `design:ux-copy` | Faz 7 | Web arayüzündeki Türkçe metinler, buton adları |
| `design:accessibility-review` | Faz 7 | Dokunma hedefi boyutu, kontrast (telefonda kullanılacak) |
| `vibesec` | **Faz 6 & 9** | Web server'daki girdi doğrulama açıkları |
| `skill-creator` | Faz 0 | Projeye özel firmware kuralları skill'i üretmek |
| `productivity:memory-management` | Faz 0 | `CLAUDE.md` kalıcı proje hafızası |
| `anthropic-skills:docx` / `pdf` | Faz 9 | Bahçeşehir Exp#6 lab raporunu üretmek |

### MCP / Araçlar

| Araç | Ne için | Not |
|:---|:---|:---|
| **Bash** | `arduino-cli`, `g++`, `git`, `curl` | Planın belkemiği |
| **GitHub MCP** | Repo, commit, PR, faz başına branch | Yetkilendirme gerekir |
| **Chrome MCP** (`claude-in-chrome`) | Wokwi'yi açma, web arayüzünü tarayıcıda test etme, ThingSpeak grafiğini doğrulama | Faz 7, 8, 9'da kritik |
| **WebFetch / WebSearch** | Kütüphane API dokümanı, ESP32 core sürüm notları | Faz 1'deki LEDC sürüm farkı için |
| **Wokwi VS Code eklentisi** | Sanal ESP32 + DHT11 + LCD + Wi-Fi | Kişi A'yı beklemeden test |

---

## 4. Proje Dosya Yapısı (Faz 0 sonunda oluşacak)

```
akillisera/
├── akillisera.ino          # setup() + loop(), yalnızca orkestrasyon
├── config.h                # TÜM pin ve eşik tanımları (tek doğruluk kaynağı)
├── control_logic.h         # Saf mantık — Arduino bağımlılığı YOK, host'ta test edilir
├── sensors.h               # DHT11, toprak nem, LDR okuma + filtreleme
├── actuators.h             # Röle, LEDC PWM, buzzer, bloklamayan pompa
├── display.h               # 16x2 I2C LCD, 3 sayfa
├── buttons.h               # Debounce'lu buton okuma
├── network.h               # Wi-Fi, mDNS, WebServer, JSON API
├── web_ui.h                # PROGMEM içindeki HTML/CSS/JS (PWA)
├── cloud.h                 # NTP, ThingSpeak, TalkBack
└── secrets.h               # Wi-Fi ve API anahtarları — .gitignore'da!

tests/
├── test_control_logic.cpp  # Host birim testleri
└── doctest.h               # Tek başlıklı test çatısı

wokwi/
├── diagram.json            # Sanal devre şeması
└── wokwi.toml              # Firmware yolu

.claude/
├── settings.json           # PostToolUse derleme hook'u
└── skills/
    └── esp32-firmware-kurallari/SKILL.md

CLAUDE.md                   # Proje hafızası — Claude Code her oturumda okur
.gitignore
```

**Neden `.h` dosyaları, `.cpp` değil?** Arduino IDE bir klasördeki tüm `.cpp` dosyalarını ayrı derleme birimi olarak işler ve bu, `#include` sırası hassas olan Arduino projelerinde bağlantı (linker) hatalarına yol açar. Tüm implementasyonu `.h` içinde `inline` fonksiyonlar olarak tutmak, tek derleme birimi (unity build) davranışını korur ve hem `arduino-cli` hem Arduino IDE ile sorunsuz derlenir.

---

## 5. Her Fazın Ortak Şablonu

Her faz dosyası şu bölümleri içerir:

1. **🎯 Amaç** — faz sonunda neyin çalışıyor olacağı
2. **📋 Ön koşul** — hangi fazın bitmiş olması gerektiği
3. **🧩 Kullanılacak skill / MCP**
4. **📁 Dokunulacak dosyalar**
5. **💬 PROMPT** — Claude Code'a olduğu gibi yapıştırılacak metin
6. **✅ Doğrulama komutları** — çalıştırılıp yeşil görülmesi gereken komutlar
7. **🎓 Kabul kriterleri** — işaretlenecek kontrol listesi
8. **⚠️ Bilinen tuzaklar** — bu fazda sık yapılan hatalar

---

## 6. Başlamadan Önce: Kritik Teknik Uyarılar

Bu üç madde, plan boyunca tekrar tekrar karşınıza çıkacak ve baştan bilinmezse saatler kaybettirir.

### ⚠️ ESP32 Arduino Core 3.x, LEDC API'sini değiştirdi

Rapordaki kod `ledcSetup()` + `ledcAttachPin()` kullanıyor. Bu fonksiyonlar **core 3.0 ile kaldırıldı**. Core 3.x'te yerine `ledcAttach(pin, freq, resolution)` gelir ve `ledcWrite()` artık kanal değil **pin** numarası alır. Faz 1'de her iki sürümde de derlenen bir makro katmanı yazılacak.

### ⚠️ Wi-Fi açıkken ADC2 pinleri okunamaz

ESP32'de ADC2 birimi Wi-Fi radyosuyla paylaşımlıdır; Wi-Fi aktifken `analogRead()` ADC2 pinlerinde **0 veya çöp değer** döndürür. Projedeki `GPIO34` (ADC1_CH6) ve `GPIO35` (ADC1_CH7) **ADC1'e bağlıdır**, dolayısıyla güvenlidir. Pin değiştirme ihtiyacı doğarsa yalnızca ADC1 pinleri (GPIO32-39) seçilmelidir.

### ⚠️ `delay()` kullanımı projeyi bitirir

Web server, mDNS ve ThingSpeak zamanlayıcıları `loop()`'un akmasına bağlıdır. Tek bir `delay(1000)` bile web arayüzünü yanıt veremez hale getirir. Tüm zamanlama `millis()` tabanlı olacak; bu kural `CLAUDE.md`'ye ve projeye özel skill'e yazılacak ki Claude Code her oturumda hatırlasın.

---

## 7. Önerilen Oturum Düzeni

Her fazı **ayrı bir Claude Code oturumunda** çalıştırın. Faz sonunda:

```bash
git add -A && git commit -m "Faz N: <kısa açıklama>"
```

Bu, bir faz bozulduğunda tüm projeyi değil yalnızca o fazı geri almanızı sağlar. Oturum başında Claude Code `CLAUDE.md`'yi otomatik okuduğu için önceki fazların kararlarını hatırlar; uzun tek bir oturumda bağlam dolup kararlar unutulur.

**Sonraki adım:** `01_FAZ0_ortam_kurulumu.md` dosyasını açın.

---

## 8. Doğrulanmış Teknik Kaynaklar

Plandaki kritik teknik iddialar aşağıdaki kaynaklardan doğrulanmıştır:

* **ESP32 core 2.x → 3.0 LEDC API değişikliği** — `ledcSetup()` ve `ledcAttachPin()` kaldırıldı, yerine `ledcAttach(pin, freq, resolution)` geldi; `ledcWrite()` artık kanal değil pin alıyor.
  [Espressif resmî geçiş kılavuzu](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html) · [Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-migrating-version-2-to-3-arduino/)
* **Wokwi Wi-Fi simülasyonu** — `Wokwi-GUEST` parolasız erişim noktası; Public Gateway internete erişir ama yerel ağa erişmez, yerel ağ için Private Gateway gerekir.
  [Wokwi ESP32 WiFi dokümanı](https://docs.wokwi.com/guides/esp32-wifi)
* **ThingSpeak TalkBack** — komut kuyruğu, `commands/execute` sıradaki komutu döndürür ve siler; ücretsiz hesapta günde ~8000 API isteği sınırı.
  [MathWorks TalkBack + ESP32 örneği](https://www.mathworks.com/help/thingspeak/control-a-light-with-talkback-and-esp32.html)

