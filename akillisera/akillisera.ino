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
  // Gündüz/gece (NTP): 20:00-06:00 gece. Saat senkron değilse gündüz varsay.
  int gS, gD, gSn; bool gece = false;
  if (suankiZaman(gS, gD, gSn)) gece = (gS < 6 || gS >= 20);
  // Seçili bitki + büyüme evresi + mevsim + gündüz/gece -> ETKİN ideal bantlar.
  EtkinParam ep = etkinParam(bitkiParam(bitkiAl()), evreAl(), mevsimAl(), gece);

  pompaSuresiAyarla(ep.sulamaMs);   // sulama MİKTARI bitki+evre+mevsime göre

  // Ağ + bulut + pompa: her turda, gecikmesiz.
  agGuncelle(simdi);
  agIsle();
  bulutGuncelle(simdi);
  pompayiGuncelle(simdi);

  // SULAMA modu: OTO = bitkinin optimum saatleri; MANUEL = kullanıcı planı.
  // Manuel WATER butonu her koşulda çalışır (elle sulama).
  if (kdSulamaOto()) bitkiOtoSulamaIsle(ep.otoSulama, ep.otoSulamaAdet, simdi);
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

    // OTO sulama: toprak ideal bandın ALTINA inince banda çek (bitkiye özel %).
    if (kdSulamaOto() && sulamaGerekliYuzde(toprakY, ep.sLo, pompaCalisiyor()))
      pompayiTetikle(simdi);

    // Fan: BANT hedefli — sıcaklık/nem bandın üstüne çıkınca havalandır, banda döndür.
    if (agFanOto()) {
      fanDurum      = fanKarariBant(v.nem, v.sicaklik, fanDurum, ep.hLo, ep.hHi, ep.tLo, ep.tHi);
      karar.fanAcik = fanDurum;
    } else {
      karar.fanAcik = agManuelFan();
    }

    // LED: gece söndür (fotoperiyot/dinlenme); gündüz ışık bandın altındaysa yak.
    if (agLedOto()) karar.ledParlaklik = (!gece && isikY < ep.lLo) ? LED_BRIGHTNESS : 0;
    else            karar.ledParlaklik = ledParlaklikKarari(0, true, agManuelLed());

    karar.alarmAktif = alarmKarari(v.sicaklik, ep.alarmHigh);   // bant+4C = tehlike

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
    dk.gece = gece; dk.vpd = v.dhtGecerli ? vpdKpa(v.sicaklik, v.nem) : 0.0f;
    kdDurumGuncelle(dk);
  }

  ekran.pompaCalisiyor = pompaCalisiyor();
  ekran.pompaKalanSn   = pompaKalanSaniye(simdi);

  eyleyicileriUygula(karar, simdi);
  ekraniGuncelle(ekran, simdi);

  kdLoopKaydet(micros() - loopBasla);   // en yüksek loop süresi (/api/saglik)
}
