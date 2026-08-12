// actuators.h — TÜM donanım sürme kodu burada.
// digitalWrite / LED_PWM_WRITE yalnızca bu dosyada; başka yerde pin sürme YOK.
// Pompa 5 sn çalışmasını delay() ile DEĞİL, millis() durum makinesiyle yapar.
#pragma once

#include "config.h"
#include "control_logic.h"

// --- Modül durumu ---
struct PompaDurumu { bool acik; unsigned long baslangic; };
static PompaDurumu    s_pompa = {false, 0};
static unsigned long  s_pompaSonBitis = 0;      // son sulamanın bitiş anı (soğuma için)
static bool           s_sonFanDurumu = false;
static int            s_sonLedDuty = -1;         // -1: henüz yazılmadı, ilk yazımı zorla
static unsigned long  s_alarmSonDegisim = 0;
static bool           s_alarmDurum = false;      // alarm yanıp sönmede o anki faz

// 1) BAŞLATMA
inline void eyleyicileriBaslat() {
  // KRİTİK SIRA: Röle pinlerini pinMode'dan ÖNCE RELAY_OFF'a çek. pinMode(OUTPUT)
  // pini LOW'a çeker; aktif-LOW röle LOW=AÇIK olduğu için ters sırada pompa
  // açılışta bir an çalışır. Önce çıktı latch'ini RELAY_OFF yap, sonra pinMode.
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
  digitalWrite(FAN_RELAY_PIN,  RELAY_OFF);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN,  OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
  digitalWrite(FAN_RELAY_PIN,  RELAY_OFF);

  pinMode(BUZZER_PIN, OUTPUT);    digitalWrite(BUZZER_PIN, LOW);
  pinMode(GREEN_LED_PIN, OUTPUT); digitalWrite(GREEN_LED_PIN, HIGH);  // normalde yeşil yanar
  pinMode(RED_LED_PIN, OUTPUT);   digitalWrite(RED_LED_PIN, LOW);

  LED_PWM_BEGIN();
  LED_PWM_WRITE(0);               // şerit LED karanlıkta başlar
}

// 2) POMPA — bloklamayan durum makinesi
inline bool pompaCalisiyor() { return s_pompa.acik; }

// Sulama tetikle. Güvenlik kilidi (60 sn soğuma) control_logic'te; burada uygulanır.
// Zaten açıksa süreyi UZATMAZ (üst üste basılan buton yeni 5 sn başlatmaz — bilinçli).
inline void pompayiTetikle(unsigned long simdi) {
  if (s_pompa.acik) return;
  if (!pompaTetiklenebilir((uint32_t)simdi, (uint32_t)s_pompaSonBitis, s_pompa.acik)) {
    Serial.println(F("Sulama engellendi: 60 sn soguma kilidi aktif"));
    return;
  }
  digitalWrite(PUMP_RELAY_PIN, RELAY_ON);
  s_pompa.acik = true;
  s_pompa.baslangic = simdi;
}

// Her loop turunda çağrılır: süre dolduysa pompayı kapatır (gecikmesiz).
inline void pompayiGuncelle(unsigned long simdi) {
  if (s_pompa.acik && pompaSuresiDoldu((uint32_t)simdi, (uint32_t)s_pompa.baslangic)) {
    digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
    s_pompa.acik = false;
    s_pompaSonBitis = simdi;
  }
}

// Kalan sulama süresi (saniye, yukarı yuvarlı) — LCD geri sayımı için.
inline int pompaKalanSaniye(unsigned long simdi) {
  if (!s_pompa.acik) return 0;
  uint32_t gecen = (uint32_t)simdi - (uint32_t)s_pompa.baslangic;
  if (gecen >= WATERING_DURATION) return 0;
  return (int)((WATERING_DURATION - gecen + 999) / 1000);
}

// 3) FAN — yalnızca durum değişince pin yaz (gereksiz röle işlemi yok)
inline void fanAyarla(bool acik) {
  if (acik == s_sonFanDurumu) return;
  digitalWrite(FAN_RELAY_PIN, acik ? RELAY_ON : RELAY_OFF);
  s_sonFanDurumu = acik;
}

// 4) ŞERİT LED — kırp, yalnızca değişince yaz
inline void ledParlaklikAyarla(int duty) {
  duty = kirp255(duty);
  if (duty == s_sonLedDuty) return;
  LED_PWM_WRITE(duty);
  s_sonLedDuty = duty;
}
inline int ledMevcutParlaklik() { return s_sonLedDuty < 0 ? 0 : s_sonLedDuty; }

// 5) ALARM — buzzer + kırmızı LED. Aktifken millis() tabanlı 500 ms yanıp sönme.
// tone() KULLANILMAZ: ESP32 core 3.x'te tone() LEDC kanalı kullanır ve şerit LED
// PWM'iyle çakışır. Bunun yerine düz digitalWrite kare dalga.
inline void alarmGuncelle(bool aktif, unsigned long simdi) {
  if (!aktif) {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    s_alarmDurum = false;
    return;
  }
  digitalWrite(GREEN_LED_PIN, LOW);
  if (simdi - s_alarmSonDegisim >= ALARM_BLINK_MS) {
    s_alarmSonDegisim = simdi;
    s_alarmDurum = !s_alarmDurum;
    digitalWrite(BUZZER_PIN,  s_alarmDurum ? HIGH : LOW);
    digitalWrite(RED_LED_PIN, s_alarmDurum ? HIGH : LOW);
  }
}

// 6) TOPLU UYGULAMA — kararı donanıma yansıtır (karar VERMEZ). Pompa ayrı yönetilir.
inline void eyleyicileriUygula(const EyleyiciKarari& k, unsigned long simdi) {
  fanAyarla(k.fanAcik);
  ledParlaklikAyarla(k.ledParlaklik);
  alarmGuncelle(k.alarmAktif, simdi);
}
