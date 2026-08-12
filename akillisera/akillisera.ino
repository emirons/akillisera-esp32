// akillisera.ino — ESP32 Akıllı Sera firmware orkestrasyonu
// Faz 4: eyleyici katmanı. Zamanlama millis() tabanlı, loop'ta delay() YOK.

#include "config.h"
#include "control_logic.h"
#include "sensors.h"
#include "actuators.h"

void setup() {
  Serial.begin(115200);
  eyleyicileriBaslat();   // önce eyleyiciler — röleler güvenli (RELAY_OFF) başlar
  sensorlerBaslat();
  Serial.println(F("Akilli Sera — eyleyici katmani hazir"));
}

void loop() {
  unsigned long simdi = millis();

  // Pompa durum makinesi: her turda, gecikmesiz (5 sn dolunca kapatır).
  pompayiGuncelle(simdi);

  static unsigned long sonSensorOkuma = 0;
  static bool          fanDurum = false;                 // histerezis için önceki durum
  static EyleyiciKarari karar = {false, 0, false};        // en son hesaplanan karar

  if (simdi - sonSensorOkuma >= SENSOR_READ_INTERVAL) {
    sonSensorOkuma = simdi;
    SensorVerisi v = tumSensorleriOku();

    // Otomatik sulama (güvenlik kilidi pompayiTetikle içinde)
    if (sulamaGerekli(v.toprakHam, pompaCalisiyor())) pompayiTetikle(simdi);

    // Kararlar — control_logic.h saf mantığı
    fanDurum          = fanKarari(v.nem, v.sicaklik, fanDurum);
    karar.fanAcik     = fanDurum;
    karar.ledParlaklik = ledParlaklikKarari(v.isikHam, false, 0);   // Faz 7'de manuel mod
    karar.alarmAktif  = alarmKarari(v.sicaklik);

    // Seri log
    Serial.print(F("T:"));    Serial.print(v.sicaklik);
    Serial.print(F(" H:"));   Serial.print(v.nem);
    Serial.print(F(" Toprak:")); Serial.print(v.toprakHam);
    Serial.print(F(" Isik:")); Serial.print(v.isikHam);
    Serial.print(F(" Pompa:")); Serial.print(pompaCalisiyor());
    Serial.print(F(" Fan:"));  Serial.print(karar.fanAcik);
    Serial.print(F(" LED:"));  Serial.print(karar.ledParlaklik);
    Serial.print(F(" Alarm:")); Serial.print(karar.alarmAktif);
    if (!v.dhtGecerli) Serial.print(F(" [DHT ARIZALI]"));
    Serial.println();
  }

  // Kararı her turda uygula: fan/LED idempotent (değişmezse yazmaz),
  // alarm buzzer/LED 500 ms yanıp sönmesi için sık çağrı gerekir.
  eyleyicileriUygula(karar, simdi);
}
