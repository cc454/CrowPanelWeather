#include "storage.h"
#include <SD.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

int main() {
  using namespace cpw;
  const std::string secret = R"({"wifi_ssid":"Example","wifi_password":"test-password"})";
  const std::string original = R"({"latitude":47.3769,"longitude":8.5417,"label":"Zurich"})";
  const std::string replacement = R"({"latitude":35.6762,"longitude":139.6503,"label":"Tokyo"})";
  Config saved, active;
  std::string error;
  assert(parseConfig(secret, original, R"({"forecast_view":"large_three"})", saved, error));
  storage_test::preferences["config"] = configJson(saved);
  assert(beginStorage());
  assert(storage_test::pins[42] == HIGH);
  assert(storage_test::clock - storage_test::powerEnabledAt >= 10);
  assert(!cardAvailable());
  assert(loadConfiguration(active, false, error));
  assert(active.label == "Zurich" && error.find("mount failed") != std::string::npos);

  // Replacement inserted after startup: explicit Reload must mount it afresh.
  storage_test::inserted = true;
  SD.files["/secrets"] = secret;
  SD.files["/location"] = replacement;
  auto attempts = storage_test::mounts;
  assert(loadConfiguration(active, true, error));
  assert(storage_test::mounts == attempts + 1 && cardAvailable());
  assert(active.label == "Tokyo" && std::abs(active.latitude - 35.6762) < 1e-6 && error.empty());
  Config restored;
  assert(restoreConfig(storage_test::preferences["config"], restored, error));
  assert(restored.label == "Tokyo");

  // Saved menu choices survive ordinary startup; card location still wins.
  saved.label = "Saved label";
  storage_test::preferences["config"] = configJson(saved);
  assert(loadConfiguration(active, false, error));
  assert(active.label == "Tokyo" && active.largeThree);

  // Singular or hidden-extension filenames must report an actionable cause.
  SD.files["/secret"] = SD.files["/secrets"];
  SD.files.erase("/secrets");
  assert(!loadConfiguration(active, true, error));
  assert(error.find("secret to secrets") != std::string::npos);
  SD.files["/secrets.txt"] = SD.files["/secret"];
  SD.files.erase("/secret");
  assert(!loadConfiguration(active, true, error));
  assert(error.find("secrets.txt to secrets") != std::string::npos);
  SD.files["/secrets"] = secret;
  SD.files.erase("/secrets.txt");
  SD.files["/location"] = "not JSON";
  assert(loadConfiguration(active, false, error));
  assert(error.find("Using saved setup") != std::string::npos);
  assert(error.find("location: invalid") != std::string::npos);
  assert(error.find("test-password") == std::string::npos);
  assert(!loadConfiguration(active, true, error));
  SD.files["/location"] = replacement;
  auto oldSaved = storage_test::preferences["config"];
  storage_test::allowSave = false;
  assert(!loadConfiguration(active, true, error));
  assert(storage_test::preferences["config"] == oldSaved);
  storage_test::allowSave = true;

  std::ifstream fixture("tests/fixtures/open-meteo.json");
  std::string raw((std::istreambuf_iterator<char>(fixture)), {});
  assert(loadConfiguration(active, true, error));
  assert(saveCache(active, raw, 1793095200));
  Weather weather;
  assert(loadCache(active, weather));
  auto changed = active;
  changed.latitude += 1;
  assert(!loadCache(changed, weather));
  std::cout << "Storage: replacement remount, location import, filenames, fallback and cache "
               "isolation passed\n";
}
