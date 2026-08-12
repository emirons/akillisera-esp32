// Host birim testleri — saf kontrol mantığı (control_logic.h)
//
// Derleme + çalıştırma:
//   g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <cmath>
#include <limits>
#include "control_logic.h"

// ---------------------------------------------------------------------------
// 1. sulamaGerekli — toprakNem > SOIL_DRY_THRESHOLD (2200) VE pompa kapalı
// ---------------------------------------------------------------------------
TEST_CASE("sulamaGerekli: esigin ustunde ve pompa kapali -> sula") {
  CHECK(sulamaGerekli(2201, false) == true);
}
TEST_CASE("sulamaGerekli: esik tam degeri -> sulama yok (ust degil)") {
  CHECK(sulamaGerekli(2200, false) == false);
}
TEST_CASE("sulamaGerekli: esigin altinda -> sulama yok") {
  CHECK(sulamaGerekli(2199, false) == false);
}
TEST_CASE("sulamaGerekli: kuru ama pompa zaten acik -> tekrar baslatma") {
  CHECK(sulamaGerekli(3000, true) == false);
}

// ---------------------------------------------------------------------------
// 2. pompaSuresiDoldu — simdi - baslangic >= WATERING_DURATION (5000), 32-bit wrap
// ---------------------------------------------------------------------------
TEST_CASE("pompaSuresiDoldu: sure tam doldu (>=)") {
  CHECK(pompaSuresiDoldu(5000u, 0u) == true);
}
TEST_CASE("pompaSuresiDoldu: sure dolmadi") {
  CHECK(pompaSuresiDoldu(4999u, 0u) == false);
}
TEST_CASE("pompaSuresiDoldu: millis tasmasi — gecen 32 ms, dolmadi") {
  // baslangic max'a yakin, simdi wrap sonrasi: 0x10 - 0xFFFFFFF0 = 32 (uint32)
  CHECK(pompaSuresiDoldu(0x00000010u, 0xFFFFFFF0u) == false);
}
TEST_CASE("pompaSuresiDoldu: millis tasmasi — gecen 6000 ms, doldu") {
  // baslangic 0xFFFFF000, simdi wrap sonrasi 1904 -> gecen 6000
  CHECK(pompaSuresiDoldu(1904u, 0xFFFFF000u) == true);
}

// ---------------------------------------------------------------------------
// 3. fanKarari — histerezis: HUM_HIGH 70, HUM_LOW 60, TEMP_HIGH 35
// ---------------------------------------------------------------------------
TEST_CASE("fanKarari: nem yuksek -> ac") {
  CHECK(fanKarari(75.0f, 20.0f, false) == true);
}
TEST_CASE("fanKarari: sicaklik yuksek -> ac") {
  CHECK(fanKarari(50.0f, 40.0f, false) == true);
}
TEST_CASE("fanKarari: ikisi de dusuk -> kapat") {
  CHECK(fanKarari(50.0f, 20.0f, true) == false);
}
TEST_CASE("fanKarari: histerezis bandi, onceki KAPALI -> kapali kalir") {
  CHECK(fanKarari(65.0f, 20.0f, false) == false);
}
TEST_CASE("fanKarari: histerezis bandi, onceki ACIK -> acik kalir") {
  CHECK(fanKarari(65.0f, 20.0f, true) == true);
}
TEST_CASE("fanKarari: nem tam 70 (ust degil) bantta -> onceki korunur") {
  CHECK(fanKarari(70.0f, 20.0f, false) == false);
  CHECK(fanKarari(70.0f, 20.0f, true) == true);
}
TEST_CASE("fanKarari: nem tam 60 (alt degil) bantta -> onceki korunur") {
  CHECK(fanKarari(60.0f, 20.0f, true) == true);
  CHECK(fanKarari(60.0f, 20.0f, false) == false);
}

// ---------------------------------------------------------------------------
// 4. ledParlaklikKarari — manuel kırpma; oto: isik<1500 ? 200 : 0
// ---------------------------------------------------------------------------
TEST_CASE("ledParlaklikKarari: manuel deger aynen") {
  CHECK(ledParlaklikKarari(0, true, 150) == 150);
}
TEST_CASE("ledParlaklikKarari: manuel ust kirpma 255") {
  CHECK(ledParlaklikKarari(0, true, 300) == 255);
}
TEST_CASE("ledParlaklikKarari: manuel alt kirpma 0") {
  CHECK(ledParlaklikKarari(0, true, -10) == 0);
}
TEST_CASE("ledParlaklikKarari: oto karanlik -> LED_BRIGHTNESS") {
  CHECK(ledParlaklikKarari(1000, false, 0) == 200);
}
TEST_CASE("ledParlaklikKarari: oto aydinlik -> 0") {
  CHECK(ledParlaklikKarari(2000, false, 0) == 0);
}
TEST_CASE("ledParlaklikKarari: oto esik tam degeri (alt degil) -> 0") {
  CHECK(ledParlaklikKarari(1500, false, 0) == 0);
}

// ---------------------------------------------------------------------------
// 5. alarmKarari — sicaklik > TEMP_HIGH_THRESHOLD (35)
// ---------------------------------------------------------------------------
TEST_CASE("alarmKarari: ustunde -> alarm") {
  CHECK(alarmKarari(36.0f) == true);
}
TEST_CASE("alarmKarari: tam esik -> alarm yok") {
  CHECK(alarmKarari(35.0f) == false);
}
TEST_CASE("alarmKarari: altinda -> alarm yok") {
  CHECK(alarmKarari(34.0f) == false);
}

// ---------------------------------------------------------------------------
// 6. sensorDegeriGecerli — NaN/inf degil ve -40..80 araliginda
// ---------------------------------------------------------------------------
TEST_CASE("sensorDegeriGecerli: makul deger") {
  CHECK(sensorDegeriGecerli(25.0f) == true);
}
TEST_CASE("sensorDegeriGecerli: NaN -> gecersiz") {
  CHECK(sensorDegeriGecerli(std::numeric_limits<float>::quiet_NaN()) == false);
}
TEST_CASE("sensorDegeriGecerli: sonsuz -> gecersiz") {
  CHECK(sensorDegeriGecerli(std::numeric_limits<float>::infinity()) == false);
}
TEST_CASE("sensorDegeriGecerli: alt sinir -40 dahil") {
  CHECK(sensorDegeriGecerli(-40.0f) == true);
}
TEST_CASE("sensorDegeriGecerli: ust sinir 80 dahil") {
  CHECK(sensorDegeriGecerli(80.0f) == true);
}
TEST_CASE("sensorDegeriGecerli: sinir disi alt") {
  CHECK(sensorDegeriGecerli(-41.0f) == false);
}
TEST_CASE("sensorDegeriGecerli: sinir disi ust") {
  CHECK(sensorDegeriGecerli(81.0f) == false);
}

// ---------------------------------------------------------------------------
// 7. komutAyristir — metin -> {Komut, deger}
// ---------------------------------------------------------------------------
TEST_CASE("komutAyristir: SULA") {
  KomutSonucu r = komutAyristir("SULA");
  CHECK(r.komut == Komut::SULA);
  CHECK(r.deger == 0);
}
TEST_CASE("komutAyristir: FAN_AC") {
  CHECK(komutAyristir("FAN_AC").komut == Komut::FAN_AC);
}
TEST_CASE("komutAyristir: FAN_KAPA") {
  CHECK(komutAyristir("FAN_KAPA").komut == Komut::FAN_KAPA);
}
TEST_CASE("komutAyristir: OTO") {
  CHECK(komutAyristir("OTO").komut == Komut::OTO);
}
TEST_CASE("komutAyristir: LED_150 -> deger 150") {
  KomutSonucu r = komutAyristir("LED_150");
  CHECK(r.komut == Komut::LED_AYARLA);
  CHECK(r.deger == 150);
}
TEST_CASE("komutAyristir: LED_999 -> ust kirpma 255") {
  CHECK(komutAyristir("LED_999").deger == 255);
}
TEST_CASE("komutAyristir: LED_-5 -> alt kirpma 0") {
  KomutSonucu r = komutAyristir("LED_-5");
  CHECK(r.komut == Komut::LED_AYARLA);
  CHECK(r.deger == 0);
}
TEST_CASE("komutAyristir: gecersiz metin -> YOK") {
  CHECK(komutAyristir("SACMA").komut == Komut::YOK);
}
TEST_CASE("komutAyristir: bos metin -> YOK") {
  CHECK(komutAyristir("").komut == Komut::YOK);
}
TEST_CASE("komutAyristir: nullptr -> YOK") {
  CHECK(komutAyristir(nullptr).komut == Komut::YOK);
}

// ---------------------------------------------------------------------------
// 8. pompaTetiklenebilir — sulama güvenlik kilidi (WATERING_COOLDOWN 60000)
// ---------------------------------------------------------------------------
TEST_CASE("pompaTetiklenebilir: pompa zaten acik -> reddet") {
  CHECK(pompaTetiklenebilir(100000u, 0u, true) == false);
}
TEST_CASE("pompaTetiklenebilir: soguma dolmadi (59 sn) -> reddet") {
  CHECK(pompaTetiklenebilir(59000u, 0u, false) == false);
}
TEST_CASE("pompaTetiklenebilir: soguma tam siniri (60 sn) -> izin") {
  CHECK(pompaTetiklenebilir(60000u, 0u, false) == true);
}
TEST_CASE("pompaTetiklenebilir: soguma gecti (61 sn) -> izin") {
  CHECK(pompaTetiklenebilir(61000u, 0u, false) == true);
}
TEST_CASE("pompaTetiklenebilir: ilk tetikleme, sonBitis 0, cok sonra -> izin") {
  CHECK(pompaTetiklenebilir(500000u, 0u, false) == true);
}
TEST_CASE("pompaTetiklenebilir: millis tasmasi, gecen 60 sn -> izin") {
  // sonBitis 0xFFFF0000, simdi wrap sonrasi: gecen tam 60000
  uint32_t sonBitis = 0xFFFF0000u;
  uint32_t simdi = sonBitis + 60000u;  // uint32 wrap
  CHECK(pompaTetiklenebilir(simdi, sonBitis, false) == true);
}
TEST_CASE("pompaTetiklenebilir: millis tasmasi, gecen 59 sn -> reddet") {
  uint32_t sonBitis = 0xFFFF0000u;
  uint32_t simdi = sonBitis + 59000u;
  CHECK(pompaTetiklenebilir(simdi, sonBitis, false) == false);
}
