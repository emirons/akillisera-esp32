# Faz 4 — Eyleyici Katmanı

## 🎯 Amaç

Pompa, fan, şerit LED, buzzer ve durum LED'lerini süren katmanı yazmak. Kritik nokta: **pompanın 5 saniyelik çalışması `delay()` ile değil, durum makinesiyle** gerçeklenecek — aksi halde web sunucusu 5 saniye boyunca donar.

## 📋 Ön koşul

Faz 3 tamamlanmış.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `.claude/skills/esp32-firmware-kurallari` | Otomatik (aktif-LOW, LEDC, millis kalıpları) |
| `superpowers:test-driven-development` | Pompa durum makinesi mantığı için |

## 📁 Dosyalar

`akillisera/actuators.h` (yeni) · `akillisera/control_logic.h` (genişletilir) · `tests/test_control_logic.cpp` (genişletilir)

---

## 💬 PROMPT 4.1 — actuators.h

```text
akillisera/actuators.h yaz. Tüm digitalWrite / LED_PWM_WRITE çağrıları burada
olacak; başka dosyada pin sürme kodu bulunmayacak.

1) BAŞLATMA
   inline void eyleyicileriBaslat()
   - RELAY_PUMP, RELAY_FAN, BUZZER_PIN, LED_GREEN, LED_RED pinlerini OUTPUT yap
   - KRİTİK SIRA: pinMode'dan HEMEN SONRA röleleri RELAY_OFF'a çek. Ters sırada
     yaparsan ESP32 açılışında pompa bir an çalışır. Bu sırayı yorumla belirt.
   - LED_PWM_BEGIN() çağır ve LED_PWM_WRITE(0) ile karanlıkta başlat

2) POMPA — BLOKLAMAYAN DURUM MAKİNESİ
   Bu bölüm fazın en kritik parçası. delay() KESİNLİKLE kullanılmayacak.

   struct PompaDurumu { bool acik; unsigned long baslangic; };
   static PompaDurumu pompa = {false, 0};

   inline void pompayiTetikle(unsigned long simdi)
   - zaten açıksa hiçbir şey yapma (tetiklemeyi sıfırlama — üst üste basılan
     butonun süreyi uzatmasını istemiyoruz; bu bilinçli bir karar, yorumla)
   - değilse: digitalWrite(RELAY_PUMP, RELAY_ON); pompa.acik = true;
     pompa.baslangic = simdi;

   inline void pompayiGuncelle(unsigned long simdi)
   - pompa açıksa ve control_logic.h'daki pompaSuresiDoldu(simdi, pompa.baslangic)
     true ise: digitalWrite(RELAY_PUMP, RELAY_OFF); pompa.acik = false;
   - Bu fonksiyon loop() içinde HER TURDA çağrılacak.

   inline bool pompaCalisiyor()

   GÜVENLİK KİLİDİ EKLE: Ardışık sulamalar arasında en az 60 saniye beklenmeli
   (config.h'a WATERING_COOLDOWN = 60000 ekle). Toprak nem sensörü arızalanıp
   sürekli "kuru" derse bitki boğulmasın. pompayiTetikle() bu kilidi kontrol
   etsin ve engellenen tetiklemeyi Serial'e yazsın.

3) FAN
   inline void fanAyarla(bool acik)
   digitalWrite(RELAY_FAN, acik ? RELAY_ON : RELAY_OFF);
   Durum değişmediyse pin yazma — gereksiz röle işlemi yok.
   static bool sonFanDurumu ile karşılaştır.

4) ŞERİT LED
   inline void ledParlaklikAyarla(int duty)
   - 0-255 arasına kırp
   - LED_PWM_WRITE(duty)
   - Değişmediyse yazma
   inline int ledMevcutParlaklik()

5) ALARM (buzzer + kırmızı LED)
   inline void alarmGuncelle(bool aktif, unsigned long simdi)
   - aktif false: buzzer kapalı, kırmızı LED sönük, yeşil LED yanık
   - aktif true : buzzer ve kırmızı LED 500 ms periyotla yanıp sönsün
     (millis() tabanlı, tone() veya digitalWrite ile — delay YOK),
     yeşil LED sönük
   Yanıp sönme durumunu static unsigned long sonDegisim ve static bool durum ile
   yönet.

6) TOPLU UYGULAMA
   inline void eyleyicileriUygula(const SeraDurumu& d, unsigned long simdi)
   control_logic.h'ın ürettiği karar struct'ını alıp donanıma yansıtır.
   Karar VERMEZ, yalnızca uygular. Karar control_logic.h'ın işi.

Yazdıktan sonra akillisera.ino'nun loop()'unu şu iskelete getir:

void loop() {
  unsigned long simdi = millis();
  pompayiGuncelle(simdi);          // her turda, gecikmesiz
  if (simdi - sonSensorOkuma >= SENSOR_READ_INTERVAL) { ... }
  // karar + uygulama
}

Derle ve göster.
```

---

## 💬 PROMPT 4.2 — Durum makinesi testleri

```text
control_logic.h'a pompa durum makinesinin saf mantığını taşı ve test et:

bool pompaTetiklenebilir(unsigned long simdi, unsigned long sonBitis,
                         bool suAnAcik)
- suAnAcik ise false
- simdi - sonBitis < WATERING_COOLDOWN ise false
- değilse true

TEST DURUMLARI:
- Pompa açıkken tetikleme reddediliyor
- Soğuma süresi dolmadan tetikleme reddediliyor (59 sn -> false, 61 sn -> true)
- Soğuma sınırı tam üzerinde (60000 ms -> true)
- millis() taşması sırasında soğuma hesabı doğru

Testleri çalıştır, çıktıyı göster. Sonra actuators.h'daki pompayiTetikle()
bu fonksiyonu kullanacak şekilde güncelle.
```

---

## ✅ Doğrulama komutları

```bash
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
grep -rn "delay(" akillisera/*.h akillisera/*.ino     # yalnızca setup() içinde veya hiç
grep -rn "digitalWrite" akillisera/ --include=*.h      # yalnızca actuators.h
```

## 🎓 Kabul kriterleri

- [ ] Pompa 5 saniyelik çalışmayı `delay()` olmadan, durum makinesiyle yapıyor
- [ ] `setup()` içinde röleler pinMode'dan hemen sonra kapatılıyor
- [ ] 60 saniyelik sulama soğuma kilidi var ve test edilmiş
- [ ] Fan ve LED yalnızca durum değiştiğinde pin yazıyor
- [ ] Alarm yanıp sönmesi `millis()` tabanlı
- [ ] `LED_PWM_WRITE` makrosu doğrudan kullanılıyor, ham `ledcWrite` yok
- [ ] Tüm `digitalWrite` çağrıları `actuators.h` içinde
- [ ] Host testleri ve ESP32 derlemesi yeşil

## ⚠️ Bilinen tuzaklar

**`pinMode(OUTPUT)` çağrısı pini LOW'a çeker.** Aktif-LOW rölelerde bu, röleyi **açar**. ESP32 her açılışta pompayı bir an çalıştırır ve bunu kimse fark etmez — ta ki toprak sürekli ıslanana kadar. Doğru çözüm: `digitalWrite(pin, HIGH)` çağrısını `pinMode`'dan **önce** yapmaktır; ESP32'de bu çıktı tamponunu önceden ayarlar.

**`tone()` fonksiyonu ESP32 core 3.x'te LEDC kanallarını kullanır** ve şerit LED'in PWM kanalıyla çakışabilir. Buzzer için `tone()` yerine basit `digitalWrite` ile kare dalga üretmek veya farklı bir LEDC kanalı atamak gerekir. Claude Code'a bu çakışmayı açıkça sorun.

**Pompa soğuma kilidi olmadan** sensör arızası bitkiyi öldürür. Bu, raporda "güvenlik kilidi" olarak anlatılabilecek, jüriye iyi görünen bir mühendislik detayıdır.
