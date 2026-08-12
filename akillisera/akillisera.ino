// akillisera.ino — ESP32 Akıllı Sera firmware orkestrasyonu
// Faz 5: LCD + butonlar. Zamanlama millis() tabanlı, loop'ta delay() YOK.

#include "config.h"
#include "control_logic.h"
#include "sensors.h"
#include "actuators.h"
#include "display.h"
#include "buttons.h"

void setup() {
  Serial.begin(115200);
  eyleyicileriBaslat();   // önce eyleyiciler — röleler güvenli (RELAY_OFF) başlar
  sensorlerBaslat();
  butonlariBaslat();
  ekraniBaslat();         // içinde tek istisna delay(1500) açılış mesajı
  Serial.println(F("Akilli Sera — LCD ve butonlar hazir"));
}

void loop() {
  unsigned long simdi = millis();

  // Pompa durum makinesi ve butonlar: her turda, gecikmesiz.
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

    if (sulamaGerekli(v.toprakHam, pompaCalisiyor())) pompayiTetikle(simdi);

    fanDurum           = fanKarari(v.nem, v.sicaklik, fanDurum);
    karar.fanAcik      = fanDurum;
    karar.ledParlaklik = ledParlaklikKarari(v.isikHam, false, 0);
    karar.alarmAktif   = alarmKarari(v.sicaklik);

    // Ekran verisini güncelle (sensör periyodunda değişen alanlar)
    ekran.sicaklik    = v.sicaklik;
    ekran.nem         = v.nem;
    ekran.toprakYuzde = toprakNemYuzde(v.toprakHam);
    ekran.fanAcik     = karar.fanAcik;
    ekran.ledParlaklik = karar.ledParlaklik;
    ekran.dhtArizali  = !v.dhtGecerli;
    ekran.wifiVar     = false;          // Faz 6'da doldurulacak
    ekran.ipSonOktet  = 0;
  }

  // Sık değişen alanlar her turda
  ekran.pompaCalisiyor = pompaCalisiyor();
  ekran.pompaKalanSn   = pompaKalanSaniye(simdi);

  eyleyicileriUygula(karar, simdi);       // alarm blink için her tur
  ekraniGuncelle(ekran, simdi);           // içeride 500 ms periyot
}
