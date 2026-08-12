// akillisera.ino — ESP32 Akıllı Sera firmware orkestrasyonu
// Faz 6: WiFi + mDNS + WebServer. loop()'ta delay()/while-bekleme YOK.

#include "config.h"
#include "control_logic.h"
#include "sensors.h"
#include "actuators.h"
#include "display.h"
#include "buttons.h"
#include "agkatmani.h"   // NOT: "network.h" macOS case-insensitive FS'de core'un Network.h ile cakisir

void setup() {
  Serial.begin(115200);
  eyleyicileriBaslat();
  sensorlerBaslat();
  butonlariBaslat();
  ekraniBaslat();
  planBaslat();           // zamanlı sulama planını flash'tan yükle
  agBaslat();             // bloklamaz — bağlantı arka planda
  Serial.println(F("Akilli Sera — ag katmani hazir"));
}

void loop() {
  unsigned long simdi = millis();

  // Ağ + pompa + butonlar: her turda, gecikmesiz.
  agGuncelle(simdi);
  agIsle();
  planGuncelle(simdi);      // zamanlı sulama — saat plana denk gelince tetikler
  pompayiGuncelle(simdi);
  if (sayfaButonunaBasildi(simdi))  sayfaDegistir();
  if (sulamaButonunaBasildi(simdi)) pompayiTetikle(simdi);

  static unsigned long sonSensorOkuma = 0;
  static bool          fanDurum = false;
  static EyleyiciKarari karar = {false, 0, false};
  static EkranVerisi   ekran = {};

  if (simdi - sonSensorOkuma >= SENSOR_READ_INTERVAL) {
    sonSensorOkuma = simdi;
    SensorVerisi v = tumSensorleriOku();

    // Otomatik sulama her zaman aktif (güvenlik kilidi 60 sn içeride).
    // Manuel sulama buton/API'den her koşulda çalışır.
    if (sulamaGerekli(v.toprakHam, pompaCalisiyor()))
      pompayiTetikle(simdi);

    // Fan: bağımsız oto/manuel.
    if (agFanOto()) {
      fanDurum      = fanKarari(v.nem, v.sicaklik, fanDurum);
      karar.fanAcik = fanDurum;
    } else {
      karar.fanAcik = agManuelFan();
    }

    // LED: bağımsız oto/manuel.
    if (agLedOto()) karar.ledParlaklik = ledParlaklikKarari(v.isikHam, false, 0);
    else            karar.ledParlaklik = ledParlaklikKarari(0, true, agManuelLed());

    karar.alarmAktif = alarmKarari(v.sicaklik);      // alarm her koşulda güvenlik

    // Ekran + API durum snapshot
    ekran.sicaklik    = v.sicaklik;
    ekran.nem         = v.nem;
    ekran.toprakYuzde = toprakNemYuzde(v.toprakHam);
    ekran.fanAcik     = karar.fanAcik;
    ekran.ledParlaklik = karar.ledParlaklik;
    ekran.dhtArizali  = !v.dhtGecerli;
    ekran.wifiVar     = agBagli();
    ekran.ipSonOktet  = agIpSonOktet();

    DurumKaydi dk;
    dk.sicaklik = v.sicaklik; dk.nem = v.nem;
    dk.toprakHam = v.toprakHam; dk.toprakYuzde = ekran.toprakYuzde;
    dk.isikHam = v.isikHam;     dk.isikYuzde = isikYuzde(v.isikHam);
    dk.pompa = pompaCalisiyor(); dk.fan = karar.fanAcik; dk.led = karar.ledParlaklik;
    dk.alarm = karar.alarmAktif; dk.dhtGecerli = v.dhtGecerli; dk.dhtHata = dhtHataSayisi();
    agDurumGuncelle(dk);
  }

  ekran.pompaCalisiyor = pompaCalisiyor();
  ekran.pompaKalanSn   = pompaKalanSaniye(simdi);

  eyleyicileriUygula(karar, simdi);
  ekraniGuncelle(ekran, simdi);
}
