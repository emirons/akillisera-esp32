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

// Büyüme evresi + mevsime göre uyarlanmış ETKİN eşikler. Otonom kontrol ve arayüz
// önerileri bunu kullanır. Sulama saatleri tabandan gelir; eşikler kaydırılır.
struct EtkinParam {
  float tempHigh, humHigh, humLow;
  int   soilDryYuzde, lightLowYuzde;
  const SulamaZamani* otoSulama;
  int   otoSulamaAdet;
};

inline int   biKirp(int v, int lo, int hi)       { return v < lo ? lo : (v > hi ? hi : v); }
inline float biKirpF(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline EtkinParam etkinParam(const BitkiParam& bp, const char* evre, const char* mevsim) {
  float tH = bp.tempHigh, hH = bp.humHigh, hL = bp.humLow;
  int   sD = bp.soilDryYuzde, lL = bp.lightLowYuzde;

  // Büyüme evresi: fide daha nemli + az ışık; meyve daha çok su + ışık, düşük nem.
  if      (strcmp(evre, "fide")  == 0) { sD += 10; lL -= 10; hH += 5; hL += 5; tH -= 2; }
  else if (strcmp(evre, "meyve") == 0) { sD += 10; lL += 10; hH -= 5;          tH += 1; }

  // Mevsim: yaz erken fan + çok su + az LED; kış az su + çok LED + sıcaklık toleransı.
  if      (strcmp(mevsim, "yaz") == 0) { tH -= 2; sD += 8;  lL -= 10; }
  else if (strcmp(mevsim, "kis") == 0) { tH += 2; sD -= 8;  lL += 15; }

  EtkinParam e;
  e.tempHigh = biKirpF(tH, 15, 45);
  e.humHigh  = biKirpF(hH, 40, 95);
  e.humLow   = biKirpF(hL, 30, 90);
  if (e.humLow >= e.humHigh) e.humLow = e.humHigh - 5;   // histerezis bandı korunsun
  e.soilDryYuzde  = biKirp(sD, 10, 90);
  e.lightLowYuzde = biKirp(lL, 0, 95);
  e.otoSulama = bp.otoSulama;
  e.otoSulamaAdet = bp.otoSulamaAdet;
  return e;
}
