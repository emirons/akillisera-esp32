// buttons.h — debounce'lu fiziksel buton okuma.
// Ters INPUT_PULLUP mantığı control_logic.h'daki butonBasildi() içinde kapsüllü.
#pragma once

#include "config.h"
#include "control_logic.h"

// Serbest durumda başlat: ham=true (PULLUP), kararlı=true.
static ButonDurumu s_btnSulama = {true, true, 0};
static ButonDurumu s_btnSayfa  = {true, true, 0};

inline void butonlariBaslat() {
  pinMode(BTN_WATER_PIN, INPUT_PULLUP);
  pinMode(BTN_PAGE_PIN,  INPUT_PULLUP);
}

// Yalnızca YENİ basış anında (falling edge) true — basılı tutma tekrar üretmez.
inline bool sulamaButonunaBasildi(unsigned long simdi) {
  bool ham = digitalRead(BTN_WATER_PIN);
  bool degisti = butonGuncelle(s_btnSulama, ham, simdi);
  return degisti && butonBasildi(s_btnSulama);
}

inline bool sayfaButonunaBasildi(unsigned long simdi) {
  bool ham = digitalRead(BTN_PAGE_PIN);
  bool degisti = butonGuncelle(s_btnSayfa, ham, simdi);
  return degisti && butonBasildi(s_btnSayfa);
}
