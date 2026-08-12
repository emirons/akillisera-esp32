# Faz 2 — Saf Kontrol Mantığı (Test Odaklı Geliştirme)

## 🎯 Amaç

Projenin **beyni** olan karar mantığını, hiçbir Arduino bağımlılığı olmadan yazmak ve bilgisayarda birim testleriyle doğrulamak. Sulama kararı, fan histerezisi, LED parlaklığı ve alarm koşulu buraya girer. Bu faz, tüm planın en değerli parçasıdır: donanım olmadan mantığın doğruluğu kanıtlanır.

## 📋 Ön koşul

Faz 1 tamamlanmış, `config.h` hazır.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| **`superpowers:test-driven-development`** | Bu fazın omurgası — zorunlu |
| `engineering:testing-strategy` | Faz başında, neyin test edilebilir olduğunu belirlemek |
| `superpowers:requesting-code-review` | Faz sonunda |
| **Bash** | `g++` ile test derleme |

## 📁 Dosyalar

`akillisera/control_logic.h` (yeni) · `tests/test_control_logic.cpp` (yeniden yazılır)

---

## 💬 PROMPT 2.1 — TDD ile kontrol mantığı

```text
superpowers:test-driven-development skill'ini kullan. Bu fazda ÖNCE test yazacaksın,
sonra implementasyonu. Test yazmadan implementasyon yazma.

HEDEF: akillisera/control_logic.h — projenin tüm karar mantığı. Bu dosya Arduino.h
dahil HİÇBİR Arduino başlığını include etmeyecek; yalnızca <cstdint> ve <cmath>
kullanacak. Böylece bilgisayarda g++ ile derlenip test edilebilecek.

TASARIM: Durumu tutan bir struct ve onu ilerleten saf fonksiyonlar. Global değişken
YOK, donanım çağrısı YOK. Fonksiyonlar girdi alır, çıktı döndürür.

struct SeraDurumu {
  // Sensör girdileri
  float sicaklik;      // C
  float nem;           // %
  int   toprakNem;     // ham ADC, 0-4095
  int   isikSeviyesi;  // ham ADC, 0-4095
  // Eyleyici çıktıları
  bool  pompaAcik;
  bool  fanAcik;
  int   ledParlaklik;  // 0-255
  bool  alarmAktif;
  // İç durum
  unsigned long pompaBaslangic;
  bool  manuelFan;
  bool  manuelLed;
};

YAZILACAK FONKSİYONLAR:

1. bool sulamaGerekli(int toprakNem, bool pompaAcik)
   toprakNem > SOIL_DRY_THRESHOLD ve pompa zaten açık değilse true.

2. bool pompaSuresiDoldu(unsigned long simdi, unsigned long baslangic)
   simdi - baslangic >= WATERING_DURATION ise true.
   ÖNEMLİ: millis() taşması (49.7 gün sonra 0'a döner) durumunda da doğru
   çalışmalı. Çıkarma işlemi unsigned aritmetikte doğal olarak taşmayı tolere
   eder — bunu test et.

3. bool fanKarari(float nem, float sicaklik, bool oncekiDurum)
   Histerezisli karar:
   - nem > HUM_HIGH_THRESHOLD  VEYA sicaklik > TEMP_HIGH_THRESHOLD -> true
   - nem < HUM_LOW_THRESHOLD   VE   sicaklik < TEMP_HIGH_THRESHOLD -> false
   - ikisi de değilse oncekiDurum korunur (histerezis bandı içindeyiz)

4. int ledParlaklikKarari(int isikSeviyesi, bool manuelMod, int manuelDeger)
   manuelMod true ise manuelDeger'i 0-255'e kırparak döndür.
   Değilse: isikSeviyesi < LIGHT_LOW_THRESHOLD ? LED_BRIGHTNESS : 0

5. bool alarmKarari(float sicaklik)
   sicaklik > TEMP_HIGH_THRESHOLD

6. bool sensorDegeriGecerli(float deger)
   NaN veya sonsuz değilse ve -40..80 aralığındaysa true.
   (DHT11 arıza durumunda NaN döndürür; bu kontrol her okumada kullanılacak.)

7. enum class Komut { YOK, SULA, FAN_AC, FAN_KAPA, OTO, LED_AYARLA };
   struct KomutSonucu { Komut komut; int deger; };
   KomutSonucu komutAyristir(const char* metin)
   TalkBack ve web arayüzünden gelen metinleri ayrıştırır:
   "SULA" -> {SULA, 0}
   "FAN_AC" -> {FAN_AC, 0}
   "FAN_KAPA" -> {FAN_KAPA, 0}
   "OTO" -> {OTO, 0}
   "LED_150" -> {LED_AYARLA, 150}
   "LED_999" -> {LED_AYARLA, 255}   (kırpılır)
   "LED_-5" -> {LED_AYARLA, 0}      (kırpılır)
   "SAÇMA" / "" / nullptr -> {YOK, 0}

TEST KAPSAMI — her fonksiyon için en az şu durumlar:
- Eşiğin tam üstü, tam altı, tam üstünde (sınır değerler)
- Histerezis için: bant içinde yukarı çıkış ve aşağı iniş ayrı ayrı
- Pompa süresi için: normal durum + millis() taşması durumu
- komutAyristir için: geçerli, geçersiz, boş, nullptr, sınır dışı sayı
- sensorDegeriGecerli için: NaN, sonsuz, makul değer, uç değer

ÇALIŞMA SIRASI (TDD):
1. tests/test_control_logic.cpp'ye YALNIZCA testleri yaz. Derle -> derlenmemeli.
2. control_logic.h'a fonksiyon imzalarını boş gövdelerle ekle. Derle -> testler
   KALMALI (kırmızı). Bunu bana kanıtla.
3. Fonksiyonları tek tek implemente et; her birinden sonra testleri çalıştır.
4. Hepsi yeşil olunca bana tam test çıktısını göster.

control_logic.h config.h'ı include edecek; ama config.h Arduino tiplerine
(uint8_t) bağımlı. Host'ta derlenebilmesi için config.h'ın en üstüne
#include <cstdint> ekle ve pin tanımlarını Arduino'ya bağımlı olmayan hale getir.
LOW/HIGH sabitleri host'ta tanımsız olacağı için RELAY_ON/RELAY_OFF tanımlarını
#ifdef ARDUINO koruması içine al.

Test derleme komutu:
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
```

---

## 💬 PROMPT 2.2 — Kod incelemesi

```text
superpowers:requesting-code-review skill'ini kullanarak control_logic.h ve
testlerini incelet. İnceleyende özellikle şunlara bakılsın:

- Histerezis mantığı gerçekten çatırdamayı (chattering) önlüyor mu? Eşiklerin
  tam sınırında salınım olabilir mi?
- unsigned long taşması gerçekten doğru ele alınmış mı, yoksa test mi yanlış?
- komutAyristir tampon taşmasına (buffer overrun) açık mı? Gelen metin
  sonlandırılmamışsa ne olur?
- Test kapsamında atlanan bir sınır durumu var mı?

İnceleme bulgularını bana özetle, sonra superpowers:receiving-code-review
skill'ine göre her bulguyu değerlendir — körü körüne uygulama, teknik olarak
gerekçesiz bulunanları gerekçesiyle reddet.
```

---

## ✅ Doğrulama komutları

```bash
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
```

Beklenen test çıktısı: tüm assertion'lar geçmiş, `0 failed`.

## 🎓 Kabul kriterleri

- [ ] `control_logic.h` hiçbir Arduino başlığı include etmiyor
- [ ] 7 fonksiyonun tamamı yazıldı ve testleri var
- [ ] Testler önce kırmızıydı, sonra yeşile döndü (TDD sırası kanıtlandı)
- [ ] `millis()` taşma senaryosu test ediliyor
- [ ] Histerezisin her iki yönü ayrı ayrı test ediliyor
- [ ] `komutAyristir` geçersiz/boş/nullptr girdilerle test ediliyor
- [ ] Kod incelemesi yapıldı, bulgular değerlendirildi
- [ ] Hem host testleri hem ESP32 derlemesi yeşil

## ⚠️ Bilinen tuzaklar

**Claude Code testleri implementasyondan sonra yazmaya meyillidir.** Prompt'ta "testler kırmızıyken bana kanıtla" demek bunu engeller. Kırmızı adımı atlanırsa test, kodun yaptığı şeyi doğrular; kodun *doğru* şeyi yaptığını değil.

**`millis()` taşması testi yanlış kurulabilir.** Doğru test: `baslangic = 0xFFFFFFF0`, `simdi = 0x00000010` verildiğinde geçen süre 32 ms olmalıdır. `unsigned long` aritmetiğinde `simdi - baslangic` bunu doğal olarak verir; test bunu doğrulamalı.

**Histerezis fonksiyonunun `oncekiDurum` parametresi unutulursa** fonksiyon saf olmaktan çıkıp global duruma bağımlı hale gelir ve test edilemez. Bu imza tasarımı bilinçlidir.
