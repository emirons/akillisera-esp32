# Faz 5 — LCD Ekran & Fiziksel Butonlar

## 🎯 Amaç

16x2 I2C LCD'de üç bilgi sayfası göstermek ve iki fiziksel butonu titreşimsiz (debounce) okumak. Bu, projenin donanım olmadan da kullanılabilir olduğunu gösteren yerel arayüz katmanı.

## 📋 Ön koşul

Faz 4 tamamlanmış.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `superpowers:test-driven-development` | Debounce mantığı için |
| `design:ux-copy` | 16 karaktere sığan Türkçe etiketler |
| **Wokwi** | LCD çıktısını görsel doğrulama |

## 📁 Dosyalar

`akillisera/display.h` · `akillisera/buttons.h` · `control_logic.h` (genişletilir)

---

## 💬 PROMPT 5.1 — buttons.h (TDD)

```text
superpowers:test-driven-development skill'ini kullan.

ÖNCE control_logic.h'a saf debounce mantığını ekle ve test et:

struct ButonDurumu {
  bool sonOkuma;          // pinin son ham okuması (INPUT_PULLUP: basılı = false)
  bool kararliDurum;      // debounce sonrası kararlı durum
  unsigned long sonDegisim;
};

bool butonGuncelle(ButonDurumu& d, bool hamOkuma, unsigned long simdi)
- hamOkuma sonOkuma'dan farklıysa sonDegisim = simdi, sonOkuma = hamOkuma
- simdi - sonDegisim >= BUTTON_DEBOUNCE_MS ve hamOkuma != kararliDurum ise
  kararliDurum = hamOkuma ve DEĞİŞİM OLDUĞUNU bildirmek için true dön
- diğer durumlarda false dön
Dönüş değeri "durum az önce değişti" anlamına gelir.

bool butonBasildi(const ButonDurumu& d)
INPUT_PULLUP olduğu için kararliDurum == false ise basılı demektir.
Bu ters mantığı fonksiyon içinde kapsülle ki çağıran taraf kafa karıştırmasın.

TEST DURUMLARI:
- Temiz basış: false->true geçişi 50 ms sonra bildiriliyor
- Titreşim: 10 ms içinde 5 kez değişen sinyal TEK bir geçiş üretiyor
- 49 ms'de değişim bildirilmiyor, 50 ms'de bildiriliyor (sınır)
- Basılı tutma: sürekli basılıyken tekrar tekrar true dönmüyor
- millis() taşması sırasında doğru çalışıyor

Testler yeşil olunca akillisera/buttons.h yaz:
inline void butonlariBaslat()  -> pinMode(BTN_WATER, INPUT_PULLUP), aynısı BTN_PAGE
inline bool sulamaButonunaBasildi(unsigned long simdi)   // yalnızca YENİ basışta true
inline bool sayfaButonunaBasildi(unsigned long simdi)
Bu iki fonksiyon içeride ButonDurumu tutar ve butonGuncelle() kullanır;
yalnızca basılı-hale-geçiş anında (rising edge değil, falling edge) true döner.
```

---

## 💬 PROMPT 5.2 — display.h

```text
akillisera/display.h yaz. 16x2 I2C LCD, adres 0x27, SDA=GPIO21, SCL=GPIO22.

enum class Sayfa : uint8_t { IKLIM = 0, TOPRAK = 1, SISTEM = 2, SAYI = 3 };

SAYFA İÇERİKLERİ — her satır TAM 16 karaktere sığmalı. Türkçe karakterler
LCD'nin karakter setinde YOKTUR; ç,ğ,ı,ö,ş,ü yerine c,g,i,o,s,u kullan.
Bunu kod içinde yorumla belirt.

Sayfa 1 — IKLIM:
  Satır 1: "Sic:23.4C Nem:65"
  Satır 2: "Fan:ACIK  LED:200"   (LED değeri 3 hane sığmazsa kısalt)

Sayfa 2 — TOPRAK:
  Satır 1: "Toprak: %42"
  Satır 2: "Pompa: KAPALI"  veya sulama sırasında "Sulaniyor 3s" (geri sayım)

Sayfa 3 — SISTEM:
  Satır 1: "WiFi:OK  IP son okteti" veya "WiFi:YOK"
  Satır 2: "akillisera.local" (kaydırmalı) veya saat "12:34:56" (NTP'den)

FONKSİYONLAR:
inline void ekraniBaslat()
  lcd.init(); lcd.backlight(); açılış mesajı 1.5 sn (setup içinde delay
  KABUL EDİLİR, bu tek istisna — yorumla belirt).

inline void sayfaDegistir()   // Sayfa'yı döngüsel ilerletir
inline Sayfa mevcutSayfa()

inline void ekraniGuncelle(const SeraDurumu& d, unsigned long simdi)
  - LCD_REFRESH_INTERVAL (500 ms) periyodunda çağrılır
  - KRİTİK: Her seferinde lcd.clear() ÇAĞIRMA. clear() ~2 ms sürer ve gözle
    görülür titreme yaratır. Bunun yerine her satırı 16 karaktere padding ile
    tamamlayıp setCursor + print yap. Bu tekniği yorumla açıkla.
  - Yazılacak metin bir öncekiyle aynıysa hiç yazma (static char önceki[2][17]
    ile karşılaştır). I2C trafiğini gereksiz meşgul etme.

inline void ekranMesaji(const char* satir1, const char* satir2)
  Geçici bildirim için (örn. "Komut alindi" / "SULA").

DHT arızalıysa (sensors.h'daki dhtArizali()) IKLIM sayfasında sıcaklık/nem
yerine "SENSOR HATASI" yaz — eski veriyi doğruymuş gibi gösterme.

Yazdıktan sonra akillisera.ino'yu güncelle:
- setup(): ekraniBaslat(), butonlariBaslat()
- loop(): sayfaButonunaBasildi() -> sayfaDegistir()
          sulamaButonunaBasildi() -> pompayiTetikle()
          500 ms periyotta ekraniGuncelle()
Derle ve göster.
```

---

## 💬 PROMPT 5.3 — Metin denetimi

```text
design:ux-copy skill'ini kullanarak LCD metinlerini gözden geçir:
- Her satır 16 karaktere sığıyor mu? Bunu program olarak doğrula: display.h'daki
  tüm sabit metinlerin uzunluğunu ölçen küçük bir host testi yaz.
- En uzun olası değerlerde taşma var mı? (Sic:-10.5C Nem:100 -> kaç karakter?)
- Kısaltmalar anlaşılır mı? "Sic" yerine "T:" daha net olur mu?
- Türkçe karakter kalmış mı? grep ile kontrol et.

Bulguları uygula ve testi kalıcı hale getir.
```

---

## ✅ Doğrulama komutları

```bash
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
grep -nP '[çğıöşüÇĞİÖŞÜ]' akillisera/display.h    # LCD metinlerinde çıkmamalı
grep -rn "lcd.clear()" akillisera/display.h        # yalnızca ekraniBaslat() içinde
```

## 🎓 Kabul kriterleri

- [ ] Debounce mantığı saf fonksiyon olarak test edilmiş (titreşim senaryosu dahil)
- [ ] Butonlar yalnızca yeni basışta tetikliyor, basılı tutma tekrar üretmiyor
- [ ] 3 sayfa tanımlı, buton ile döngüsel geçiş çalışıyor
- [ ] `lcd.clear()` yalnızca başlatmada kullanılıyor, güncellemede padding tekniği var
- [ ] Değişmeyen satırlar yeniden yazılmıyor
- [ ] LCD metinlerinde Türkçe karakter yok, her satır ≤ 16 karakter
- [ ] DHT arızasında "SENSOR HATASI" gösteriliyor
- [ ] Wokwi'de LCD çıktısı görsel olarak doğrulandı

## ⚠️ Bilinen tuzaklar

**I2C adresi 0x27 olmayabilir.** Bazı modüllerde 0x3F'tir. Kişi A'ya bir I2C tarayıcı taslağı yükletip adresi doğrulatın; yanlış adres sessizce hiçbir şey göstermez, hata da vermez.

**`lcd.clear()` + `print` döngüsü titreme yaratır.** Bu, öğrenci projelerinde en sık görülen görsel kusurdur ve jüri hemen fark eder. Padding tekniği tek satırlık bir ek yükle bunu tamamen çözer.

**INPUT_PULLUP mantığı terstir** — basılı durumda pin LOW okur. Bu ters mantık kod içinde birden fazla yerde tekrarlanırsa er geç biri karıştırır; `butonBasildi()` içinde bir kez kapsüllemek doğru yaklaşımdır.
