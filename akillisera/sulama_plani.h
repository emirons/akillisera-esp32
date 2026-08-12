// sulama_plani.h — zamanlı sulama modülü. NTP saatiyle kullanıcının belirlediği
// saatlerde (her gün tekrarlı) otomatik sulama. Flash'ta (NVS) kalıcı — reboot'ta
// silinmez. Manuel SULA butonu/API'den bağımsız çalışır.
#pragma once

#include <Preferences.h>
#include "config.h"
#include "control_logic.h"
#include "zaman.h"
#include "actuators.h"

static SulamaZamani s_plan[MAKS_SULAMA_ZAMANI];
static int          s_planAdet = 0;
static int          s_sonTetikDakika = -1;   // aynı dakikada tekrar tetiklemeyi önler
static Preferences  s_pref;

// Kalıcı depodan yükle (NVS namespace "sulama").
inline void planYukle() {
  s_pref.begin("sulama", true);              // salt-okunur
  s_planAdet = s_pref.getInt("adet", 0);
  if (s_planAdet < 0) s_planAdet = 0;
  if (s_planAdet > MAKS_SULAMA_ZAMANI) s_planAdet = MAKS_SULAMA_ZAMANI;
  s_pref.getBytes("plan", s_plan, s_planAdet * sizeof(SulamaZamani));
  s_pref.end();
}

inline void planKaydet() {
  s_pref.begin("sulama", false);             // yazılabilir
  s_pref.putInt("adet", s_planAdet);
  s_pref.putBytes("plan", s_plan, s_planAdet * sizeof(SulamaZamani));
  s_pref.end();
}

inline void planBaslat() { planYukle(); }

inline int  planAdet() { return s_planAdet; }
inline SulamaZamani planAl(int i) { return s_plan[i]; }

// Yeni sulama zamanı ekle. Geçersiz/dolu/mükerrer ise false.
inline bool planEkle(int saat, int dakika) {
  if (!zamanGecerliMi(saat, dakika)) return false;
  if (s_planAdet >= MAKS_SULAMA_ZAMANI) return false;
  if (sulamaZamaniEslesme(s_plan, s_planAdet, saat, dakika) >= 0) return false;  // mükerrer
  s_plan[s_planAdet].saat = (uint8_t)saat;
  s_plan[s_planAdet].dakika = (uint8_t)dakika;
  s_planAdet++;
  planKaydet();
  return true;
}

// İndeksteki zamanı sil.
inline bool planSil(int index) {
  if (index < 0 || index >= s_planAdet) return false;
  for (int i = index; i < s_planAdet - 1; i++) s_plan[i] = s_plan[i + 1];
  s_planAdet--;
  planKaydet();
  return true;
}

// Her loop turunda çağrılır. Saat bir plana denk gelince günde/dakikada bir kez
// pompayı tetikler (60 sn güvenlik kilidi ayrıca korur). NTP yoksa hiçbir şey yapmaz.
inline void planGuncelle(unsigned long simdi) {
  int saat, dakika, saniye;
  if (!suankiZaman(saat, dakika, saniye)) return;   // saat senkron değil -> pas
  int anahtar = saat * 60 + dakika;
  if (anahtar == s_sonTetikDakika) return;          // bu dakika zaten işlendi
  if (sulamaZamaniEslesme(s_plan, s_planAdet, saat, dakika) >= 0) {
    pompayiTetikle(simdi);
    s_sonTetikDakika = anahtar;
  }
}
