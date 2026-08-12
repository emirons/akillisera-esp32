// Host birim testleri — saf kontrol mantığı (control_logic.h)
//
// Derleme + çalıştırma:
//   g++ -std=c++17 -Wall -Wextra -Iakillisera tests/test_control_logic.cpp -o /tmp/tests && /tmp/tests
//
// NOT (Faz 0): control_logic.h henüz yok — bu dosya şu an DERLENMEZ.
// Faz 2'de control_logic.h yazılınca aşağıdaki include açılır ve testler yeşile döner.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Faz 2'de aç:
// #include "control_logic.h"

TEST_CASE("placeholder — Faz 2'de gercek testlerle degistirilecek") {
  CHECK(1 + 1 == 2);
}
