// sensors.h — TÜM sensör okumaları burada kapsüllenir.
// Başka hiçbir dosyada analogRead() / dht.read*() çağrısı YOK.
// Tüm fonksiyonlar inline, modül durumu static (global namespace kirletilmez).
#pragma once

#include <DHT.h>
#include "config.h"
#include "control_logic.h"

// --- Modül durumu ---
static DHT s_dht(DHT_PIN, DHT11);
static unsigned long s_dhtSonOkuma = 0;   // son DHT örneklemesi (ms)
static int  s_dhtHataSayaci = 0;          // ardışık başarısız okuma
static bool s_dhtIlkOkuma = true;         // ilk çağrıda hız sınırını atla

// Tüm sensörleri başlat. Analog pinler için pinMode gerekmez.
inline void sensorlerBaslat() {
  s_dht.begin();
}

// DHT11 oku. Hız sınırlı (>=2000 ms), NaN filtreli.
// Geçersiz/erken çağrıda referanslar DEĞİŞMEZ (son geçerli değer korunur).
// Yeni geçerli okuma yapıldığında true, filtrelenen NaN'de false döner.
inline bool dhtOku(float& sicaklik, float& nem) {
  unsigned long simdi = millis();
  // Hız sınırı: 2 sn dolmadıysa okuma yapma, eski değeri koru.
  if (!s_dhtIlkOkuma && (simdi - s_dhtSonOkuma) < SENSOR_READ_INTERVAL) {
    return true;
  }
  s_dhtSonOkuma = simdi;
  s_dhtIlkOkuma = false;

  float t = s_dht.readTemperature();
  float h = s_dht.readHumidity();
  if (!sensorDegeriGecerli(t) || !sensorDegeriGecerli(h)) {
    if (s_dhtHataSayaci < DHT_MAX_FAILS) s_dhtHataSayaci++;
    return false;                 // referanslara DOKUNMA — son geçerli değer korunur
  }
  s_dhtHataSayaci = 0;
  sicaklik = t;
  nem = h;
  return true;
}

// Ardışık DHT_MAX_FAILS (5) başarısız okuma -> sensör arızalı.
inline bool dhtArizali() {
  return s_dhtHataSayaci >= DHT_MAX_FAILS;
}

// Analog okuma medyan filtresi. Ortalama tek sıçramadan etkilenir; medyan etkilenmez
// (elektriksel gürültü, pompa rölesi çekerken besleme dalgalanması).
inline int analogOkuMedyan(uint8_t pin, uint8_t ornekSayisi = 5) {
  constexpr uint8_t MAKS = 15;
  if (ornekSayisi < 1)    ornekSayisi = 1;
  if (ornekSayisi > MAKS) ornekSayisi = MAKS;

  int buf[MAKS];
  for (uint8_t i = 0; i < ornekSayisi; i++) {
    buf[i] = analogRead(pin);
    delayMicroseconds(2000);      // delay() DEĞİL — loop'ta güvenli, toplam ~10 ms
  }
  // Insertion sort (küçük dizi)
  for (uint8_t i = 1; i < ornekSayisi; i++) {
    int anahtar = buf[i];
    int j = i - 1;
    while (j >= 0 && buf[j] > anahtar) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = anahtar;
  }
  return buf[ornekSayisi / 2];
}

inline int toprakNemOku() { return analogOkuMedyan(SOIL_PIN); }
inline int isikOku()      { return analogOkuMedyan(LDR_PIN); }

// Ham -> yüzde dönüşümleri (yalnızca arayüzde göstermek için; karar mantığı ham kullanır).
inline int toprakNemYuzde(int hamDeger) {
  // Kapasitif: ıslak=düşük ham -> %100, kuru=yüksek ham -> %0
  int y = map(hamDeger, SOIL_RAW_WET, SOIL_RAW_DRY, 100, 0);
  return constrain(y, 0, 100);
}

inline int isikYuzde(int hamDeger) {
  int y = map(hamDeger, 0, 4095, 0, 100);
  return constrain(y, 0, 100);
}

// --- Toplu okuma ---
struct SensorVerisi {
  float sicaklik;
  float nem;
  int   toprakHam;
  int   isikHam;
  bool  dhtGecerli;
};

// loop() bunu SENSOR_READ_INTERVAL periyodunda çağırır.
// sicaklik/nem son geçerli değerleri korumak için static tutulur.
inline SensorVerisi tumSensorleriOku() {
  static float sonSicaklik = 0.0f;
  static float sonNem = 0.0f;
  bool gecerli = dhtOku(sonSicaklik, sonNem);
  SensorVerisi v;
  v.sicaklik   = sonSicaklik;
  v.nem        = sonNem;
  v.toprakHam  = toprakNemOku();
  v.isikHam    = isikOku();
  v.dhtGecerli = gecerli && !dhtArizali();
  return v;
}
