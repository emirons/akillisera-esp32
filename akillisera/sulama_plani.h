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
  size_t beklenen = (size_t)s_planAdet * sizeof(SulamaZamani);
  size_t okunan   = s_pref.getBytes("plan", s_plan, beklenen);
  s_pref.end();

  // NVS bozulmuş/eksikse dizi sıfır kalır ve her girdi 00:00 olur -> gece yarısı
  // hayalet sulama tetiklenirdi. Eksik okuma veya geçersiz saat varsa planı boşalt.
  if (s_planAdet > 0 && okunan != beklenen) {
    Serial.println(F("Sulama plani bozuk (eksik NVS verisi) - plan sifirlandi"));
    s_planAdet = 0;
    return;
  }
  for (int i = 0; i < s_planAdet; i++) {
    if (!zamanGecerliMi(s_plan[i].saat, s_plan[i].dakika)) {
      Serial.println(F("Sulama planinda gecersiz saat - plan sifirlandi"));
      s_planAdet = 0;
      return;
    }
  }
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

// Ortak: verilen zaman listesi saate denk gelince günde/dakikada bir kez tetikler.
// (60 sn kilit ayrıca korur.) NTP yoksa pas geçer. sonKey mükerrer tetiği önler.
inline void zamanliSulamaIsle(const SulamaZamani* liste, int adet,
                              unsigned long simdi, int& sonKey) {
  int saat, dakika, saniye;
  if (!suankiZaman(saat, dakika, saniye)) return;
  int anahtar = saat * 60 + dakika;
  if (anahtar == sonKey) return;
  if (sulamaZamaniEslesme(liste, adet, saat, dakika) >= 0) {
    pompayiTetikle(simdi);
    sonKey = anahtar;
  }
}

// Kullanıcı planı (MANUEL sulama modunda çağrılır).
inline void planGuncelle(unsigned long simdi) {
  zamanliSulamaIsle(s_plan, s_planAdet, simdi, s_sonTetikDakika);
}

// Bitki optimum saatleri (OTO sulama modunda çağrılır) — ayrı mükerrer-koruma.
static int s_bitkiSonTetik = -1;
inline void bitkiOtoSulamaIsle(const SulamaZamani* liste, int adet, unsigned long simdi) {
  zamanliSulamaIsle(liste, adet, simdi, s_bitkiSonTetik);
}
