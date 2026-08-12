// zaman.h — NTP saat. WiFi bağlanınca başlatılır. Bloklamaz (time() anlık okur).
#pragma once

#include <time.h>
#include "config.h"

// SNTP başlat (WiFi bağlandıktan sonra çağrılır). Arka planda senkronize olur.
inline void zamanBaslat() {
  configTime(GMT_OFFSET_SN, YAZ_SAATI_OFFSET_SN, NTP_SUNUCU);
}

// Saat NTP ile senkronize oldu mu? (epoch makul bir değerin üstündeyse).
inline bool zamanGecerli() {
  return time(nullptr) > 1700000000;   // ~2023 sonrası
}

// Şu anki yerel saat. Senkron değilse false döner, referanslara dokunmaz.
inline bool suankiZaman(int& saat, int& dakika, int& saniye) {
  time_t t = time(nullptr);
  if (t <= 1700000000) return false;
  struct tm ti;
  localtime_r(&t, &ti);
  saat = ti.tm_hour; dakika = ti.tm_min; saniye = ti.tm_sec;
  return true;
}

// "HH:MM:SS" (LCD/arayüz için). Senkron değilse "--:--:--".
inline void zamanMetni(char* buf, size_t n) {
  int s, d, sn;
  if (suankiZaman(s, d, sn)) snprintf(buf, n, "%02d:%02d:%02d", s, d, sn);
  else                       snprintf(buf, n, "--:--:--");
}
