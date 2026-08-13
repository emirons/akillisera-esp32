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
#include "kontrol_durumu.h"
#include "actuators.h"
#include "zaman.h"
#include "sulama_plani.h"
#include "bitki.h"
#include "bitki_veri.h"
#include "web_ui.h"
#include "secrets.h"

enum class AgDurumu { BASLATILMADI, BAGLANIYOR, BAGLI, HATA };

// --- Modül durumu ---
static WebServer     s_server(80);
static AgDurumu      s_agDurumu = AgDurumu::BASLATILMADI;
static unsigned long s_baglanmaBaslangic = 0;
static unsigned long s_sonYenidenDeneme = 0;
static bool          s_sunucuBasladi = false;

// Kontrol durumu (fan/LED modları, durum snapshot) kontrol_durumu.h'da (kd*).

// Zaman sabitleri
constexpr unsigned long WIFI_BAGLANTI_ZAMANASIMI = 20000UL;  // 20 sn bağlanamazsa HATA
constexpr unsigned long WIFI_YENIDEN_DENEME       = 30000UL;  // 30 sn sonra tekrar dene
constexpr size_t        MAKS_GOVDE_BAYT           = 512;      // istek gövdesi üst sınır

// --- Loop'un okuyacağı erişimciler (kontrol durumu kd* delege) ---
inline bool    agFanOto()      { return kdFanOto(); }
inline bool    agLedOto()      { return kdLedOto(); }
inline bool    agManuelFan()   { return kdManuelFan(); }
inline int     agManuelLed()   { return kdManuelLed(); }
inline bool    agBagli()       { return s_agDurumu == AgDurumu::BAGLI; }
inline uint8_t agIpSonOktet()  { return agBagli() ? WiFi.localIP()[3] : 0; }

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
  const DurumKaydi& d = kdDurum();
  JsonDocument doc;
  doc["sicaklik"]      = d.sicaklik;
  doc["nem"]           = d.nem;
  doc["toprakHam"]     = d.toprakHam;
  doc["toprakYuzde"]   = d.toprakYuzde;
  doc["isikHam"]       = d.isikHam;
  doc["isikYuzde"]     = d.isikYuzde;
  doc["pompa"]         = d.pompa;
  doc["fan"]           = d.fan;
  doc["led"]           = d.led;
  doc["alarm"]         = d.alarm;
  doc["fanOto"]        = kdFanOto();
  doc["ledOto"]        = kdLedOto();
  doc["sulamaOto"]     = kdSulamaOto();
  // AUTO sulama modunda o bitkinin optimum saatleri (UI'de göstermek için)
  const BitkiParam& bp = bitkiParam(bitkiAl());
  JsonArray os = doc["otoSulama"].to<JsonArray>();
  for (int i = 0; i < bp.otoSulamaAdet; i++) {
    JsonObject o = os.add<JsonObject>();
    o["saat"] = bp.otoSulama[i].saat;
    o["dakika"] = bp.otoSulama[i].dakika;
  }
  doc["dhtGecerli"]    = d.dhtGecerli;
  doc["gece"]          = d.gece;
  doc["vpd"]           = d.vpd;
  doc["rssi"]          = WiFi.RSSI();
  doc["tsKanal"]       = TS_CHANNEL_ID;   // public channel ID (write key gizli kalır)
  doc["bitki"]         = bitkiAl();
  doc["evre"]          = evreAl();
  doc["mevsim"]        = mevsimAl();
  char sbuf[12]; zamanMetni(sbuf, sizeof(sbuf));
  doc["saat"]          = sbuf;
  doc["calismaSuresi"] = millis();
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(200, F("application/json"), cikti);
}

// GET /api/plan -> zamanlı sulama listesi
inline void handlePlan() {
  JsonDocument doc;
  JsonArray arr = doc["zamanlar"].to<JsonArray>();
  for (int i = 0; i < planAdet(); i++) {
    SulamaZamani z = planAl(i);
    JsonObject o = arr.add<JsonObject>();
    o["saat"] = z.saat;
    o["dakika"] = z.dakika;
  }
  String cikti;
  serializeJson(doc, cikti);
  corsBasliklari();
  s_server.send(200, F("application/json"), cikti);
}

// POST /api/plan {saat,dakika} -> ekle
inline void handlePlanEkle() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["saat"].is<int>() || !doc["dakika"].is<int>()) { jsonHata(400, "saat ve dakika sayi olmali"); return; }
  int s = doc["saat"], d = doc["dakika"];
  if (!zamanGecerliMi(s, d)) { jsonHata(400, "saat 0-23, dakika 0-59 olmali"); return; }
  if (!planEkle(s, d)) { jsonHata(400, "eklenemedi (dolu veya mukerrer)"); return; }
  jsonOk("Sulama zamani eklendi");
}

// GET /api/bitki -> seçili bitki; POST /api/bitki {bitki:"anahtar"} -> ayarla
// Kısa metin anahtarını güvenli doğrula (boş değil, < sınır).
inline bool anahtarGecerli(const char* k) { size_t n = strlen(k); return n > 0 && n < 16; }

// GET -> bitki/evre/mevsim; POST {bitki?,evre?,mevsim?} -> verilenleri ayarla.
inline void handleBitki() {
  if (s_server.method() == HTTP_POST) {
    JsonDocument doc;
    if (!govdeyiAl(doc)) return;
    bool degisti = false;
    if (doc["bitki"].is<const char*>())  { const char* v = doc["bitki"];  if (!anahtarGecerli(v)) { jsonHata(400,"gecersiz bitki"); return; } bitkiAyarla(v);  degisti = true; }
    if (doc["evre"].is<const char*>())   { const char* v = doc["evre"];   if (!anahtarGecerli(v)) { jsonHata(400,"gecersiz evre"); return; }  evreAyarla(v);   degisti = true; }
    if (doc["mevsim"].is<const char*>()) { const char* v = doc["mevsim"]; if (!anahtarGecerli(v)) { jsonHata(400,"gecersiz mevsim"); return; } mevsimAyarla(v); degisti = true; }
    if (!degisti) { jsonHata(400, "bitki, evre veya mevsim gerekli"); return; }
    jsonOk("Ayarlandi");
  } else {
    JsonDocument doc;
    doc["bitki"]  = bitkiAl();
    doc["evre"]   = evreAl();
    doc["mevsim"] = mevsimAl();
    String cikti; serializeJson(doc, cikti);
    corsBasliklari();
    s_server.send(200, F("application/json"), cikti);
  }
}

// DELETE /api/plan {index} -> sil
inline void handlePlanSil() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["index"].is<int>()) { jsonHata(400, "index sayi olmali"); return; }
  if (!planSil(doc["index"])) { jsonHata(400, "gecersiz index"); return; }
  jsonOk("Sulama zamani silindi");
}

inline void handleSula() {
  pompayiTetikle(millis());          // güvenlik kilidi (60 sn) içeride
  jsonOk("Sulama tetiklendi");
}

inline void handleFan() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["acik"].is<bool>()) { jsonHata(400, "acik bool olmali"); return; }
  kdFanElle(doc["acik"]);            // manuel fan + fanı otomatik dışına al
  jsonOk("Fan ayarlandi");
}

inline void handleLed() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  if (!doc["parlaklik"].is<int>()) { jsonHata(400, "parlaklik sayi olmali"); return; }
  int p = doc["parlaklik"];
  if (!parlaklikGecerliMi(p)) { jsonHata(400, "parlaklik 0-255 araliginda olmali"); return; }
  kdLedElle(p);                      // manuel LED + LED'i otomatik dışına al
  jsonOk("LED ayarlandi");
}

// Her özellik ayrı: gövde {"fan":bool} ve/veya {"led":bool} (true=otomatik).
// Yalnızca verilen alanları günceller; en az biri gerekli.
inline void handleMod() {
  JsonDocument doc;
  if (!govdeyiAl(doc)) return;
  bool degisti = false;
  if (doc["fan"].is<bool>())    { kdFanOtoAyarla(doc["fan"]);    degisti = true; }
  if (doc["led"].is<bool>())    { kdLedOtoAyarla(doc["led"]);    degisti = true; }
  if (doc["sulama"].is<bool>()) { kdSulamaOtoAyarla(doc["sulama"]); degisti = true; }
  if (!degisti) { jsonHata(400, "fan, led veya sulama (bool) gerekli"); return; }
  jsonOk("Mod ayarlandi");
}

inline void handleSaglik() {
  JsonDocument doc;
  doc["calismaSuresi"]     = millis();
  doc["bosHeap"]           = ESP.getFreeHeap();
  doc["enDusukHeap"]       = ESP.getMinFreeHeap();
  doc["maxLoopMikrosaniye"] = kdMaxLoopUs();
  doc["rssi"]              = WiFi.RSSI();
  doc["dhtHata"]           = kdDurum().dhtHata;
  doc["agDurumu"]          = agBagli() ? "BAGLI" : "OTONOM";
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
  s_server.on("/api/saglik",  HTTP_GET,    handleSaglik);
  s_server.on("/api/plan",    HTTP_GET,    handlePlan);
  s_server.on("/api/plan",    HTTP_POST,   handlePlanEkle);
  s_server.on("/api/plan",    HTTP_DELETE, handlePlanSil);
  s_server.on("/api/bitki",   HTTP_GET,    handleBitki);
  s_server.on("/api/bitki",   HTTP_POST,   handleBitki);
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
        zamanBaslat();       // NTP senkronu başlat (zamanlı sulama için)
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
