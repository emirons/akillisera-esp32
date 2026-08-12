// control_logic.h — projenin BEYNI: saf karar mantığı.
// HİÇBİR Arduino başlığı include ETMEZ; yalnızca standart C++ başlıkları.
// Böylece host'ta (g++) derlenip birim testleriyle doğrulanır.
// Global değişken YOK, donanım çağrısı YOK — fonksiyonlar girdi alır, çıktı döndürür.
#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "config.h"

// Web arayüzü ve TalkBack'ten gelen komutlar.
enum class Komut { YOK, SULA, FAN_AC, FAN_KAPA, OTO, LED_AYARLA };
struct KomutSonucu { Komut komut; int deger; };

// Kontrol mantığının ürettiği eyleyici kararları (pompa hariç — o durum makinesi).
// actuators.h bunu alıp donanıma yansıtır; karar VERMEZ.
struct EyleyiciKarari { bool fanAcik; int ledParlaklik; bool alarmAktif; };

// Yardımcı: 0-255 aralığına kırp.
inline int kirp255(int v) {
  if (v < 0)   return 0;
  if (v > 255) return 255;
  return v;
}

// 1. Toprak kuruysa ve pompa zaten açık değilse sulama gerekir.
inline bool sulamaGerekli(int toprakNem, bool pompaAcik) {
  return toprakNem > SOIL_DRY_THRESHOLD && !pompaAcik;
}

// 2. Pompa süresi doldu mu? uint32_t çıkarma millis() taşmasını doğal tolere eder.
inline bool pompaSuresiDoldu(uint32_t simdi, uint32_t baslangic) {
  return (uint32_t)(simdi - baslangic) >= WATERING_DURATION;
}

// 2b. Sulama güvenlik kilidi: pompa açık değilse VE son sulamadan bu yana
//     WATERING_COOLDOWN geçtiyse tetiklenebilir. Sensör arızası "sürekli kuru"
//     derse bitkiyi boğmayı önler. uint32_t çıkarma millis() taşmasını tolere eder.
inline bool pompaTetiklenebilir(uint32_t simdi, uint32_t sonBitis, bool suAnAcik) {
  if (suAnAcik) return false;
  if ((uint32_t)(simdi - sonBitis) < WATERING_COOLDOWN) return false;
  return true;
}

// 3. Fan histerezisi. Bant içindeyken önceki durum korunur (röle çatırdaması önlenir).
inline bool fanKarari(float nem, float sicaklik, bool oncekiDurum) {
  if (nem > HUM_HIGH_THRESHOLD || sicaklik > TEMP_HIGH_THRESHOLD) return true;
  if (nem < HUM_LOW_THRESHOLD  && sicaklik < TEMP_HIGH_THRESHOLD) return false;
  return oncekiDurum;  // histerezis bandı
}

// 4. Şerit LED parlaklığı. Manuel modda değer kırpılır; otomatikte ışığa göre.
inline int ledParlaklikKarari(int isikSeviyesi, bool manuelMod, int manuelDeger) {
  if (manuelMod) return kirp255(manuelDeger);
  return isikSeviyesi < LIGHT_LOW_THRESHOLD ? LED_BRIGHTNESS : 0;
}

// 5. Sıcaklık alarmı.
inline bool alarmKarari(float sicaklik) {
  return sicaklik > TEMP_HIGH_THRESHOLD;
}

// 6. Sensör değeri geçerli mi? NaN/sonsuz değil ve makul aralıkta.
inline bool sensorDegeriGecerli(float deger) {
  return std::isfinite(deger) && deger >= -40.0f && deger <= 80.0f;
}

// 7. Komut ayrıştırma. nullptr ve geçersiz metin güvenli -> {YOK,0}.
inline KomutSonucu komutAyristir(const char* metin) {
  if (metin == nullptr) return {Komut::YOK, 0};
  if (std::strcmp(metin, "SULA")     == 0) return {Komut::SULA, 0};
  if (std::strcmp(metin, "FAN_AC")   == 0) return {Komut::FAN_AC, 0};
  if (std::strcmp(metin, "FAN_KAPA") == 0) return {Komut::FAN_KAPA, 0};
  if (std::strcmp(metin, "OTO")      == 0) return {Komut::OTO, 0};
  if (std::strncmp(metin, "LED_", 4) == 0) {
    return {Komut::LED_AYARLA, kirp255(std::atoi(metin + 4))};
  }
  return {Komut::YOK, 0};
}
