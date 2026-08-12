# Faz 3 — Sensör Katmanı

## 🎯 Amaç

Fiziksel sensörlerden güvenilir veri okumak: DHT11'in NaN dönmesini yönetmek, analog okumaları gürültüye karşı filtrelemek ve okuma sıklığını sensörlerin fiziksel sınırlarına uydurmak.

## 📋 Ön koşul

Faz 2 tamamlanmış, `control_logic.h` testleri yeşil.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `.claude/skills/esp32-firmware-kurallari` | Otomatik |
| `superpowers:systematic-debugging` | Okuma tutarsızlığı çıkarsa |
| **WebFetch** | DHT sensor library API dokümanı |

## 📁 Dosyalar

`akillisera/sensors.h` (yeni) · `akillisera/akillisera.ino` (güncellenir)

---

## 💬 PROMPT 3.1 — sensors.h

```text
akillisera/sensors.h yaz. Tüm sensör okumaları burada; başka hiçbir dosyada
analogRead() veya dht.read*() çağrısı olmayacak.

YAPI:
- Tüm fonksiyonlar `inline`, dosya tek derleme birimi içinde kalacak.
- Modül seviyesinde `static` değişkenler kullan, global namespace'i kirletme.
- config.h ve control_logic.h'ı include et.

1) DHT11
   #include <DHT.h>
   static DHT dht(DHT_PIN, DHT11);
   inline void sensorlerBaslat() -> dht.begin(), analog pinler için pinMode gerekmez.

   inline bool dhtOku(float& sicaklik, float& nem)
   - dht.readTemperature() ve dht.readHumidity() çağırır
   - control_logic.h'daki sensorDegeriGecerli() ile İKİSİNİ de doğrular
   - geçersizse false döner ve referansları DEĞİŞTİRMEZ (son geçerli değer korunur)
   - KRİTİK: DHT11 en fazla 1 Hz örneklenebilir. Bu fonksiyon, son okumadan
     bu yana 2000 ms geçmemişse okuma yapmadan true dönmeli (son değeri korur).
     Bunu içeride static unsigned long sonOkuma ile yönet.

   Ardışık başarısız okuma sayacı tut: art arda 5 okuma başarısızsa
   `bool dhtArizali()` true dönsün. Bu bilgi LCD'de ve web arayüzünde
   gösterilecek — sessizce eski veri göstermek tehlikeli.

2) ANALOG SENSÖRLER (toprak nem GPIO34, LDR GPIO35)
   inline int analogOkuMedyan(uint8_t pin, uint8_t ornekSayisi = 5)
   - ornekSayisi kadar okuma yapar, aralarında 2 ms bekler (setup değil loop'ta
     çağrılacağı için delayMicroseconds(2000) yerine basit bir for döngüsü ve
     delayMicroseconds kullan — 10 ms toplam kabul edilebilir)
   - okumaları sıralar, ORTANCA değeri döndürür
   Neden medyan? Ortalama, tek bir ani sıçramadan (elektriksel gürültü, pompa
   rölesi çekerken oluşan besleme dalgalanması) etkilenir; medyan etkilenmez.

   inline int toprakNemOku()  -> analogOkuMedyan(SOIL_PIN)
   inline int isikOku()       -> analogOkuMedyan(LDR_PIN)

3) YÜZDE DÖNÜŞÜMÜ (arayüzde göstermek için)
   inline int toprakNemYuzde(int hamDeger)
   Kapasitif sensörde ham değer TERS orantılıdır: kuru toprakta yüksek, ıslakta
   düşük. Kalibrasyon sabitlerini config.h'a ekle:
   SOIL_RAW_DRY = 3000 (havada), SOIL_RAW_WET = 1200 (suda)
   map() ile 0-100'e çevir ve constrain() ile kırp.
   Bu sabitlerin yanına "Kişi A kalibrasyonda ölçtükten sonra güncellenecek"
   yorumunu yaz.

   inline int isikYuzde(int hamDeger) -> benzer şekilde 0-100.

4) TOPLU OKUMA
   struct SensorVerisi { float sicaklik; float nem; int toprakHam; int isikHam;
                         bool dhtGecerli; };
   inline SensorVerisi tumSensorleriOku()
   Tüm okumaları yapıp tek struct döndürür. loop() bunu SENSOR_READ_INTERVAL
   (2000 ms) periyodunda çağıracak.

Yazdıktan sonra akillisera.ino'da:
- setup() içinde Serial.begin(115200) ve sensorlerBaslat()
- loop() içinde millis() tabanlı 2 saniyelik zamanlayıcı ile tumSensorleriOku()
  ve sonucu Serial'e yazdır (F() makrosu ile).
DELAY KULLANMA.

Derle ve çıktıyı göster.
```

---

## 💬 PROMPT 3.2 — Wokwi'de doğrulama

```text
Şimdi sensör okumalarını Wokwi simülasyonunda doğrulayalım.

1. Projeyi --output-dir build ile derle.
2. Wokwi simülasyonunu başlatmam için bana adımları söyle (VS Code Wokwi
   eklentisi veya wokwi.com üzerinden).
3. Simülasyonda potansiyometreleri çevirdiğimde seri monitörde toprak nem ve
   ışık değerlerinin değişmesi gerekiyor. DHT sanal sensöründe sıcaklık/nem
   ayarlanabiliyor.

Ben sana seri monitör çıktısını yapıştıracağım. Şunları kontrol et:
- Değerler 0-4095 aralığında mı?
- DHT okumaları 2 saniyede bir mi güncelleniyor (daha sık değil)?
- NaN görünüyor mu? Görünüyorsa dhtOku() neden filtrelememiş?
- Medyan filtresi çalışıyor mu — potansiyometre sabitken değer titriyor mu?

Sorun bulursan superpowers:systematic-debugging skill'ini kullan: önce hipotez
kur, sonra o hipotezi ayıklayacak bir ölçüm ekle, tahminle düzeltme yapma.
```

---

## ✅ Doğrulama komutları

```bash
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
grep -rn "analogRead\|readTemperature\|readHumidity" akillisera/ --include=*.h --include=*.ino
# çıktı YALNIZCA sensors.h içinde olmalı
grep -rn "delay(" akillisera/*.ino akillisera/*.h
# setup() dışında hiç olmamalı
```

## 🎓 Kabul kriterleri

- [ ] Tüm sensör okumaları `sensors.h` içinde kapsüllendi
- [ ] DHT11 en fazla 0.5 Hz örnekleniyor (2 sn aralık)
- [ ] NaN okumalar filtreleniyor, son geçerli değer korunuyor
- [ ] Ardışık 5 hatada `dhtArizali()` true dönüyor
- [ ] Analog okumalar medyan filtreli
- [ ] Kalibrasyon sabitleri `config.h`'da ve yorumlanmış
- [ ] `loop()` içinde `delay()` yok
- [ ] Wokwi'de seri çıktı doğrulandı

## ⚠️ Bilinen tuzaklar

**DHT11'i 2 saniyeden sık okumak** sensörün NaN döndürmesine yol açar ve bu, "sensör bozuk" sanılmasına neden olan en yaygın hatadır. Sınırlama kütüphane içinde değil, sensörün fiziğinde.

**`analogRead()` çağrısını pompa rölesi çekerken yapmak** besleme dalgalanması nedeniyle 200-300 birimlik sıçrama üretebilir. Medyan filtre bunu büyük ölçüde bastırır; sorun devam ederse okumayı röle geçişlerinden 100 ms uzağa kaydırmak gerekir.

**ESP32'nin ADC'si doğrusal değildir**, özellikle 0-100 mV ve 3.1 V üstünde. Mutlak doğruluk gerekmediği, yalnızca eşik karşılaştırması yapıldığı için bu proje açısından sorun değil — ama raporda bu sınırlamayı belirtmek yerinde olur.
