// Host birim testleri — saf kontrol mantığı (control_logic.h)
//
// Derleme + çalıştırma:
//   g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <cmath>
#include <limits>
#include "control_logic.h"
#include "bitki_veri.h"

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

// ---------------------------------------------------------------------------
// 9. butonGuncelle / butonBasildi — debounce (BUTTON_DEBOUNCE_MS 50), INPUT_PULLUP
// ---------------------------------------------------------------------------
TEST_CASE("butonGuncelle: temiz basis 50 ms sonra bildirilir") {
  ButonDurumu d = {true, true, 0};   // serbest: ham=true, kararli=true
  CHECK(butonGuncelle(d, false, 0)  == false);  // yeni ham, henuz kararli degil
  CHECK(butonGuncelle(d, false, 49) == false);  // 49 ms < 50
  CHECK(butonGuncelle(d, false, 50) == true);   // 50 ms -> gecis bildirilir
  CHECK(butonBasildi(d) == true);               // INPUT_PULLUP: kararli false = basili
}
TEST_CASE("butonGuncelle: titresim tek gecis uretir") {
  ButonDurumu d = {true, true, 0};
  int gecisSayisi = 0;
  int t[] = {0, 2, 4, 6, 8};                    // 10 ms icinde 5 kez cirpinma
  bool ham[] = {false, true, false, true, false};
  for (int i = 0; i < 5; i++) if (butonGuncelle(d, ham[i], t[i])) gecisSayisi++;
  // cirpinma sirasinda gecis olmamali
  CHECK(gecisSayisi == 0);
  // son degisim t=8, kararli hale 50 ms sonra
  CHECK(butonGuncelle(d, false, 58) == true);
  CHECK(gecisSayisi == 0);
}
TEST_CASE("butonGuncelle: basili tutma tekrar uretmez") {
  ButonDurumu d = {true, true, 0};
  butonGuncelle(d, false, 0);
  CHECK(butonGuncelle(d, false, 50) == true);   // ilk gecis
  CHECK(butonGuncelle(d, false, 100) == false); // basili tutuluyor, tekrar yok
  CHECK(butonGuncelle(d, false, 5000) == false);
}
TEST_CASE("butonGuncelle: birak-bas iki ayri gecis") {
  ButonDurumu d = {true, true, 0};
  butonGuncelle(d, false, 0);
  CHECK(butonGuncelle(d, false, 50) == true);    // basildi
  butonGuncelle(d, true, 100);                    // birakildi (ham degisti)
  CHECK(butonGuncelle(d, true, 150) == true);     // serbest gecisi
  CHECK(butonBasildi(d) == false);
}
TEST_CASE("butonGuncelle: millis tasmasi sirasinda dogru") {
  ButonDurumu d = {false, true, 0xFFFFFFF0u};    // ham=false, kararli henuz true
  CHECK(butonGuncelle(d, false, 34u) == true);   // gecen 50 ms (uint32 wrap)
}

// ---------------------------------------------------------------------------
// 10. sayfaSatirlari — LCD metinleri: tam 16 karakter, ASCII (Türkçe yok)
// ---------------------------------------------------------------------------
static bool asciiVe16(const char* s) {
  if (std::strlen(s) != 16) return false;
  for (int i = 0; i < 16; i++) if ((unsigned char)s[i] > 126 || (unsigned char)s[i] < 32) return false;
  return true;
}
TEST_CASE("sayfaSatirlari: IKLIM normal -> 16 char, ASCII") {
  EkranVerisi d = {23.4f, 65.0f, 42, false, 0, false, 0, false, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::IKLIM, l1, l2);
  CHECK(asciiVe16(l1));
  CHECK(asciiVe16(l2));
  CHECK(std::strncmp(l1, "T:23.4C H:65%", 13) == 0);
}
TEST_CASE("sayfaSatirlari: IKLIM uc degerler tasmadan sigar") {
  // en uzun: negatif sicaklik + nem 100 + LED 255
  EkranVerisi d = {-40.0f, 100.0f, 100, true, 255, false, 0, false, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::IKLIM, l1, l2);
  CHECK(asciiVe16(l1));               // "T:-40.0C H:100%" 15 -> padli 16
  CHECK(asciiVe16(l2));               // "Fan:ACIK LED:255" tam 16
  CHECK(std::strncmp(l2, "Fan:ACIK LED:255", 16) == 0);
}
TEST_CASE("sayfaSatirlari: IKLIM DHT arizali -> SENSOR HATASI") {
  EkranVerisi d = {0, 0, 0, false, 0, false, 0, true, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::IKLIM, l1, l2);
  CHECK(std::strncmp(l1, "SENSOR HATASI", 13) == 0);
  CHECK(asciiVe16(l1));
}
TEST_CASE("sayfaSatirlari: TOPRAK sulama geri sayim") {
  EkranVerisi d = {0, 0, 100, false, 0, true, 3, false, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::TOPRAK, l1, l2);
  CHECK(asciiVe16(l1));
  CHECK(asciiVe16(l2));
  CHECK(std::strncmp(l1, "Toprak: 100%", 12) == 0);
  CHECK(std::strncmp(l2, "Sulaniyor 3s", 12) == 0);
}
TEST_CASE("sayfaSatirlari: TOPRAK pompa kapali") {
  EkranVerisi d = {0, 0, 42, false, 0, false, 0, false, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::TOPRAK, l1, l2);
  CHECK(std::strncmp(l2, "Pompa: KAPALI", 13) == 0);
}
// ---------------------------------------------------------------------------
// 11. parlaklikGecerliMi — API girdi doğrulama (0-255)
// ---------------------------------------------------------------------------
TEST_CASE("parlaklikGecerliMi: sinirlar ve disi") {
  CHECK(parlaklikGecerliMi(0)   == true);
  CHECK(parlaklikGecerliMi(255) == true);
  CHECK(parlaklikGecerliMi(128) == true);
  CHECK(parlaklikGecerliMi(-1)  == false);
  CHECK(parlaklikGecerliMi(256) == false);
  CHECK(parlaklikGecerliMi(999) == false);
}

// ---------------------------------------------------------------------------
// 12. Zamanlı sulama — zamanGecerliMi + sulamaZamaniEslesme (saf)
// ---------------------------------------------------------------------------
TEST_CASE("zamanGecerliMi: gecerli ve gecersiz saatler") {
  CHECK(zamanGecerliMi(8, 0)   == true);
  CHECK(zamanGecerliMi(23, 59) == true);
  CHECK(zamanGecerliMi(0, 0)   == true);
  CHECK(zamanGecerliMi(24, 0)  == false);
  CHECK(zamanGecerliMi(12, 60) == false);
  CHECK(zamanGecerliMi(-1, 0)  == false);
  CHECK(zamanGecerliMi(12, -1) == false);
}
TEST_CASE("sulamaZamaniEslesme: eslesen indeks veya -1") {
  SulamaZamani liste[] = {{8,0},{12,0},{20,0}};
  CHECK(sulamaZamaniEslesme(liste, 3, 8, 0)   == 0);
  CHECK(sulamaZamaniEslesme(liste, 3, 12, 0)  == 1);
  CHECK(sulamaZamaniEslesme(liste, 3, 20, 0)  == 2);
  CHECK(sulamaZamaniEslesme(liste, 3, 12, 1)  == -1);  // dakika tutmuyor
  CHECK(sulamaZamaniEslesme(liste, 3, 7, 59)  == -1);
  CHECK(sulamaZamaniEslesme(liste, 0, 8, 0)   == -1);  // bos liste
}
TEST_CASE("sulamaZamaniEslesme: 10 ve 17 farkli dongu") {
  SulamaZamani liste[] = {{10,0},{17,0}};
  CHECK(sulamaZamaniEslesme(liste, 2, 10, 0) == 0);
  CHECK(sulamaZamaniEslesme(liste, 2, 17, 0) == 1);
  CHECK(sulamaZamaniEslesme(liste, 2, 8, 0)  == -1);
}

// ---------------------------------------------------------------------------
// 13. ThingSpeak bütçe + TalkBack metin kırpma (Faz 8)
// ---------------------------------------------------------------------------
TEST_CASE("gunlukIstekSayisi: 30 sn ve 15 sn") {
  CHECK(gunlukIstekSayisi(30000) == 2880);
  CHECK(gunlukIstekSayisi(15000) == 5760);
  CHECK(gunlukIstekSayisi(30000) + gunlukIstekSayisi(30000) == 5760);   // limit alti
  CHECK(gunlukIstekSayisi(15000) + gunlukIstekSayisi(15000) == 11520);  // limit ustu
}
TEST_CASE("metniKirp: sondaki CRLF temizlenir") {
  char b[16] = "SULA\r\n";
  metniKirp(b);
  CHECK(std::strcmp(b, "SULA") == 0);
}
TEST_CASE("metniKirp: bastaki/sondaki bosluk") {
  char b[16] = "  OTO  ";
  metniKirp(b);
  CHECK(std::strcmp(b, "OTO") == 0);
}
TEST_CASE("metniKirp: LED komutu newline ile") {
  char b[16] = "LED_120\n";
  metniKirp(b);
  CHECK(std::strcmp(b, "LED_120") == 0);
  CHECK(komutAyristir(b).komut == Komut::LED_AYARLA);
  CHECK(komutAyristir(b).deger == 120);
}
TEST_CASE("metniKirp: bos ve nullptr guvenli") {
  char b[4] = "";
  metniKirp(b);
  CHECK(std::strcmp(b, "") == 0);
  metniKirp(nullptr);   // crash olmamali
}

// ---------------------------------------------------------------------------
// 14. Bitkiye özel kontrol — sulamaGerekliYuzde + parametrik fan/alarm
// ---------------------------------------------------------------------------
TEST_CASE("sulamaGerekliYuzde: esik altinda kuru -> sula") {
  CHECK(sulamaGerekliYuzde(39, 40, false) == true);
  CHECK(sulamaGerekliYuzde(40, 40, false) == false);
  CHECK(sulamaGerekliYuzde(30, 40, true)  == false);
}
TEST_CASE("fanKarari: bitki esikleriyle (lettuce tempHigh 24)") {
  CHECK(fanKarari(50.0f, 25.0f, false, 70.0f, 55.0f, 24.0f) == true);
  CHECK(fanKarari(50.0f, 20.0f, true,  70.0f, 55.0f, 24.0f) == false);
  CHECK(fanKarari(75.0f, 20.0f, false, 70.0f, 55.0f, 24.0f) == true);
}
TEST_CASE("alarmKarari: bitki esigiyle") {
  CHECK(alarmKarari(25.0f, 24.0f) == true);
  CHECK(alarmKarari(23.0f, 24.0f) == false);
  CHECK(alarmKarari(36.0f) == true);
}

// ---------------------------------------------------------------------------
// 15. etkinParam — büyüme evresi + mevsim eşik kaydırma
// ---------------------------------------------------------------------------
TEST_CASE("etkinParam: vejetatif + ilkbahar = taban bantlar") {
  const BitkiParam& tomato = bitkiParam("tomato");   // t18-27 h50-70 s50-75 l60-90
  EtkinParam e = etkinParam(tomato, "vejetatif", "ilkbahar", false);
  CHECK(e.tHi == 27.0f);
  CHECK(e.sLo == 50);
  CHECK(e.lLo == 60);
}
TEST_CASE("etkinParam: fide -> daha nemli toprak, az isik, serin") {
  const BitkiParam& tomato = bitkiParam("tomato");
  EtkinParam e = etkinParam(tomato, "fide", "ilkbahar", false);
  CHECK(e.sLo == 60);    // 50+10 topragi daha nemli tut
  CHECK(e.lLo == 50);    // 60-10 az isik yeter
  CHECK(e.tHi == 25.0f); // 27-2 serin
}
TEST_CASE("etkinParam: meyve+yaz -> cok su, erken fan") {
  const BitkiParam& tomato = bitkiParam("tomato");
  EtkinParam e = etkinParam(tomato, "meyve", "yaz", false);
  CHECK(e.sLo == 68);    // 50+10+8
  CHECK(e.tHi == 26.0f); // 27+1-2 erken fan
}
TEST_CASE("sulamaSuresiOlcekle: evre ve mevsim miktari degistirir") {
  CHECK(sulamaSuresiOlcekle(8000, false,false, false,false) == 8000);   // taban
  CHECK(sulamaSuresiOlcekle(8000, true, false, false,false) == 4800);   // fide x0.6
  CHECK(sulamaSuresiOlcekle(8000, false,true,  false,false) == 10400);  // meyve x1.3
  CHECK(sulamaSuresiOlcekle(8000, false,false, true, false) == 10000);  // yaz x1.25
  CHECK(sulamaSuresiOlcekle(8000, false,false, false,true)  == 5600);   // kis x0.7
}
TEST_CASE("sulamaSuresiOlcekle: sinirlara kirpilir") {
  CHECK(sulamaSuresiOlcekle(1000, true, false, false,true) == WATERING_MIN_MS);  // cok kucuk
  CHECK(sulamaSuresiOlcekle(14000, false,true, true, false) == WATERING_MAX_MS); // cok buyuk
}
TEST_CASE("bitkiler: sulama sikligi ve miktari GERCEKTEN farkli") {
  // her bitkinin (adet, sure) ikilisi benzersiz olmali - hepsi 08:00/5sn olmamali
  int n = BITKI_SAYISI;
  int ayni = 0;
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++) {
      bool ayniAdet = BITKILER[i].otoSulamaAdet == BITKILER[j].otoSulamaAdet;
      bool ayniSure = BITKILER[i].sulamaMs == BITKILER[j].sulamaMs;
      bool ayniSaat = ayniAdet &&
        BITKILER[i].otoSulama[0].saat == BITKILER[j].otoSulama[0].saat &&
        BITKILER[i].otoSulama[0].dakika == BITKILER[j].otoSulama[0].dakika;
      if (ayniAdet && ayniSure && ayniSaat) ayni++;
    }
  CHECK(ayni == 0);
}
TEST_CASE("bitkiler: sig kok sik+az, derin kok seyrek+bol") {
  const BitkiParam& lettuce  = bitkiParam("lettuce");
  const BitkiParam& cucumber = bitkiParam("cucumber");
  const BitkiParam& tomato   = bitkiParam("tomato");
  const BitkiParam& pepper   = bitkiParam("pepper");
  CHECK(cucumber.otoSulamaAdet > tomato.otoSulamaAdet);   // salatalik daha sik
  CHECK(lettuce.sulamaMs < tomato.sulamaMs);              // marul daha az su
  CHECK(pepper.otoSulamaAdet == 1);                       // biber seyrek
  CHECK(pepper.sulamaMs > lettuce.sulamaMs);              // ama daha bol
}

TEST_CASE("etkinParam: gece serin (t band -2)") {
  const BitkiParam& tomato = bitkiParam("tomato");
  EtkinParam gunduz = etkinParam(tomato, "vejetatif", "ilkbahar", false);
  EtkinParam gece   = etkinParam(tomato, "vejetatif", "ilkbahar", true);
  CHECK(gece.tHi == gunduz.tHi - 2.0f);
}
TEST_CASE("etkinParam: histerezis bandi korunur") {
  const BitkiParam& tomato = bitkiParam("tomato");
  EtkinParam e = etkinParam(tomato, "fide", "ilkbahar", false);
  CHECK(e.hLo < e.hHi);    // bant ters donmez
}

// ---------------------------------------------------------------------------
// 16. Bant hedefli fan + VPD
// ---------------------------------------------------------------------------
TEST_CASE("fanKarariBant: banttan yuksek -> ac, banttan dusuk -> kapat, ic -> koru") {
  // lettuce band nem[50,70] sic[10,20]
  CHECK(fanKarariBant(60, 25, false, 50,70,10,20) == true);   // sic>20 ac
  CHECK(fanKarariBant(75, 15, false, 50,70,10,20) == true);   // nem>70 ac
  CHECK(fanKarariBant(60, 15, true,  50,70,10,20) == true);   // ikisi de bant ici -> koru(on)
  CHECK(fanKarariBant(60, 15, false, 50,70,10,20) == false);  // bant ici -> koru(off)
  CHECK(fanKarariBant(45,  8, true,  50,70,10,20) == false);  // ikisi de dusuk -> kapat
  CHECK(fanKarariBant(60,  8, true,  50,70,10,20) == true);   // nem bant ici -> koru(on)
}
TEST_CASE("vpdKpa: bilinen deger ~1.27 kPa (25C,60%)") {
  float v = vpdKpa(25.0f, 60.0f);
  CHECK(v > 1.20f);
  CHECK(v < 1.35f);
}
TEST_CASE("vpdKpa: cok nemli -> dusuk VPD") {
  CHECK(vpdKpa(22.0f, 95.0f) < 0.2f);
}

TEST_CASE("sayfaSatirlari: SISTEM wifi yok / var") {
  EkranVerisi d = {0, 0, 0, false, 0, false, 0, false, false, 0};
  char l1[17], l2[17];
  sayfaSatirlari(d, Sayfa::SISTEM, l1, l2);
  CHECK(std::strncmp(l1, "WiFi:YOK", 8) == 0);
  CHECK(std::strncmp(l2, "akillisera.local", 16) == 0);
  CHECK(asciiVe16(l2));
  d.wifiVar = true; d.ipSonOktet = 42;
  sayfaSatirlari(d, Sayfa::SISTEM, l1, l2);
  CHECK(std::strncmp(l1, "WiFi:OK .42", 11) == 0);
  CHECK(asciiVe16(l1));
}
