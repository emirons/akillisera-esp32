// cloud.h — ThingSpeak gönderimi + TalkBack yoklaması + komut uygulama.
// NTP saat zaman.h'da. Wi-Fi yoksa hiç deneme yapılmaz (sera otonom).
//
// ── API BÜTÇESİ (DÜŞÜRME!) ────────────────────────────────────────────────
// Ücretsiz ThingSpeak günde ~8000 istek kabul eder. İki görev (gönderim +
// TalkBack) 30 sn'de bir çalışınca: 2880 + 2880 = 5760 istek/gün (limit altı).
// 15 sn yapılırsa 5760 + 5760 = 11520 > 8000 -> KOTA AŞILIR. config.h'daki
// static_assert bunu derleme anında korur (bkz. control_logic.h gunlukIstekSayisi).
// ──────────────────────────────────────────────────────────────────────────
#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "control_logic.h"
#include "kontrol_durumu.h"
#include "actuators.h"
#include "display.h"
#include "secrets.h"

// ThingSpeak'e 8 alan gönderir. Yanıt gövdesi yeni girdi no'su; "0" REDDEDİLMİŞTİR
// (HTTP kodu yine 200'dür — bu yüzden gövde kontrol edilir).
inline bool thingSpeakGonder(const DurumKaydi& d) {
  HTTPClient http;
  http.setTimeout(5000);   // varsayılan çok uzun; ağ sorununda loop'u kilitler
  String url = String("http://api.thingspeak.com/update?api_key=") + TS_WRITE_API_KEY
    + "&field1=" + String(d.sicaklik, 1)
    + "&field2=" + String(d.nem, 1)
    + "&field3=" + d.toprakYuzde
    + "&field4=" + d.isikYuzde
    + "&field5=" + (d.pompa ? 1 : 0)
    + "&field6=" + (d.fan ? 1 : 0)
    + "&field7=" + d.led
    + "&field8=" + (d.alarm ? 1 : 0);
  http.begin(url);
  int kod = http.GET();
  bool ok = false;
  if (kod == 200) {
    String govde = http.getString();
    if (govde == "0") Serial.println(F("ThingSpeak reddetti (15 sn kurali veya kota)"));
    else { Serial.print(F("ThingSpeak OK, girdi #")); Serial.println(govde); ok = true; }
  } else {
    Serial.print(F("ThingSpeak HTTP hata: ")); Serial.println(kod);
  }
  http.end();
  return ok;
}

// TalkBack kuyruğundan sıradaki komutu çeker (ve siler). Komut varsa buf'a
// güvenli kopyalar (tampon taşması koruması) ve true döner.
inline bool talkBackYokla(char* buf, size_t boyut) {
  HTTPClient http;
  http.setTimeout(5000);
  String url = String("http://api.thingspeak.com/talkbacks/") + TB_ID
    + "/commands/execute?api_key=" + TB_KEY;
  http.begin(url);
  int kod = http.GET();
  bool var = false;
  if (kod == 200) {
    String govde = http.getString();
    if (govde.length() > 0) {
      strncpy(buf, govde.c_str(), boyut - 1);
      buf[boyut - 1] = '\0';         // her koşulda sonlandır (taşma koruması)
      var = true;
    }
  } else {
    Serial.print(F("TalkBack HTTP hata: ")); Serial.println(kod);
  }
  http.end();
  return var;
}

// Gelen komut metnini ayrıştırıp uygular. Metni önce kırpar (TalkBack "\r\n" ekler).
inline void komutUygula(const char* komutMetni, unsigned long simdi) {
  char tmp[32];
  strncpy(tmp, komutMetni, sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = '\0';
  metniKirp(tmp);                    // "SULA\r\n" -> "SULA"
  KomutSonucu k = komutAyristir(tmp);
  switch (k.komut) {
    case Komut::SULA:
      pompayiTetikle(simdi);         // 60 sn soğuma kilidi devrede kalır
      ekranMesaji("Uzak komut", "SULA");
      Serial.println(F("TalkBack: SULA"));
      break;
    case Komut::FAN_AC:
      kdFanElle(true);  ekranMesaji("Uzak komut", "FAN AC");  break;
    case Komut::FAN_KAPA:
      kdFanElle(false); ekranMesaji("Uzak komut", "FAN KAPA"); break;
    case Komut::OTO:
      kdFanOtoAyarla(true); kdLedOtoAyarla(true); ekranMesaji("Uzak komut", "OTO"); break;
    case Komut::LED_AYARLA:
      kdLedElle(k.deger);
      { char m[17]; snprintf(m, sizeof(m), "LED %d", k.deger); ekranMesaji("Uzak komut", m); }
      break;
    case Komut::YOK:
    default:
      Serial.print(F("Bilinmeyen komut: ")); Serial.println(tmp);
      break;
  }
}

// Bulut zamanlayıcı. Wi-Fi bağlı değilse hiç deneme.
// İki HTTP görevini AYNI loop turunda çalıştırmaz: gönderimden sonra en az 5 sn
// geçmeden TalkBack yoklaması yapılmaz. Art arda iki istek loop'u 10 sn'ye kadar
// kilitleyebilir; bu ofset bunu önler.
inline void bulutGuncelle(unsigned long simdi) {
  if (WiFi.status() != WL_CONNECTED) return;
  static unsigned long sonGonderim = 0, sonYoklama = 0;

  if (simdi - sonGonderim >= CLOUD_INTERVAL) {
    sonGonderim = simdi;
    thingSpeakGonder(kdDurum());
    return;                          // bu turda TalkBack'i çalıştırma (5 sn ofset)
  }
  if (simdi - sonYoklama >= TALKBACK_INTERVAL && simdi - sonGonderim >= 5000) {
    sonYoklama = simdi;
    char komut[32];
    if (talkBackYokla(komut, sizeof(komut))) komutUygula(komut, simdi);
  }
}
