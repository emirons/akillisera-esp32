// akillisera.ino — ESP32 Akıllı Sera firmware orkestrasyonu
// Faz 3: sensör katmanı eklendi. Zamanlama millis() tabanlı, delay() YOK.

#include "config.h"
#include "control_logic.h"
#include "sensors.h"

static unsigned long sonSensorOkuma = 0;

void setup() {
  Serial.begin(115200);
  sensorlerBaslat();
  Serial.println(F("Akilli Sera — sensor katmani hazir"));
}

void loop() {
  unsigned long simdi = millis();

  if (simdi - sonSensorOkuma >= SENSOR_READ_INTERVAL) {
    sonSensorOkuma = simdi;
    SensorVerisi v = tumSensorleriOku();

    Serial.print(F("Sicaklik: "));
    Serial.print(v.sicaklik);
    Serial.print(F(" C  Nem: "));
    Serial.print(v.nem);
    Serial.print(F(" %  ToprakHam: "));
    Serial.print(v.toprakHam);
    Serial.print(F(" ("));
    Serial.print(toprakNemYuzde(v.toprakHam));
    Serial.print(F("%)  IsikHam: "));
    Serial.print(v.isikHam);
    Serial.print(F(" ("));
    Serial.print(isikYuzde(v.isikHam));
    Serial.print(F("%)"));
    if (!v.dhtGecerli) Serial.print(F("  [DHT ARIZALI]"));
    Serial.println();
  }

}
