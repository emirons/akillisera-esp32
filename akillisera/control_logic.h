// control_logic.h — projenin BEYNI: saf karar mantığı.
// HİÇBİR Arduino başlığı include ETMEZ; yalnızca standart C++ başlıkları.
// Böylece host'ta (g++) derlenip birim testleriyle doğrulanır.
// Global değişken YOK, donanım çağrısı YOK — fonksiyonlar girdi alır, çıktı döndürür.
#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "config.h"

// Web arayüzü ve TalkBack'ten gelen komutlar.
enum class Komut { YOK, SULA, FAN_AC, FAN_KAPA, OTO, LED_AYARLA };
struct KomutSonucu { Komut komut; int deger; };

// Kontrol mantığının ürettiği eyleyici kararları (pompa hariç — o durum makinesi).
// actuators.h bunu alıp donanıma yansıtır; karar VERMEZ.
struct EyleyiciKarari { bool fanAcik; int ledParlaklik; bool alarmAktif; };

// --- LCD sayfa formatlama (saf, host'ta test edilebilir) ---
// LCD karakter seti Türkçe içermez -> yalnızca ASCII (c,g,i,o,s,u).
enum class Sayfa : uint8_t { IKLIM = 0, TOPRAK = 1, SISTEM = 2, SAYI = 3 };

struct EkranVerisi {
  float   sicaklik;
  float   nem;
  int     toprakYuzde;
  bool    fanAcik;
  int     ledParlaklik;
  bool    pompaCalisiyor;
  int     pompaKalanSn;
  bool    dhtArizali;
  bool    wifiVar;
  uint8_t ipSonOktet;
};

// Metni tam 16 karaktere padle/kırp (sağ boşluk). out en az 17 bayt.
inline void pad16(const char* src, char* out) {
  snprintf(out, 17, "%-16.16s", src);
}

// Sayfaya göre iki LCD satırını (tam 16 karakter, padded) üretir. Donanım YOK.
inline void sayfaSatirlari(const EkranVerisi& d, Sayfa sayfa, char l1[17], char l2[17]) {
  char a[24], b[24];
  switch (sayfa) {
    case Sayfa::IKLIM:
      if (d.dhtArizali) snprintf(a, sizeof(a), "SENSOR HATASI");
      else              snprintf(a, sizeof(a), "T:%.1fC H:%d%%", d.sicaklik, (int)d.nem);
      snprintf(b, sizeof(b), "Fan:%-4s LED:%d", d.fanAcik ? "ACIK" : "KAPA", d.ledParlaklik);
      break;
    case Sayfa::TOPRAK:
      snprintf(a, sizeof(a), "Toprak: %d%%", d.toprakYuzde);
      if (d.pompaCalisiyor) snprintf(b, sizeof(b), "Sulaniyor %ds", d.pompaKalanSn);
      else                  snprintf(b, sizeof(b), "Pompa: KAPALI");
      break;
    case Sayfa::SISTEM:
    default:
      if (d.wifiVar) snprintf(a, sizeof(a), "WiFi:OK .%d", d.ipSonOktet);
      else           snprintf(a, sizeof(a), "WiFi:YOK");
      snprintf(b, sizeof(b), "akillisera.local");
      break;
  }
  pad16(a, l1);
  pad16(b, l2);
}

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

// Buton debounce durumu (INPUT_PULLUP: basılı = false).
struct ButonDurumu {
  bool sonOkuma;             // pinin son ham okuması
  bool kararliDurum;         // debounce sonrası kararlı durum
  unsigned long sonDegisim;  // son ham değişim anı
};

// Buton debounce. Dönüş: kararlı durum AZ ÖNCE değişti mi (evet -> true).
// uint32_t çıkarma millis() taşmasını tolere eder.
inline bool butonGuncelle(ButonDurumu& d, bool hamOkuma, unsigned long simdi) {
  if (hamOkuma != d.sonOkuma) {
    d.sonDegisim = simdi;
    d.sonOkuma = hamOkuma;
  }
  if ((uint32_t)(simdi - d.sonDegisim) >= BUTTON_DEBOUNCE_MS && hamOkuma != d.kararliDurum) {
    d.kararliDurum = hamOkuma;
    return true;
  }
  return false;
}

// INPUT_PULLUP ters mantığını burada kapsülle: kararlı LOW (false) = basılı.
inline bool butonBasildi(const ButonDurumu& d) {
  return d.kararliDurum == false;
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

// 5b. API girdi doğrulama (saf, test edilebilir): parlaklık 0-255 aralığında mı?
// Web/TalkBack'ten gelen değer sessizce KIRPILMAZ; geçersizse çağıran 400 alır.
inline bool parlaklikGecerliMi(int deger) {
  return deger >= 0 && deger <= 255;
}

// 5c. ThingSpeak API bütçesi: bir aralıkla günde kaç istek atılır?
// static_assert (config bölümünde) günlük ~8000 limitini derleme anında korur.
constexpr int gunlukIstekSayisi(unsigned long aralikMs) {
  return (int)(86400000UL / aralikMs);
}
// Biri aralığı 15 sn'ye düşürürse (5760+5760=11520>8000) DERLEME durur.
// Hata günlerce fark edilmez bir kota aşımı yerine anında yakalanır.
static_assert(gunlukIstekSayisi(CLOUD_INTERVAL) + gunlukIstekSayisi(TALKBACK_INTERVAL) < 8000,
              "ThingSpeak gunluk istek limiti asiliyor - araliklari artirin");

// 5d. Metni yerinde kırp: baştaki/sondaki boşluk, \r, \n, \t temizler.
// TalkBack yanıtı "SULA\r\n" gibi gelebilir; komutAyristir öncesi çağrılır.
inline void metniKirp(char* s) {
  if (s == nullptr) return;
  char* bas = s;
  while (*bas == ' ' || *bas == '\t' || *bas == '\r' || *bas == '\n') bas++;
  int n = (int)std::strlen(bas);
  while (n > 0) {
    char c = bas[n - 1];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') n--;
    else break;
  }
  for (int i = 0; i < n; i++) s[i] = bas[i];
  s[n] = '\0';
}

// --- Zamanlı sulama (saf mantık, host'ta test edilir) ---
// Her gün tekrarlı sulama saati. Donanım/NTP bağımsız düz veri.
struct SulamaZamani { uint8_t saat; uint8_t dakika; };

// Saat/dakika geçerli mi (24 saat formatı).
inline bool zamanGecerliMi(int saat, int dakika) {
  return saat >= 0 && saat <= 23 && dakika >= 0 && dakika <= 59;
}

// Verilen saat:dakika listedeki bir sulama zamanıyla eşleşiyor mu?
// Eşleşen ilk girdinin indeksini, yoksa -1 döner.
inline int sulamaZamaniEslesme(const SulamaZamani* liste, int adet, int saat, int dakika) {
  for (int i = 0; i < adet; i++) {
    if (liste[i].saat == saat && liste[i].dakika == dakika) return i;
  }
  return -1;
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
