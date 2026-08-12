// bitki.h — seçili bitki türünün anahtarını flash'ta (NVS) tutar. Reboot'ta kalır.
// Bitki bilgi tabanı (ideal aralıklar + öneriler) web arayüzünde (web_ui.h) gömülü;
// ESP32 yalnızca seçilen anahtarı saklar ve /api/bitki ile paylaşır.
#pragma once

#include <Preferences.h>

static char        g_bitki[20] = "custom";
static Preferences g_bitkiPref;

inline void bitkiYukle() {
  g_bitkiPref.begin("bitki", true);
  String k = g_bitkiPref.getString("k", "custom");
  k.toCharArray(g_bitki, sizeof(g_bitki));
  g_bitkiPref.end();
}

inline const char* bitkiAl() { return g_bitki; }

inline void bitkiAyarla(const char* anahtar) {
  strncpy(g_bitki, anahtar, sizeof(g_bitki) - 1);
  g_bitki[sizeof(g_bitki) - 1] = '\0';
  g_bitkiPref.begin("bitki", false);
  g_bitkiPref.putString("k", g_bitki);
  g_bitkiPref.end();
}
