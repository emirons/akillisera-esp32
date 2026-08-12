# Faz 9 — Entegrasyon, İnceleme ve Teslim

## 🎯 Amaç

Tüm modülleri tek bir tutarlı `loop()` altında birleştirmek, uzun süreli kararlılığı sınamak, güvenlik ve kod incelemesinden geçirmek, dokümantasyonu ve lab raporunu üretmek.

## 📋 Ön koşul

Faz 0-8 tamamlanmış, tüm modüller ayrı ayrı çalışıyor.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| **`engineering:code-review`** | Tam kod tabanı incelemesi |
| **`vibesec`** | Son güvenlik taraması |
| `superpowers:verification-before-completion` | Teslim öncesi kanıt toplama |
| `engineering:documentation` | README ve kod dokümantasyonu |
| **`anthropic-skills:docx`** | Bahçeşehir Exp#6 lab raporu |
| **Chrome MCP** | Son uçtan uca test, ekran görüntüleri |
| **GitHub MCP** | Etiketli sürüm, son commit |

---

## 💬 PROMPT 9.1 — loop() birleştirme ve zamanlama denetimi

```text
Şimdi tüm modülleri tek bir tutarlı loop() altında birleştir. Mevcut loop()
fazlar boyunca parça parça büyüdü; onu baştan, açık bir zamanlama tasarımıyla
yeniden yaz.

TASARIM İLKESİ: loop() bir "görev zamanlayıcı" gibi çalışacak. Her görevin
kendi periyodu ve son çalışma zamanı olacak. Hiçbir görev diğerini bekletmeyecek.

void loop() {
  const unsigned long simdi = millis();

  // --- Her turda, gecikmesiz ---
  agIsle();                    // server.handleClient()
  pompayiGuncelle(simdi);      // 5 sn dolduğunda pompayı kapat
  agGuncelle(simdi);           // Wi-Fi durum makinesi

  // --- Butonlar: her turda örneklenir (debounce içeride) ---
  if (sulamaButonunaBasildi(simdi)) pompayiTetikle(simdi);
  if (sayfaButonunaBasildi(simdi))  sayfaDegistir();

  // --- 2000 ms: sensör okuma + karar ---
  // --- 500 ms: LCD güncelleme ---
  // --- 30000 ms: ThingSpeak (ofset 0) ---
  // --- 30000 ms: TalkBack (ofset 5000) ---
  // --- 100 ms: alarm yanıp sönme ---
}

GÖREVLER:
1. Bu yapıyı kur, her bölümü yorumla.
2. loop() süresini ölç: başında micros() al, sonunda farkı hesapla, en yüksek
   değeri static olarak sakla ve /api/saglik JSON'una "maxLoopMikrosaniye"
   alanı olarak ekle. Bu değer 50000'i (50 ms) aşarsa bir sorun var demektir.
3. Tüm dosyalarda delay() araması yap. setup() dışında kalan her delay()'i
   gerekçesiyle bana raporla ve kaldır.
4. Boş heap'i /api/saglik'a ekle (ESP.getFreeHeap()) ve en düşük görülen
   değeri de sakla (ESP.getMinFreeHeap()).

Derle, Wokwi'de çalıştır, /api/saglik çıktısını göster.
```

---

## 💬 PROMPT 9.2 — Uzun süreli kararlılık testi

```text
Bellek sızıntısı ve zamanlayıcı kayması avı yapacağız. Bu, gömülü sistemlerde
teslim öncesi atlanmaması gereken adımdır.

1. Wokwi simülasyonunu en az 20 dakika kesintisiz çalıştır (ben başlatacağım).
2. Bu süre boyunca her 2 dakikada bir /api/saglik'i çek ve şu tabloyu kur:
   | dakika | bosHeap | enDusukHeap | maxLoopMikro | wifiRSSI | dhtHataSayaci |
3. Analiz et:
   - bosHeap zamanla düşüyor mu? Düşüyorsa SIZINTI var — kaynağını bul.
     En olası yerler: her istekte ayrılan JSON belgesi, String birleştirme,
     HTTPClient nesnesinin end() çağrılmaması.
   - maxLoopMikro ne zaman zirve yapıyor? Muhtemelen ThingSpeak gönderimi
     sırasında. 5 saniyeyi aşıyorsa HTTP zaman aşımı çalışmıyor demektir.
   - ThingSpeak'e gönderilen kayıt sayısı beklenen sayıyla uyuşuyor mu?
     20 dakika / 30 saniye = 40 kayıt olmalı.

4. Ayrıca şu senaryoları test et:
   - Wi-Fi'ı kes (Wokwi'de simülasyonu ağdan koparmak zorsa secrets.h'da SSID'yi
     boz ve yeniden derle): sera otonom çalışmaya devam ediyor mu? Sulama,
     fan, LED, LCD çalışıyor mu?
   - Wi-Fi geri gelince otomatik bağlanıyor mu?
   - DHT'yi devre dışı bırak: "SENSOR HATASI" görünüyor mu, sistem çöküyor mu?

Bulguları raporla. Sızıntı varsa superpowers:systematic-debugging ile ilerle:
önce hangi isteğin sızdırdığını izole et (tek uç noktayı 100 kez çağır,
heap'i ölç), sonra düzelt.
```

---

## 💬 PROMPT 9.3 — Kod incelemesi ve güvenlik

```text
İki aşamalı son inceleme:

A) engineering:code-review skill'i ile tüm kod tabanını incele. Odak:
   - Sınır durumları: sensör NaN, ağ yok, kuyruk boş, tampon dolu
   - Kaynak yönetimi: her HTTPClient.begin() için end() var mı?
   - Yarış durumu: web isteği pompayı tetiklerken loop() aynı anda kapatıyor mu?
   - Ölü kod, kullanılmayan değişken, kopyala-yapıştır tekrarı
   - config.h dışında kalan çıplak sayı var mı?
   - Yorumlar kodun NE yaptığını değil NİÇİN yaptığını anlatıyor mu?

B) vibesec skill'i ile son güvenlik taraması. Bu sefer tüm kod tabanı:
   - Girdi doğrulama boşlukları
   - Tampon taşması riskleri (strcpy, sprintf kullanımı)
   - Sızdırılan sır: secrets.h dışında anahtar var mı, Serial'e anahtar
     yazdırılıyor mu?
   - git geçmişinde anahtar kalmış mı? (git log -p ile ara)

Bulguları önem sırasına göre listele. superpowers:receiving-code-review
skill'ine göre her bulguyu değerlendir — gerekçesiz olanları gerekçesiyle
reddet, gerçek olanları düzelt. Düzeltmelerden sonra tüm testleri ve
derlemeyi tekrar çalıştır.
```

---

## 💬 PROMPT 9.4 — Dokümantasyon ve teslim paketi

```text
engineering:documentation skill'ini kullanarak teslim paketini hazırla.

1. README.md yaz:
   - Proje özeti (2 paragraf)
   - Donanım listesi ve pin tablosu (rapordaki Bölüm 2 ile birebir aynı olmalı,
     farklıysa uyar)
   - Kurulum: arduino-cli komutları, kütüphaneler, secrets.h hazırlama
   - Derleme ve yükleme komutları
   - Eşik değerleri tablosu ve nasıl değiştirileceği
   - API uç noktaları tablosu (yöntem, yol, gövde, yanıt)
   - TalkBack komut sözlüğü
   - Sorun giderme: en sık 6 hata ve çözümü
   - Bilinen sınırlamalar (kimlik doğrulama yok, HTTP kullanılıyor, ADC
     doğrusal değil, mDNS bazı Android'lerde çalışmıyor)

2. Kod içi dokümantasyonu tamamla: her .h dosyasının başına 3-5 satırlık
   "bu modül ne yapar, neye bağımlıdır" bloğu.

3. Mimari şemayı Mermaid olarak README'ye ekle: modüller arası bağımlılık grafiği
   (config.h -> control_logic.h -> sensors/actuators -> network/cloud -> .ino)

4. Teslim kontrol listesi oluştur ve her maddeyi doğrula:
   - [ ] Tüm host testleri geçiyor (çıktıyı yapıştır)
   - [ ] Uyarısız derleniyor (--warnings all çıktısını yapıştır)
   - [ ] Flash ve RAM kullanımı raporlandı
   - [ ] secrets.h git'te değil, secrets.h.example var
   - [ ] 20 dakikalık kararlılık testi geçildi, sızıntı yok
   - [ ] Wokwi diyagramı çalışır durumda
   - [ ] README eksiksiz
```

---

## 💬 PROMPT 9.5 — Lab raporu

```text
anthropic-skills:docx skill'ini kullanarak Bahçeşehir Exp#6 Lab Raporunun
YAZILIM bölümlerini üret. (Donanım bölümlerini Kişi A dolduracak, oraya
yer tutucu bırak.)

İçerik:
1. Yazılım mimarisi: modüler .h yapısı, neden bu ayrım seçildi
2. Kontrol algoritmaları:
   - Otomatik sulama akış şeması ve soğuma kilidi gerekçesi
   - Fan histerezis bandı: neden 10 puanlık band, röle çatırdaması nedir,
     histerezis olmasaydı ne olurdu (grafikle)
   - PWM ile LED parlaklık kontrolü: LEDC birimi, 5 kHz seçimi
3. Bloklamayan zamanlama: delay() yerine millis() tabanlı görev zamanlayıcı,
   ölçülen maksimum loop süresi
4. IoT katmanı: mDNS'in yerel ağ sınırı, TalkBack ile uzaktan erişim,
   API istek bütçesi hesabı
5. Test metodolojisi: host birim testleri (kaç test, neyi kapsıyor),
   Wokwi simülasyonu, 20 dakikalık kararlılık testi sonuçları
6. Karşılaşılan sorunlar ve çözümleri — gerçek olanları yaz, uydurma
7. Bilinen sınırlamalar ve gelecek çalışma önerileri

Her sayısal iddia için kaynak göster: test çıktısı, /api/saglik ölçümü,
derleme raporu. Ölçmediğin bir şeyi rapora yazma.

Ekran görüntüsü yer tutucuları bırak: web arayüzü, ThingSpeak grafiği,
Wokwi simülasyonu, seri monitör çıktısı.
```

---

## ✅ Doğrulama komutları

```bash
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
git log --oneline                              # faz başına anlamlı commit'ler
git log -p | grep -i "api_key\|password" | head # geçmişte sır sızıntısı
curl -s http://akillisera.local/api/saglik | python3 -m json.tool
```

## 🎓 Kabul kriterleri

- [ ] `loop()` görev zamanlayıcı yapısında, maksimum süre ölçülüyor ve < 50 ms
- [ ] `setup()` dışında hiç `delay()` yok
- [ ] 20 dakikalık testte boş heap düşmüyor (sızıntı yok)
- [ ] Wi-Fi kesildiğinde sera otonom çalışıyor, geri geldiğinde bağlanıyor
- [ ] Kod incelemesi ve güvenlik taraması yapıldı, bulgular ele alındı
- [ ] Git geçmişinde API anahtarı yok
- [ ] README eksiksiz, mimari şema dahil
- [ ] Lab raporunun yazılım bölümleri üretildi
- [ ] Tüm sayısal iddialar ölçümle destekleniyor

## ⚠️ Bilinen tuzaklar

**Bellek sızıntısı 20 dakikada görünmeyebilir.** Sızıntı isteğe bağlıysa (her HTTP isteğinde 40 bayt gibi), fark yalnızca binlerce istekten sonra belirginleşir. Sızıntı şüphesi varsa tek bir uç noktayı `for i in {1..500}; do curl ...; done` ile döverek heap'i ölçün — bu, 20 dakikalık testten çok daha keskin bir sinyal verir.

**`String` sınıfı ESP32'de heap parçalanmasının bir numaralı sebebidir.** Kod tabanında `String` kullanımı varsa `char[]` ve `snprintf` ile değiştirmeyi değerlendirin; özellikle `loop()` içinde çağrılan yollarda.

**Teslim öncesi `secrets.h`'ı git'e eklemek** en sık yapılan son dakika hatasıdır. `git log -p | grep` ile geçmişi taramak, dosya sonradan `.gitignore`'a eklenmiş olsa bile eski commit'lerde kalan anahtarı yakalar. Anahtar sızmışsa ThingSpeak'ten yeni anahtar üretin — geçmişi temizlemek yerine anahtarı iptal etmek daha güvenilirdir.
