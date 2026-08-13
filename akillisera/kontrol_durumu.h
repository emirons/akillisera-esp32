// kontrol_durumu.h — paylaşılan çalışma-zamanı durumu. Web API (agkatmani.h) ve
// bulut/TalkBack (cloud.h) aynı fan/LED modlarını ve durum snapshot'ını kullanır.
// Tek yerde tutmak iki modülün birbirini include etmesini (döngü) önler.
#pragma once

#include "control_logic.h"

// Fan, LED ve SULAMA bağımsız oto/manuel.
// Sulama: oto=bitkinin optimum saatleri + toprak eşiği; manuel=kullanıcı planı + buton.
static bool g_fanOto = true, g_ledOto = true, g_sulamaOto = true, g_manuelFan = false;
static int  g_manuelLed = 0;

inline bool kdFanOto()    { return g_fanOto; }
inline bool kdLedOto()    { return g_ledOto; }
inline bool kdSulamaOto() { return g_sulamaOto; }
inline void kdSulamaOtoAyarla(bool oto) { g_sulamaOto = oto; }
inline bool kdManuelFan() { return g_manuelFan; }
inline int  kdManuelLed() { return g_manuelLed; }
inline void kdFanElle(bool acik)     { g_manuelFan = acik; g_fanOto = false; }
inline void kdLedElle(int v)         { g_manuelLed = kirp255(v); g_ledOto = false; }
inline void kdFanOtoAyarla(bool oto) { g_fanOto = oto; }
inline void kdLedOtoAyarla(bool oto) { g_ledOto = oto; }

// loop -> API/bulut ortak durum snapshot'ı.
struct DurumKaydi {
  float sicaklik, nem;
  int   toprakHam, toprakYuzde, isikHam, isikYuzde;
  bool  pompa, fan;
  int   led;
  bool  alarm, dhtGecerli;
  int   dhtHata;
  bool  gece;
  float vpd;
};
static DurumKaydi g_durum = {};
inline void kdDurumGuncelle(const DurumKaydi& d) { g_durum = d; }
inline const DurumKaydi& kdDurum() { return g_durum; }

// loop() süre ölçümü (µs) — /api/saglik'ta maxLoopMikrosaniye olarak raporlanır.
static unsigned long g_maxLoopUs = 0;
inline void          kdLoopKaydet(unsigned long us) { if (us > g_maxLoopUs) g_maxLoopUs = us; }
inline unsigned long kdMaxLoopUs() { return g_maxLoopUs; }
