// bitki.h — seçili bitki türü + büyüme evresi + mevsim anahtarlarını flash'ta
// (NVS) tutar. Reboot'ta kalır. Bilgi tabanı web_ui.h'da; kontrol tablosu bitki_veri.h.
#pragma once

#include <Preferences.h>

static char        g_bitki[20]  = "custom";
static char        g_evre[16]   = "vejetatif";   // fide | vejetatif | meyve
static char        g_mevsim[16] = "ilkbahar";    // ilkbahar | yaz | sonbahar | kis
static Preferences g_bitkiPref;

inline void bitkiYukle() {
  g_bitkiPref.begin("bitki", true);
  g_bitkiPref.getString("k", "custom").toCharArray(g_bitki, sizeof(g_bitki));
  g_bitkiPref.getString("evre", "vejetatif").toCharArray(g_evre, sizeof(g_evre));
  g_bitkiPref.getString("mevsim", "ilkbahar").toCharArray(g_mevsim, sizeof(g_mevsim));
  g_bitkiPref.end();
}

inline const char* bitkiAl()  { return g_bitki; }
inline const char* evreAl()   { return g_evre; }
inline const char* mevsimAl() { return g_mevsim; }

inline void bitkiAyarla(const char* k) {
  strncpy(g_bitki, k, sizeof(g_bitki) - 1); g_bitki[sizeof(g_bitki) - 1] = '\0';
  g_bitkiPref.begin("bitki", false); g_bitkiPref.putString("k", g_bitki); g_bitkiPref.end();
}
inline void evreAyarla(const char* e) {
  strncpy(g_evre, e, sizeof(g_evre) - 1); g_evre[sizeof(g_evre) - 1] = '\0';
  g_bitkiPref.begin("bitki", false); g_bitkiPref.putString("evre", g_evre); g_bitkiPref.end();
}
inline void mevsimAyarla(const char* m) {
  strncpy(g_mevsim, m, sizeof(g_mevsim) - 1); g_mevsim[sizeof(g_mevsim) - 1] = '\0';
  g_bitkiPref.begin("bitki", false); g_bitkiPref.putString("mevsim", g_mevsim); g_bitkiPref.end();
}
