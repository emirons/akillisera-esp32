# Faz 7 — Mobil Web App (PWA)

## 🎯 Amaç

ESP32'nin flash belleğinden servis edilen, telefonda ana ekrana eklenebilen tek sayfalık kontrol arayüzü. Canlı göstergeler, sulama butonu, fan anahtarı ve LED parlaklık kaydırma çubuğu.

## 📋 Ön koşul

Faz 6 tamamlanmış, `/api/*` uç noktaları çalışıyor.

## 🧩 Kullanılacak skill / MCP

| Ne | Nerede |
|:---|:---|
| `design:ux-copy` | Türkçe arayüz metinleri |
| **`design:accessibility-review`** | Dokunma hedefi, kontrast — telefonda kullanılacak |
| `design:design-critique` | Görsel düzen incelemesi |
| **Chrome MCP** | Arayüzü gerçek tarayıcıda test etmek, ekran görüntüsü almak |
| `superpowers:using-git-worktrees` | Faz 8 ile paralel çalışılacaksa |

## 📁 Dosyalar

`akillisera/web_ui.h` (yeni) · `akillisera/network.h` (güncellenir)

---

## ⚡ Paralel yürütme seçeneği

Faz 7 (arayüz) ve Faz 8 (bulut) birbirine dokunmaz — farklı dosyalar, farklı uç noktalar. İkisini eşzamanlı yürütmek isterseniz:

```text
superpowers:using-git-worktrees skill'ini kullanarak iki izole çalışma alanı aç:
- faz7-webapp dalı: web_ui.h
- faz8-cloud dalı: cloud.h
Sonra superpowers:subagent-driven-development ile her dalda bir alt ajan
çalıştır. İkisi bitince benim onayımla birleştir; çakışma yalnızca
akillisera.ino'nun loop() fonksiyonunda olacak, orayı elle çözeceğiz.
```

Tek başınıza çalışıyorsanız sırayla ilerlemek daha az riskli.

---

## 💬 PROMPT 7.1 — web_ui.h

```text
akillisera/web_ui.h yaz. Tek sayfalık kontrol arayüzü, PROGMEM içinde saklanacak.

TEKNİK KISITLAR (bunlar pazarlık konusu değil):
- Tüm HTML/CSS/JS TEK dosyada, TEK bir R"rawliteral(...)rawliteral" bloğunda,
  const char INDEX_HTML[] PROGMEM olarak.
- HİÇBİR harici CDN, font veya kütüphane YOK. Sera internetsiz de çalışacak;
  telefon ESP32'ye bağlıyken dışarı çıkamayabilir.
- Toplam boyut 12 KB'ı geçmesin. Geçerse önce CSS'i sadeleştir.
- localStorage KULLANMA (bazı gömülü tarayıcılarda kısıtlı); durum sunucudan gelir.

ARAYÜZ YAPISI (tek ekran, dikey akış, telefon önceliği):

1. Başlık: "Akıllı Sera" + bağlantı durumu noktası (yeşil/kırmızı)

2. Gösterge kartları (2x2 ızgara, telefonda tek sütuna düşsün):
   - Sıcaklık: büyük sayı + "°C"
   - Nem: büyük sayı + "%"
   - Toprak nemi: yüzde + ince ilerleme çubuğu
   - Işık: yüzde + ince ilerleme çubuğu
   DHT geçersizse ilgili kartlar "—" göstersin ve soluklaşsın.

3. Kontroller:
   - "SULA (5 sn)" — büyük birincil buton. Basılınca 5 saniye devre dışı kalsın
     ve üzerinde geri sayım göstersin.
   - Fan: aç/kapa anahtarı (toggle)
   - Şerit LED: 0-255 kaydırma çubuğu + anlık değer etiketi
   - Mod: "Otomatik / Manuel" anahtarı. Manuel'e alınınca fan ve LED
     kontrollerinin etkin olduğu, Otomatik'te eşiklerin yönettiği görsel
     olarak belli olsun (manuel değilken kontroller soluk).

4. Alt bilgi: çalışma süresi, Wi-Fi sinyal gücü, son güncelleme saniyesi

DAVRANIŞ:
- Sayfa açılınca ve sonra 2 saniyede bir GET /api/durum ile yenilenir.
- fetch başarısız olursa bağlantı noktası kırmızıya döner ve "Bağlantı yok"
  yazar; sayfa çökmez, denemeye devam eder.
- Kaydırma çubuğu SÜRÜKLENİRKEN istek atma; bırakıldığında (change olayı) tek
  istek at. Aksi halde ESP32'yi isteğe boğarsın. Bunu yorumla belirt.
- Her POST sonrası hemen /api/durum çek, arayüzü sunucu gerçeğiyle senkronla.
  İyimser güncelleme (optimistic update) YAPMA — röle gerçekten çekti mi
  bilmiyoruz.

GÖRSEL:
- Koyu tema, yüksek kontrast (serada gündüz güneş altında telefon okunacak)
- Sistem fontu: font-family: system-ui, -apple-system, sans-serif
- Tüm dokunulabilir öğeler en az 44x44 px
- <meta name="viewport" content="width=device-width, initial-scale=1">
- PWA için: <meta name="theme-color">, apple-mobile-web-app-capable
  (manifest.json ve service worker EKLEME — flash'ta yer kaplar, "ana ekrana
  ekle" bunlarsız da çalışır. Bu kararı yorumla açıkla.)

Sonra network.h'daki GET / işleyicisini bu HTML'i PROGMEM'den servis edecek
şekilde güncelle:
server.send_P(200, "text/html", INDEX_HTML);
(send yerine send_P kullan — PROGMEM'den doğrudan okur, RAM'e kopyalamaz.)

Derle ve flash kullanımını raporla.
```

---

## 💬 PROMPT 7.2 — Tarayıcıda test

> ⚠️ **Ön koşul:** Wokwi'nin varsayılan Public Gateway'i tarayıcınızdan ESP32'ye erişime izin vermez (bkz. Faz 6'daki gateway tablosu). Bu testler için ya **Wokwi Private Gateway** kurulu olmalı ya da testler gerçek donanımın hazır olduğu Faz 9'a ertelenmelidir.

```text
Chrome MCP kullanarak arayüzü gerçek tarayıcıda test et.

1. Wokwi simülasyonunu başlat (Private Gateway aktif olmalı), IP'yi al
   (veya gerçek cihazsa akillisera.local)
2. Sayfayı aç, ekran görüntüsü al ve bana göster
3. Pencereyi 390x844 (iPhone boyutu) yapıp tekrar ekran görüntüsü al —
   düzen bozuluyor mu?

Şunları tek tek test et ve sonucunu raporla:
- Göstergeler 2 saniyede bir güncelleniyor mu? (Wokwi'de potansiyometreyi
  çevirdiğimde değişmeli)
- SULA butonu: basınca geri sayım başlıyor mu, 5 sn devre dışı kalıyor mu?
- LED kaydırma çubuğu: sürüklerken istek atmıyor, bırakınca tek istek atıyor mu?
  (Network sekmesinden doğrula)
- Fan anahtarı sunucu durumunu yansıtıyor mu?
- Manuel/Otomatik geçişi kontrolleri doğru şekilde etkinleştirip soluklaştırıyor mu?
- Wokwi'yi durdurduğumda "Bağlantı yok" görünüyor mu, sayfa çöküyor mu?

Konsol hatalarını da oku ve raporla.
```

---

## 💬 PROMPT 7.3 — Erişilebilirlik ve metin denetimi

```text
İki inceleme yap:

1. design:accessibility-review skill'i ile arayüzü denetle:
   - Renk kontrastı WCAG AA eşiğini geçiyor mu? (Özellikle koyu temada gri metin)
   - Tüm dokunma hedefleri ≥ 44x44 px mi?
   - Kaydırma çubuğu klavyeyle kullanılabiliyor mu?
   - Yalnızca renkle aktarılan bilgi var mı? (Bağlantı noktası hem renk hem
     metinle durum belirtmeli)

2. design:ux-copy skill'i ile Türkçe metinleri gözden geçir:
   - Buton adları eylem bildiriyor mu? ("SULA" iyi, "Gönder" kötü)
   - Hata mesajları kullanıcıya ne yapacağını söylüyor mu?
   - Teknik jargon gereksiz yere kullanılmış mı? ("PWM duty" yerine "Parlaklık")

Bulguları uygula, sonra tekrar derleyip flash kullanımını karşılaştır.
```

---

## ✅ Doğrulama komutları

```bash
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s --output-dir build --warnings all akillisera/
# Flash kullanımı çıktısını not al — %70'i geçerse HTML'i sadeleştir

grep -c "cdn\|http://.*\.js\|https://.*\.css" akillisera/web_ui.h   # 0 olmalı
grep -c "localStorage\|sessionStorage" akillisera/web_ui.h          # 0 olmalı
wc -c akillisera/web_ui.h                                            # < 14000 bayt
```

## 🎓 Kabul kriterleri

- [ ] Arayüz tek dosyada, PROGMEM'de, `send_P` ile servis ediliyor
- [ ] Harici CDN bağımlılığı yok, çevrimdışı çalışıyor
- [ ] 2 saniyelik yoklama ile göstergeler canlı
- [ ] Kaydırma çubuğu yalnızca bırakıldığında istek atıyor
- [ ] Bağlantı kopunca arayüz çökmüyor, durum bildiriyor
- [ ] Telefon boyutunda düzen bozulmuyor (ekran görüntüsüyle kanıtlandı)
- [ ] Erişilebilirlik incelemesi yapıldı, kontrast ve dokunma hedefleri uygun
- [ ] Flash kullanımı %70'in altında

## ⚠️ Bilinen tuzaklar

**`server.send()` ile PROGMEM string göndermek** tüm HTML'i RAM'e kopyalar; 12 KB'lık bir sayfa ESP32'nin boş heap'inin önemli bir kısmını yer ve eşzamanlı isteklerde çökme üretir. `send_P` bu kopyalamayı yapmaz.

**Kaydırma çubuğunda `input` olayını dinlemek** saniyede onlarca HTTP isteği üretir ve ESP32'nin web sunucusu bunu kaldıramaz — arayüz kilitlenir. `change` olayı yalnızca bırakıldığında tetiklenir.

**Raw string literal içinde `)rawliteral` dizisi geçerse** derleme bozulur. HTML içinde bu dizinin geçmediğinden emin olun; Claude Code'a bu riski hatırlatın.
