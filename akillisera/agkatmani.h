// agkatmani.h — WiFi, mDNS, WebServer, JSON API. Hepsi bloklamayan.
// (Dosya adı "network.h" DEĞİL: macOS case-insensitive FS'de core Network.h ile çakışır.)
// KRİTİK: Wi-Fi yoksa sera OTONOM çalışır — loop() burada asla delay/while beklemez.
//
// GÜVENLİK NOTLARI (vibesec taraması — bilinen sınırlamalar, ödev kapsamı):
//  - KİMLİK DOĞRULAMA YOK: yerel ağdaki herkes API'yi çağırabilir. Kabul edildi
//    çünkü cihaz izole ev/lab ağında. Üretimde token/HTTP Basic gerekir.
//  - CORS "*" ve CSRF koruması yok: yalnızca yerel ağ geliştirme testi için.
//    Üretimde CORS başlığı kaldırılmalı ve durum-değiştiren POST'lar korunmalı.
//  - Gövde boyut kontrolü (413) post-hoc: WebServer gövdeyi zaten belleğe alır;
//    gerçek DoS koruması Content-Length reddiyle olur (ESP32 WebServer sınırlar).
//  - Girdi doğrulama saf fonksiyonlara delege (parlaklikGecerliMi) — test edilir.
//  - JSON üretimi yalnızca ArduinoJson (injection yok); Arduino String (taşma yok).
#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "control_logic.h"
#include "actuators.h"
#include "web_ui.h"
#include "secrets.h"

// loop -> API'ye taşınan anlık durum (JSON /api/durum için).
struct DurumKaydi {
  float sicaklik, nem;
  int   toprakHam, toprakYuzde, isikHam, isikYuzde;
  bool  pompa, fan;
  int   led;
  bool  alarm, dhtGecerli;
  int   dhtHata;
};

enum class AgDurumu { BASLATILMADI, BAGLANIYOR, BAGLI, HATA };

// --- Modül durumu ---
static WebServer     s_server(80);
static AgDurumu      s_agDurumu = AgDurumu::BASLATILMADI;
static unsigned long s_baglanmaBaslangic = 0;
static unsigned long s_sonYenidenDeneme = 0;
static bool          s_sunucuBasladi = false;

// Paylaşılan kontrol durumu (API <-> loop). Her özellik BAĞIMSIZ oto/manuel:
// kullanıcı fanı otomatik tutup LED'i manuel ayarlayabilir (veya tersi).
static bool s_fanOto    = true;   // fan: true=sensör karar verir, false=manuel
static bool s_ledOto    = true;   // LED: true=ışığa göre, false=manuel parlaklık
static bool s_manuelFan = false;
static int  s_manuelLed = 0;
static DurumKaydi s_durum = {};

// Zaman sabitleri
constexpr unsigned long WIFI_BAGLANTI_ZAMANASIMI = 20000UL;  // 20 sn bağlanamazsa HATA
constexpr unsigned long WIFI_YENIDEN_DENEME       = 30000UL;  // 30 sn sonra tekrar dene
constexpr size_t        MAKS_GOVDE_BAYT           = 512;      // istek gövdesi üst sınır

// --- Loop'un okuyacağı erişimciler ---
inline bool    agFanOto()      { return s_fanOto; }
inline bool    agLedOto()      { return s_ledOto; }
inline bool    agManuelFan()   { return s_manuelFan; }
inline int     agManuelLed()   { return s_manuelLed; }
inline bool    agBagli()       { return s_agDurumu == AgDurumu::BAGLI; }
inline uint8_t agIpSonOktet()  { return agBagli() ? WiFi.localIP()[3] : 0; }
inline void    agDurumGuncelle(const DurumKaydi& d) { s_durum = d; }

// --- Yardımcılar ---
// Yanıt başlıkları: nosniff (MIME sniff/XSS savunması, ucuz) + CORS.
// CORS "*" yalnızca yerel ağ içi geliştirme testi için. ÜRETİMDE KALDIRILMALI.
inline void corsBasliklari() {
  s_server.sendHeader(F("X-Content-Type-Options"), F("nosniff"));
  s_server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
}
inline void jsonHata(int kod, const char* mesaj) {
  JsonDocument doc;
  doc["hata"] = mesaj;
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(kod, F("application/json"), cikti);
}
inline void jsonOk(const char* mesaj) {
  JsonDocument doc;
  doc["sonuc"] = mesaj;
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(200, F("application/json"), cikti);
}
// POST gövdesini al + doğrula. Başarılıysa doc'a yazar ve true döner; değilse
// uygun hata kodunu gönderip false döner (413 boyut, 400 ayrıştırma).
inline bool govdeyiAl(JsonDocument& doc) {
  String govde = s_server.hasArg("plain") ? s_server.arg("plain") : String();
  if (govde.length() > MAKS_GOVDE_BAYT) { jsonHata(413, "Govde cok buyuk"); return false; }
  if (deserializeJson(doc, govde))      { jsonHata(400, "JSON ayristirilamadi"); return false; }
  return true;
}

// --- Uç nokta işleyicileri ---
inline void handleRoot() {
  corsBasliklari();
  // send_P: HTML'i PROGMEM'den doğrudan servis eder, RAM'e KOPYALAMAZ (heap tasarrufu).
  s_server.send_P(200, "text/html", INDEX_HTML);
}

inline void handleDurum() {
  JsonDocument doc;
  doc["sicaklik"]      = s_durum.sicaklik;
  doc["nem"]           = s_durum.nem;
  doc["toprakHam"]     = s_durum.toprakHam;
  doc["toprakYuzde"]   = s_durum.toprakYuzde;
  doc["isikHam"]       = s_durum.isikHam;
  doc["isikYuzde"]     = s_durum.isikYuzde;
  doc["pompa"]         = s_durum.pompa;
  doc["fan"]           = s_durum.fan;
  doc["led"]           = s_durum.led;
  doc["alarm"]         = s_durum.alarm;
  doc["fanOto"]        = s_fanOto;
  doc["ledOto"]        = s_ledOto;
  doc["dhtGecerli"]    = s_durum.dhtGecerli;
  doc["rssi"]          = WiFi.RSSI();
  doc["calismaSuresi"] = millis();
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(200, F("application/json"), cikti);
}

inline void handleSula() {
  pompayiTetikle(millis());          // güvenlik kilidi (60 sn) içeride
  jsonOk("Sulama tetiklendi");
}

inline void handleFan() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["acik"].is<bool>()) { jsonHata(400, "acik bool olmali"); return; }
  s_manuelFan = doc["acik"];
  s_fanOto = false;                  // fanı elle ayarlamak fanı manuel moda alır
  jsonOk("Fan ayarlandi");
}

inline void handleLed() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["parlaklik"].is<int>()) { jsonHata(400, "parlaklik sayi olmali"); return; }
  int p = doc["parlaklik"];
  if (!parlaklikGecerliMi(p)) { jsonHata(400, "parlaklik 0-255 araliginda olmali"); return; }
  s_manuelLed = p;
  s_ledOto = false;                  // LED'i elle ayarlamak LED'i manuel moda alır
  jsonOk("LED ayarlandi");
}

// Her özellik ayrı: gövde {"fan":bool} ve/veya {"led":bool} (true=otomatik).
// Yalnızca verilen alanları günceller; en az biri gerekli.
inline void handleMod() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  bool degisti = false;
  if (doc["fan"].is<bool>()) { s_fanOto = doc["fan"]; degisti = true; }
  if (doc["led"].is<bool>()) { s_ledOto = doc["led"]; degisti = true; }
  if (!degisti) { jsonHata(400, "fan veya led (bool) gerekli"); return; }
  jsonOk("Mod ayarlandi");
}

inline void handleSaglik() {
  JsonDocument doc;
  doc["calismaSuresi"] = millis();
  doc["bosHeap"]       = ESP.getFreeHeap();
  doc["rssi"]          = WiFi.RSSI();
  doc["dhtHata"]       = s_durum.dhtHata;
  doc["agDurumu"]      = agBagli() ? "BAGLI" : "OTONOM";
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(200, F("application/json"), cikti);
}

inline void handleNotFound() {
  jsonHata(404, "Ucnokta bulunamadi");
}

// mDNS: yalnızca AYNI yerel ağda çalışır -> http://akillisera.local
// Farklı ağdan erişim Faz 8'deki ThingSpeak TalkBack ile sağlanır (rapor Bölüm 7).
inline void mdnsBaslat() {
  if (MDNS.begin("akillisera")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(F("mDNS: http://akillisera.local"));
  } else {
    Serial.println(F("mDNS baslatilamadi (IP ile erisim calisir)"));
  }
}

inline void sunucuBaslat() {
  if (s_sunucuBasladi) return;
  s_server.on("/",            handleRoot);
  s_server.on("/api/durum",   HTTP_GET,  handleDurum);
  s_server.on("/api/sula",    HTTP_POST, handleSula);
  s_server.on("/api/fan",     HTTP_POST, handleFan);
  s_server.on("/api/led",     HTTP_POST, handleLed);
  s_server.on("/api/mod",     HTTP_POST, handleMod);
  s_server.on("/api/saglik",  HTTP_GET,  handleSaglik);
  s_server.onNotFound(handleNotFound);
  s_server.begin();
  s_sunucuBasladi = true;
}

// Bağlantı KURMA — bloklamaz, yalnızca başlatır.
inline void agBaslat() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("akillisera");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  s_agDurumu = AgDurumu::BAGLANIYOR;
  s_baglanmaBaslangic = millis();
  Serial.println(F("WiFi baglaniyor (bloklamadan)..."));
}

// Her loop turunda çağrılır. Durum makinesi; asla beklemez.
inline AgDurumu agGuncelle(unsigned long simdi) {
  wl_status_t st = WiFi.status();
  switch (s_agDurumu) {
    case AgDurumu::BASLATILMADI:
      break;
    case AgDurumu::BAGLANIYOR:
      if (st == WL_CONNECTED) {
        s_agDurumu = AgDurumu::BAGLI;
        Serial.print(F("WiFi BAGLI, IP: ")); Serial.println(WiFi.localIP());
        mdnsBaslat();
        sunucuBaslat();
      } else if (simdi - s_baglanmaBaslangic >= WIFI_BAGLANTI_ZAMANASIMI) {
        s_agDurumu = AgDurumu::HATA;
        s_sonYenidenDeneme = simdi;
        Serial.println(F("WiFi baglanamadi -> OTONOM mod"));
      }
      break;
    case AgDurumu::BAGLI:
      if (st != WL_CONNECTED) {
        s_agDurumu = AgDurumu::HATA;
        s_sonYenidenDeneme = simdi;
        Serial.println(F("WiFi koptu -> OTONOM mod"));
      }
      break;
    case AgDurumu::HATA:
      // Sabit 30 sn aralık yeterli: sera zaten otonom çalışıyor, agresif yeniden
      // deneme radyoyu ve gücü boşa harcar. Üstel geri çekilmeye gerek yok.
      if (simdi - s_sonYenidenDeneme >= WIFI_YENIDEN_DENEME) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        s_agDurumu = AgDurumu::BAGLANIYOR;
        s_baglanmaBaslangic = simdi;
        Serial.println(F("WiFi yeniden deneniyor..."));
      }
      break;
  }
  return s_agDurumu;
}

// İstemci isteklerini işle — loop()'ta her turda, gecikmesiz.
inline void agIsle() {
  if (s_sunucuBasladi) s_server.handleClient();
}
