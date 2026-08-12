// bitki_veri.h — ESP32 üstünde bitki KONTROL parametreleri. Otonom kararlar
// (sulama/fan/LED/alarm) seçili bitkinin eşiklerini kullanır. Web arayüzündeki
// bilgi tabanının kontrol karşılığı. Bahçecilik referanslarından derlenmiştir.
#pragma once

#include "config.h"
#include "control_logic.h"

struct BitkiParam {
  const char*  anahtar;
  float        tempHigh;        // C — üstünde fan + alarm
  float        humHigh, humLow; // % — fan histerezisi
  int          soilDryYuzde;    // toprak % ALTINDA -> kuru -> sula
  int          lightLowYuzde;   // ışık % ALTINDA -> LED yak
  SulamaZamani otoSulama[4];    // AUTO modda optimum sulama saatleri
  int          otoSulamaAdet;
};

// Sıralar web_ui PLANTS bilgi tabanıyla uyumlu (ideal aralık orta/üst değerleri).
static const BitkiParam BITKILER[] = {
  {"custom",     35.0f, 70.0f, 60.0f, 30, 30, {{8,0}},                    1},
  {"tomato",     30.0f, 70.0f, 55.0f, 50, 60, {{8,0},{18,0}},             2},
  {"lettuce",    24.0f, 70.0f, 55.0f, 60, 40, {{7,0},{12,0},{17,0}},      3},
  {"pepper",     33.0f, 70.0f, 55.0f, 45, 60, {{8,0}},                    1},
  {"cucumber",   30.0f, 80.0f, 65.0f, 60, 60, {{7,0},{13,0},{19,0}},      3},
  {"basil",      30.0f, 60.0f, 45.0f, 40, 55, {{8,0}},                    1},
  {"strawberry", 27.0f, 70.0f, 55.0f, 50, 55, {{8,0},{17,0}},             2},
  {"spinach",    24.0f, 70.0f, 55.0f, 55, 40, {{7,0},{17,0}},             2},
};
constexpr int BITKI_SAYISI = sizeof(BITKILER) / sizeof(BITKILER[0]);

// Anahtara göre bitki parametresi; bulunmazsa custom (indeks 0).
inline const BitkiParam& bitkiParam(const char* anahtar) {
  for (int i = 0; i < BITKI_SAYISI; i++) {
    if (strcmp(BITKILER[i].anahtar, anahtar) == 0) return BITKILER[i];
  }
  return BITKILER[0];
}
