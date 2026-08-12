// akillisera.ino — ESP32 Akıllı Sera firmware orkestrasyonu
// Otonom kontrol SEÇİLİ BİTKİYE göre (bitki_veri.h). loop()'ta delay/while YOK.

#include "config.h"
#include "control_logic.h"
#include "sensors.h"
#include "actuators.h"
#include "display.h"
#include "buttons.h"
#include "agkatmani.h"   // NOT: "network.h" macOS case-insensitive FS'de core'un Network.h ile cakisir
#include "cloud.h"

void setup() {
  Serial.begin(115200);
  eyleyicileriBaslat();
  sensorlerBaslat();
  butonlariBaslat();
  ekraniBaslat();
  planBaslat();           // kullanıcı sulama planını flash'tan yükle
  bitkiYukle();            // seçili bitki türünü flash'tan yükle
  agBaslat();             // bloklamaz — bağlantı arka planda
  Serial.println(F("Akilli Sera — ag katmani hazir"));
}

void loop() {
  unsigned long loopBasla = micros();
  unsigned long simdi = millis();
  const BitkiParam& bp = bitkiParam(bitkiAl());   // seçili bitkinin kontrol eşikleri

  // Ağ + bulut + pompa: her turda, gecikmesiz.
  agGuncelle(simdi);
  agIsle();
  bulutGuncelle(simdi);
  pompayiGuncelle(simdi);

  // SULAMA modu: OTO = bitkinin optimum saatleri; MANUEL = kullanıcı planı.
  // Manuel WATER butonu her koşulda çalışır (elle sulama).
  if (kdSulamaOto()) bitkiOtoSulamaIsle(bp.otoSulama, bp.otoSulamaAdet, simdi);
  else               planGuncelle(simdi);

  if (sayfaButonunaBasildi(simdi))  sayfaDegistir();
  if (sulamaButonunaBasildi(simdi)) pompayiTetikle(simdi);

  static unsigned long sonSensorOkuma = 0;
  static bool          fanDurum = false;
  static EyleyiciKarari karar = {false, 0, false};
  static EkranVerisi   ekran = {};

  if (simdi - sonSensorOkuma >= SENSOR_READ_INTERVAL) {
    sonSensorOkuma = simdi;
    SensorVerisi v = tumSensorleriOku();
    int toprakY = toprakNemYuzde(v.toprakHam);
    int isikY   = isikYuzde(v.isikHam);

    // OTO sulama modunda toprak-eşiği güvenlik sulaması (bitkiye özel %).
    if (kdSulamaOto() && sulamaGerekliYuzde(toprakY, bp.soilDryYuzde, pompaCalisiyor()))
      pompayiTetikle(simdi);

    // Fan: bitki histerezis eşikleriyle (oto), yoksa manuel.
    if (agFanOto()) {
      fanDurum      = fanKarari(v.nem, v.sicaklik, fanDurum, bp.humHigh, bp.humLow, bp.tempHigh);
      karar.fanAcik = fanDurum;
    } else {
      karar.fanAcik = agManuelFan();
    }

    // LED: bitki ışık eşiği % (oto), yoksa manuel parlaklık.
    if (agLedOto()) karar.ledParlaklik = (isikY < bp.lightLowYuzde) ? LED_BRIGHTNESS : 0;
    else            karar.ledParlaklik = ledParlaklikKarari(0, true, agManuelLed());

    karar.alarmAktif = alarmKarari(v.sicaklik, bp.tempHigh);   // bitki sıcaklık eşiği

    // Ekran + API durum snapshot
    ekran.sicaklik    = v.sicaklik;
    ekran.nem         = v.nem;
    ekran.toprakYuzde = toprakY;
    ekran.fanAcik     = karar.fanAcik;
    ekran.ledParlaklik = karar.ledParlaklik;
    ekran.dhtArizali  = !v.dhtGecerli;
    ekran.wifiVar     = agBagli();
    ekran.ipSonOktet  = agIpSonOktet();

    DurumKaydi dk;
    dk.sicaklik = v.sicaklik; dk.nem = v.nem;
    dk.toprakHam = v.toprakHam; dk.toprakYuzde = toprakY;
    dk.isikHam = v.isikHam;     dk.isikYuzde = isikY;
    dk.pompa = pompaCalisiyor(); dk.fan = karar.fanAcik; dk.led = karar.ledParlaklik;
    dk.alarm = karar.alarmAktif; dk.dhtGecerli = v.dhtGecerli; dk.dhtHata = dhtHataSayisi();
    kdDurumGuncelle(dk);
  }

  ekran.pompaCalisiyor = pompaCalisiyor();
  ekran.pompaKalanSn   = pompaKalanSaniye(simdi);

  eyleyicileriUygula(karar, simdi);
  ekraniGuncelle(ekran, simdi);

  kdLoopKaydet(micros() - loopBasla);   // en yüksek loop süresi (/api/saglik)
}
