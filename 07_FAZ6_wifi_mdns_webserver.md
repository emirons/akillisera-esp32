# Faz 6 — Wi-Fi, mDNS ve Web Server

## 🎯 Amaç

ESP32'yi ağa bağlamak, `http://akillisera.local` adresini yayınlamak ve JSON tabanlı bir kontrol API'si sunmak. Bağlantı koptuğunda sistemin otonom çalışmaya devam etmesi ve arka planda yeniden bağlanması sağlanacak.

## 📋 Ön koşul

Faz 5 tamamlanmış. Wi-Fi bilgileri `secrets.h`'a girilmiş.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| **`vibesec`** | HTTP girdi doğrulama açıkları — bu fazda zorunlu |
| `engineering:architecture` | API uç noktalarının tasarımı |
| `superpowers:systematic-debugging` | Bağlantı sorunlarında |
| **Chrome MCP** | Uç noktaları tarayıcıdan test etmek |

## 📁 Dosyalar

`akillisera/network.h` (yeni) · `akillisera/akillisera.ino` (güncellenir)

---

## 💬 PROMPT 6.1 — network.h

```text
akillisera/network.h yaz. WiFi, mDNS ve WebServer burada kapsüllenecek.

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "secrets.h"

1) BAĞLANTI YÖNETİMİ — BLOKLAMAYAN
   Bu bölüm kritik: Wi-Fi yoksa sera OTONOM çalışmaya devam etmeli. Sulama,
   fan ve LED mantığı ağdan bağımsızdır. Bağlantı beklerken loop() DURMAMALI.

   enum class AgDurumu { BASLATILMADI, BAGLANIYOR, BAGLI, HATA };

   inline void agBaslat()
   - WiFi.mode(WIFI_STA)
   - WiFi.setHostname("akillisera")
   - WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
   - BLOKLAMA: while(WiFi.status() != WL_CONNECTED) delay(500) YAZMA.
     Yalnızca begin() çağır ve çık.

   inline AgDurumu agGuncelle(unsigned long simdi)
   - loop()'ta her turda çağrılır
   - WiFi.status() kontrol eder
   - BAGLANIYOR durumunda 20 saniye geçtiyse HATA'ya düşer ve 30 saniye sonra
     yeniden dener (üstel geri çekilme gerekmez, sabit aralık yeterli — ama
     neden yeterli olduğunu yorumla açıkla)
   - Yeni bağlandığında: IP'yi Serial'e yaz, mDNS'i başlat, sunucuyu başlat
   - Bağlantı koptuğunda: durumu güncelle, Serial'e yaz, otonom moda geç

2) mDNS
   inline void mdnsBaslat()
   - MDNS.begin("akillisera")  -> http://akillisera.local
   - MDNS.addService("http", "tcp", 80)
   - Başarısız olursa Serial'e uyarı yaz ama sistemi durdurma; IP ile erişim
     hâlâ çalışır.
   YORUM EKLE: mDNS yalnızca aynı yerel ağda çalışır; farklı ağdan erişim
   Faz 8'deki TalkBack ile sağlanacak. (Rapordaki Bölüm 7'ye atıf ver.)

3) API UÇ NOKTALARI
   static WebServer server(80);

   GET  /              -> web_ui.h'daki HTML (Faz 7'de yazılacak; şimdilik
                          "Faz 7'de gelecek" yazan bir yer tutucu)
   GET  /api/durum     -> JSON: tüm sensör ve eyleyici durumu
   POST /api/sula      -> pompayı tetikler
   POST /api/fan       -> gövde: {"acik": true|false}
   POST /api/led       -> gövde: {"parlaklik": 0-255}
   POST /api/mod       -> gövde: {"otomatik": true|false}
   GET  /api/saglik    -> uptime, boş heap, Wi-Fi RSSI, DHT hata sayacı

   /api/durum JSON şeması:
   {
     "sicaklik": 23.4, "nem": 65.2,
     "toprakHam": 2100, "toprakYuzde": 42,
     "isikHam": 1800, "isikYuzde": 44,
     "pompa": false, "fan": true, "led": 200,
     "alarm": false, "otomatik": true,
     "dhtGecerli": true, "calismaSuresi": 123456
   }
   ArduinoJson kullan, elle string birleştirme YAPMA.

4) GİRDİ DOĞRULAMA — GÜVENLİK
   HER uç nokta girdisini doğrula:
   - /api/led parlaklık: sayı değilse, 0-255 dışındaysa -> 400 Bad Request ve
     açıklayıcı JSON hata mesajı. Sessizce kırpma; çağıran taraf hatayı bilsin.
   - JSON ayrıştırma hatası -> 400
   - Bilinmeyen uç nokta -> 404 JSON
   - İstek gövdesi 512 bayttan büyükse -> 413
   Doğrulama mantığını control_logic.h'daki saf fonksiyonlara delege et ki
   test edilebilsin.

5) CORS
   Geliştirme sırasında tarayıcıdan test edebilmek için
   server.sendHeader("Access-Control-Allow-Origin", "*") ekle ve YANINA
   "yalnızca yerel ağ içi kullanım için; üretimde kaldırılmalı" yorumu yaz.

6) inline void agIsle()  -> server.handleClient(); loop()'ta her turda.

Yazdıktan sonra akillisera.ino'yu güncelle ve derle.
DİKKAT: loop() içinde hiçbir noktada delay() veya while-bekleme olmayacak.
```

---

## 💬 PROMPT 6.2 — Güvenlik taraması

```text
vibesec skill'ini kullanarak network.h'ı tara. Özellikle:

- Girdi doğrulaması eksik uç nokta var mı?
- String birleştirme ile JSON üretilen yer kaldı mı (injection riski)?
- Tampon taşması riski: server.arg() dönüşü doğrudan char dizisine kopyalanıyor mu?
- Kimlik doğrulaması hiç yok — bu bilinçli bir karar mı, riskleri neler?
- Yığın (heap) tükenmesi: her istekte JSON belgesi ayrılıyorsa parçalanma olur mu?
  StaticJsonDocument mi DynamicJsonDocument mi kullanılmalı?

Bulguları önem sırasına göre listele. Ödev kapsamında kabul edilebilir olanları
"bilinen sınırlama" olarak kodda yorumla belgele, gerçekten düzeltilmesi
gerekenleri düzelt.
```

---

## ⚠️ Wokwi'de test etmeden önce: Public Gateway vs Private Gateway

Wokwi, ESP32 için sanal bir Wi-Fi ağı (`Wokwi-GUEST`, parolasız) simüle eder ve tam bir TCP/IP yığını sağlar. Ancak **varsayılan "Public Gateway" yalnızca giden internet erişimi verir; sizin yerel ağınıza erişim vermez.** Bu şu anlama gelir:

| İşlem | Public Gateway | Private Gateway |
|:---|:---:|:---:|
| ESP32 → ThingSpeak'e veri gönderme | ✅ | ✅ |
| ESP32 → NTP saati alma | ✅ | ✅ |
| ESP32 → TalkBack yoklaması | ✅ | ✅ |
| **Tarayıcınız → `akillisera.local`** | ❌ | ✅ |
| **Tarayıcınız → ESP32'nin IP'si** | ❌ | ✅ |

Yani Faz 8'in bulut testleri Public Gateway ile sorunsuz yapılır, ama **Faz 6 ve Faz 7'nin web arayüzü testleri için Private Gateway gerekir**. Private Gateway, bilgisayarınızda çalıştırdığınız küçük bir uygulamadır ve simülasyonu doğrudan yerel ağınıza bağlar.

Private Gateway kurmak istemiyorsanız alternatif: web arayüzünü Faz 9'da gerçek donanım hazır olduğunda test edin, Wokwi'de yalnızca derlenme ve seri çıktı doğrulaması yapın. Bu durumda Faz 7'nin tarayıcı testlerini Faz 9'a erteleyin.

---

## 💬 PROMPT 6.3 — Uç nokta testi

```text
Wokwi'de Wi-Fi çalışır (SSID: "Wokwi-GUEST", parola yok). secrets.h'ı geçici
olarak bu değerlere ayarlayıp simülasyonda test edelim.

1. secrets.h'ı Wokwi-GUEST için ayarla (gerçek bilgilerimi bozma, yorum satırına al)
2. Derle, Wokwi'yi başlatmam için talimat ver
3. Seri monitörde IP adresini gördüğümde sana söyleyeceğim

Sonra Chrome MCP ile şu testleri yap ve sonuçları raporla:
- GET  /api/durum        -> 200, geçerli JSON, tüm alanlar mevcut
- POST /api/led {"parlaklik": 128}   -> 200
- POST /api/led {"parlaklik": 999}   -> 400 (kırpma DEĞİL, hata)
- POST /api/led {"parlaklik": "abc"} -> 400
- POST /api/led  (bozuk JSON)        -> 400
- GET  /olmayan-adres                -> 404 JSON
- GET  /api/saglik                   -> 200, uptime artıyor

Her testin gerçek yanıtını göster. Beklenenden farklıysa düzelt ve tekrar test et.
```

---

## ✅ Doğrulama komutları

```bash
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
grep -rn "delay(\|while (WiFi" akillisera/network.h    # bloklama olmamalı
curl -s http://akillisera.local/api/durum | python3 -m json.tool
curl -s -X POST http://akillisera.local/api/led -d '{"parlaklik":999}' -w "\n%{http_code}\n"
```

## 🎓 Kabul kriterleri

- [ ] Wi-Fi bağlantısı bloklamıyor; ağ yokken sera otonom çalışıyor
- [ ] Bağlantı koptuğunda 30 sn'de bir yeniden deneniyor
- [ ] `http://akillisera.local` yerel ağdan açılıyor
- [ ] 7 uç nokta çalışıyor ve JSON döndürüyor
- [ ] Geçersiz girdiler 400 ile reddediliyor (sessizce kırpılmıyor)
- [ ] `vibesec` taraması yapıldı, bulgular ele alındı
- [ ] `/api/saglik` boş heap ve uptime raporluyor
- [ ] `loop()` içinde bloklama yok

## ⚠️ Bilinen tuzaklar

**`while (WiFi.status() != WL_CONNECTED) delay(500);`** neredeyse tüm ESP32 örneklerinde geçer ve bu projede kabul edilemez: Wi-Fi yoksa sera sulama yapmayı bırakır. Otonom davranış ağdan bağımsız olmalıdır.

**mDNS bazı Android sürümlerinde çalışmaz.** iOS ve masaüstü tarayıcılar sorunsuzdur; Android'de `.local` çözümlemesi cihaza göre değişir. Yedek olarak IP adresini LCD'nin SISTEM sayfasında göstermek bu sorunu pratikte çözer.

**`DynamicJsonDocument` her istekte heap'ten ayırır** ve uzun çalışmada parçalanmaya yol açar. Sabit boyutlu `StaticJsonDocument<512>` tercih edilmelidir; ArduinoJson 7'de `JsonDocument` otomatik yönetir, sürümü kontrol edin.
