# Faz 8 — ThingSpeak, NTP ve TalkBack

## 🎯 Amaç

Sensör verilerini buluta göndermek, gerçek saati almak ve farklı bir ağdan gelen komutları TalkBack kuyruğundan çekmek. Rapordaki Bölüm 7'nin koda dönüşmesi.

## 📋 Ön koşul

Faz 6 tamamlanmış (ağ katmanı çalışıyor). ThingSpeak hesabı açılmış, kanal ve TalkBack oluşturulmuş, anahtarlar `secrets.h`'a girilmiş.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `superpowers:test-driven-development` | Komut ayrıştırma ve bütçe hesabı |
| **Chrome MCP** | ThingSpeak panosunu doğrulamak, TalkBack kuyruğuna komut bırakmak |
| **WebFetch** | ThingSpeak API dokümanı |
| `superpowers:systematic-debugging` | HTTP hata kodlarında |

## 📁 Dosyalar

`akillisera/cloud.h` (yeni) · `control_logic.h` (genişletilir) · `akillisera/akillisera.ino`

---

## 💬 PROMPT 8.1 — cloud.h

```text
akillisera/cloud.h yaz. NTP, ThingSpeak gönderimi ve TalkBack yoklaması burada.

#include <HTTPClient.h>
#include <time.h>
#include "secrets.h"
#include "control_logic.h"

1) NTP
   inline void saatiBaslat()
   configTime(3 * 3600, 0, "pool.ntp.org", "time.google.com");
   (Türkiye UTC+3, yaz saati uygulaması yok -> daylightOffset = 0.
   Bunu yorumla belirt; birçok örnek 3600 yazar ve saat 1 saat kayar.)

   inline bool saatiAl(char* tampon, size_t boyut)
   getLocalTime() ile "12:34:56" formatında yazar; senkron değilse false döner.
   BLOKLAMA: getLocalTime() varsayılan olarak 5 saniye bekler. İkinci parametre
   ile timeout'u 10 ms'ye çek: getLocalTime(&timeinfo, 10)

2) THINGSPEAK GÖNDERİMİ
   inline bool thingSpeakGonder(const SeraDurumu& d)
   8 alan gönder:
   field1=sicaklik, field2=nem, field3=toprakYuzde, field4=isikYuzde,
   field5=pompa(0/1), field6=fan(0/1), field7=ledParlaklik, field8=alarm(0/1)

   URL: http://api.thingspeak.com/update?api_key=KEY&field1=...&field2=...
   HTTPClient ile GET. Yanıt gövdesi yeni girdi numarasıdır; "0" dönerse
   gönderim REDDEDİLMİŞTİR (genellikle 15 sn kuralı ihlali veya kota).
   Bu durumu ayırt et ve Serial'e farklı mesaj yaz.

   HTTP zaman aşımını 5 saniyeye ayarla: http.setTimeout(5000).
   Varsayılan çok uzun; ağ sorunlu olduğunda loop()'u kilitler.

3) TALKBACK YOKLAMASI
   inline bool talkBackYokla(char* komutTamponu, size_t boyut)
   GET http://api.thingspeak.com/talkbacks/{TB_ID}/commands/execute?api_key={TB_KEY}
   - 200 ve boş olmayan gövde -> komutu tampona kopyala (strncpy + sonlandır), true
   - 200 ve boş gövde -> kuyruk boş, false
   - Diğer kodlar -> Serial'e yaz, false
   Tampon taşmasına karşı boyut kontrolünü mutlaka yap.

4) ZAMANLAYICI VE BÜTÇE
   Rapordaki Bölüm 7'de hesaplandığı üzere ücretsiz ThingSpeak hesabı günde
   ~8000 istek kabul eder. İki görev de 30 saniyede bir çalışırsa günlük toplam
   5760 istek olur ve limit altında kalınır. 15 saniye kullanılırsa 11520 olur
   ve limit AŞILIR.

   Bu hesabı cloud.h'ın başına yorum bloğu olarak yaz ki kimse aralığı
   düşürmeye kalkışmasın.

   inline void bulutGuncelle(const SeraDurumu& d, unsigned long simdi)
   - CLOUD_INTERVAL (30000) periyodunda thingSpeakGonder
   - TALKBACK_INTERVAL (30000) periyodunda talkBackYokla
   - İKİSİNİ AYNI ANDA ÇALIŞTIRMA: aralarında en az 5 saniye ofset koy.
     İki HTTP isteği arka arkaya gelirse loop() 10 saniyeye kadar takılabilir.
     Bu ofseti yorumla açıkla.
   - Wi-Fi bağlı değilse hiç deneme, sessizce çık

5) KOMUT İŞLEME
   inline void komutUygula(const char* komutMetni, SeraDurumu& d, unsigned long simdi)
   control_logic.h'daki komutAyristir() ile ayrıştır (Faz 2'de yazıldı, test edildi),
   sonucu SeraDurumu'na uygula:
   - SULA      -> pompayiTetikle(simdi)   [soğuma kilidi devrede kalsın]
   - FAN_AC    -> d.manuelFan = true; d.fanAcik = true;
   - FAN_KAPA  -> d.manuelFan = true; d.fanAcik = false;
   - OTO       -> d.manuelFan = false; d.manuelLed = false;
   - LED_AYARLA-> d.manuelLed = true; d.ledParlaklik = deger;
   - YOK       -> Serial'e "bilinmeyen komut" yaz
   Her uygulanan komutu LCD'de 2 saniye göster (ekranMesaji ile).

Yazdıktan sonra akillisera.ino'nun loop()'una bulutGuncelle() ekle ve derle.
```

---

## 💬 PROMPT 8.2 — Bütçe testi

```text
control_logic.h'a bir yardımcı fonksiyon ekle ve test et — bu, raporun
API bütçesi bölümünü kodla doğrulayacak:

constexpr int gunlukIstekSayisi(unsigned long aralikMs) {
  return 86400000UL / aralikMs;
}

static_assert(gunlukIstekSayisi(CLOUD_INTERVAL) +
              gunlukIstekSayisi(TALKBACK_INTERVAL) < 8000,
              "ThingSpeak gunluk istek limiti asiliyor - araliklari artirin");

Bu static_assert, birinin aralığı 15 saniyeye düşürmesi durumunda projeyi
DERLEME ANINDA durdurur. Bunun neden değerli olduğunu yorumla açıkla.

Ayrıca host testine ekle:
- 30000 ms -> 2880
- 15000 ms -> 5760
- İki tanesi 15 sn olsaydı toplam 11520 > 8000 (limit aşımı)
```

---

## 💬 PROMPT 8.3 — Uçtan uca bulut testi

```text
Chrome MCP kullanarak tam zinciri test edelim.

1. Wokwi'yi başlat (Wi-Fi: Wokwi-GUEST), 2-3 dakika çalışsın
2. Chrome ile ThingSpeak kanalımı aç, grafiklerde veri noktalarının
   göründüğünü ekran görüntüsüyle doğrula
3. ThingSpeak TalkBack sayfasını aç, kuyruğa "SULA" komutu ekle
4. En geç 30 saniye içinde:
   - Seri monitörde "TalkBack komutu alindi: SULA" görülmeli
   - Wokwi'de pompa LED'i (GPIO5) 5 saniye yanmalı
   - LCD'de "SULA" mesajı görünmeli
5. "LED_120" komutu ekle, şerit LED temsili LED'in parlaklığının değiştiğini
   doğrula
6. "SACMA_KOMUT" ekle, sistemin çökmediğini ve Serial'e uyarı yazdığını doğrula

Her adımın sonucunu ekran görüntüsü veya seri çıktıyla kanıtla.
Bir adım başarısızsa superpowers:systematic-debugging ile ilerle.
```

---

## ✅ Doğrulama komutları

```bash
g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
grep -n "setTimeout" akillisera/cloud.h        # HTTP zaman aşımı ayarlanmış olmalı
grep -rn "TB_KEY\|WRITE_API_KEY" akillisera/ --include=*.h | grep -v secrets
# secrets.h dışında anahtar sızıntısı olmamalı
```

## 🎓 Kabul kriterleri

- [ ] NTP saati doğru (UTC+3, yaz saati ofseti 0), bloklamıyor
- [ ] ThingSpeak'e 8 alan gönderiliyor, "0" yanıtı hata olarak ayırt ediliyor
- [ ] TalkBack yoklaması çalışıyor, tampon taşması koruması var
- [ ] İki bulut görevi arasında ofset var, art arda çalışmıyorlar
- [ ] `static_assert` ile günlük istek bütçesi derleme anında korunuyor
- [ ] HTTP zaman aşımı 5 saniyeye ayarlı
- [ ] Wi-Fi yokken bulut kodu hiç denemiyor
- [ ] Uçtan uca test: TalkBack'e bırakılan komut fiziksel/sanal eyleyiciyi tetikledi
- [ ] Anahtarlar yalnızca `secrets.h`'da

## ⚠️ Bilinen tuzaklar

**`configTime(3600, 3600, ...)`** yaygın kopyala-yapıştır hatasıdır ve Türkiye'de saati bir saat kaydırır. Türkiye 2016'dan beri kalıcı UTC+3 kullanır, yaz saati ofseti **0** olmalıdır.

**`getLocalTime()` varsayılan 5 saniye bloklar.** İlk senkronizasyonda bu kabul edilebilir ama `loop()` içinde her çağrıda bloklarsa web sunucusu yanıt veremez. Timeout parametresini küçük tutun.

**ThingSpeak "0" döndürdüğünde HTTP kodu yine 200'dür.** Yalnızca HTTP koduna bakan kod, reddedilen gönderimi başarılı sanır ve sorun günlerce fark edilmez. Yanıt gövdesi mutlaka kontrol edilmelidir.

**TalkBack `commands/execute` komutu kuyruktan siler.** Test sırasında komutun neden "kaybolduğunu" merak etmeyin — bu tasarım gereğidir ve aynı komutun iki kez çalışmasını önler.
