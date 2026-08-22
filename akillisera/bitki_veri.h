// bitki_veri.h — ESP32 üstünde bitki KONTROL parametreleri (ideal BANTLAR).
// Otonom kontrol metrikleri bu bandların İÇİNDE tutmaya çalışır (band-hedefli).
// Bantlar web_ui PLANTS bilgi tabanıyla birebir aynı — kontrol ve gösterge tutarlı.
#pragma once

#include "config.h"
#include "control_logic.h"

struct BitkiParam {
  const char*  anahtar;
  int          tLo, tHi;        // sıcaklık ideal bandı (C)
  int          hLo, hHi;        // nem ideal bandı (%)
  int          sLo, sHi;        // toprak nem ideal bandı (%) — sLo altında sula
  int          lLo, lHi;        // ışık ideal bandı (%) — lLo altında LED
  SulamaZamani otoSulama[4];    // AUTO modda optimum sulama saatleri
  int          otoSulamaAdet;   // günlük sulama SIKLIĞI
  uint32_t     sulamaMs;        // her sulamada pompa süresi = MİKTAR
};

// Bantlar web_ui PLANTS ile aynı.
// Sulama saatleri/sıklığı/miktarı kök derinliği ve su talebine göre farklıdır:
//  - Sığ köklü + yüksek talep (lettuce, spinach, cucumber) -> SIK ve AZ su
//  - Derin köklü + kuraklığa dayanıklı (tomato, pepper)    -> SEYREK ve BOL su
//  - Sabah sulama tercih edilir: yaprak gün içinde kurur, mantar riski düşer.
//  - Akşam geç sulamadan kaçınılır (ıslak yaprak + serin gece = hastalık).
static const BitkiParam BITKILER[] = {
  // anahtar     t band  h band  s band  l band   sulama saatleri                      adet  süre
  {"custom",     18,28,  40,70,  40,80,  30,80,  {{8,0}},                                1,  5000},
  {"tomato",     18,27,  50,70,  50,75,  60,90,  {{7,0},{17,0}},                         2,  8000},
  {"lettuce",    10,20,  50,70,  60,80,  40,70,  {{6,30},{11,0},{15,30}},                3,  4000},
  {"pepper",     20,30,  50,70,  50,70,  60,95,  {{7,30}},                               1,  7000},
  {"cucumber",   20,28,  60,80,  60,85,  60,90,  {{6,30},{10,30},{14,30},{18,0}},        4,  6000},
  {"basil",      18,28,  40,60,  40,65,  55,90,  {{7,0}},                                1,  4000},
  {"strawberry", 15,24,  50,70,  50,75,  55,90,  {{7,0},{16,0}},                         2,  5000},
  {"spinach",    10,22,  50,70,  55,80,  40,70,  {{6,30},{16,30}},                       2,  4000},
};
constexpr int BITKI_SAYISI = sizeof(BITKILER) / sizeof(BITKILER[0]);

inline const BitkiParam& bitkiParam(const char* anahtar) {
  for (int i = 0; i < BITKI_SAYISI; i++)
    if (strcmp(BITKILER[i].anahtar, anahtar) == 0) return BITKILER[i];
  return BITKILER[0];
}

// Büyüme evresi + mevsim + gündüz/gece ile uyarlanmış ETKİN bantlar.
struct EtkinParam {
  float tLo, tHi, hLo, hHi;
  int   sLo, sHi, lLo, lHi;
  float alarmHigh;                     // bu değerin üstü = tehlike alarmı
  const SulamaZamani* otoSulama;
  int   otoSulamaAdet;
  uint32_t sulamaMs;                   // evre+mevsimle ölçeklenmiş sulama süresi
};

inline int biKirp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline EtkinParam etkinParam(const BitkiParam& bp, const char* evre, const char* mevsim, bool gece) {
  int tLo = bp.tLo, tHi = bp.tHi, hLo = bp.hLo, hHi = bp.hHi;
  int sLo = bp.sLo, sHi = bp.sHi, lLo = bp.lLo, lHi = bp.lHi;
  int dT = 0, dH = 0, dS = 0, dL = 0;

  if      (strcmp(evre, "fide")  == 0) { dS += 10; dL -= 10; dH += 5; dT -= 2; }
  else if (strcmp(evre, "meyve") == 0) { dS += 10; dL += 10; dH -= 5; dT += 1; }
  if      (strcmp(mevsim, "yaz") == 0) { dT -= 2; dS += 8; dL -= 10; }
  else if (strcmp(mevsim, "kis") == 0) { dT += 2; dS -= 8; dL += 15; }
  if (gece) dT -= 2;                    // gece serin tercih

  tLo += dT; tHi += dT; hLo += dH; hHi += dH;
  sLo += dS; sHi += dS; lLo += dL; lHi += dL;

  EtkinParam e;
  e.tLo = (float)biKirp(tLo, 5, 45);  e.tHi = (float)biKirp(tHi, 6, 46);
  e.hLo = (float)biKirp(hLo, 20, 90); e.hHi = (float)biKirp(hHi, 25, 95);
  e.sLo = biKirp(sLo, 5, 90);         e.sHi = biKirp(sHi, 10, 100);
  e.lLo = biKirp(lLo, 0, 90);         e.lHi = biKirp(lHi, 10, 100);
  if (e.tHi <= e.tLo) e.tHi = e.tLo + 2;
  if (e.hHi <= e.hLo) e.hHi = e.hLo + 5;
  e.alarmHigh = e.tHi + 4.0f;          // banttan 4C üstü = tehlike
  e.otoSulama = bp.otoSulama;
  e.otoSulamaAdet = bp.otoSulamaAdet;
  // Miktar da evre/mevsime göre ölçeklenir (fide az, meyve/yaz çok su).
  e.sulamaMs = sulamaSuresiOlcekle(bp.sulamaMs,
                                   strcmp(evre, "fide") == 0,
                                   strcmp(evre, "meyve") == 0,
                                   strcmp(mevsim, "yaz") == 0,
                                   strcmp(mevsim, "kis") == 0);
  return e;
}
