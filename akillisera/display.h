// display.h — 16x2 I2C LCD (0x27) donanım I/O. Sayfa/EkranVerisi/formatlama
// saf mantığı control_logic.h'da (host'ta test edilir). Burada yalnızca LCD sürme.
// clear() YALNIZCA başlatmada: her güncellemede clear() ~2 ms titreme yaratır;
// bunun yerine satırlar 16 karaktere padlenip setCursor+print ile yazılır.
#pragma once

#include <LiquidCrystal_I2C.h>
#include <cstring>
#include "config.h"
#include "control_logic.h"

static LiquidCrystal_I2C s_lcd(LCD_I2C_ADDR, 16, 2);
static Sayfa s_sayfa = Sayfa::IKLIM;
static unsigned long s_sonYenileme = 0;
static char s_onceki[2][17] = {{0}, {0}};   // son yazılan satırlar (değişince yaz)

inline void ekraniBaslat() {
  s_lcd.init();
  s_lcd.backlight();
  s_lcd.clear();                         // clear YALNIZCA burada
  s_lcd.setCursor(0, 0); s_lcd.print(F("Akilli Sera"));
  s_lcd.setCursor(0, 1); s_lcd.print(F("Baslatiliyor..."));
  delay(1500);                           // TEK İSTİSNA: setup'ta bir defalık açılış beklemesi
  s_lcd.clear();
  s_onceki[0][0] = '\0';
  s_onceki[1][0] = '\0';
}

inline void sayfaDegistir() {
  s_sayfa = static_cast<Sayfa>((static_cast<uint8_t>(s_sayfa) + 1) %
                               static_cast<uint8_t>(Sayfa::SAYI));
}
inline Sayfa mevcutSayfa() { return s_sayfa; }

// Zaten 16 karaktere padlenmiş satırı yaz; öncekiyle aynıysa YAZMA (I2C tasarrufu).
inline void satiriYaz(uint8_t row, const char* padli16) {
  if (strcmp(padli16, s_onceki[row]) == 0) return;
  strcpy(s_onceki[row], padli16);
  s_lcd.setCursor(0, row);
  s_lcd.print(padli16);
}

// LCD_REFRESH_INTERVAL (500 ms) periyodunda çağrılır.
inline void ekraniGuncelle(const EkranVerisi& d, unsigned long simdi) {
  if (simdi - s_sonYenileme < LCD_REFRESH_INTERVAL) return;
  s_sonYenileme = simdi;
  char l1[17], l2[17];
  sayfaSatirlari(d, s_sayfa, l1, l2);    // saf formatlama (control_logic.h)
  satiriYaz(0, l1);
  satiriYaz(1, l2);
}

// Geçici bildirim (örn. "SULA" / "Komut alindi").
inline void ekranMesaji(const char* satir1, const char* satir2) {
  char l1[17], l2[17];
  pad16(satir1, l1);
  pad16(satir2, l2);
  satiriYaz(0, l1);
  satiriYaz(1, l2);
}
