# Refactor: Arduino IDE → PlatformIO

Branch: `bigrefactor` · Firmware: `3.7.6` → **`4.0.2`**

A record of every change made while moving the project from an Arduino IDE sketch to a PlatformIO layout and updating the libraries. Goal: **keep the behavior and stability** of the firmware already running in the field.

---

## 1. Summary

| Item | Before | After |
|---|---|---|
| Firmware version | 3.7.6 | **4.0.2** |
| Build system | Arduino IDE (`main.ino`) | PlatformIO (`platformio.ini`) |
| Layout | Every file in the root folder | `include/` + `src/` |
| Arduino-ESP32 core | 3.2.0 (installed in the IDE) | 3.2.0 (pioarduino `54.03.20`, pinned) |
| SinricPro | 4.1.0 | **5.1.0** |
| WebSockets | 2.7.2 | **2.7.3** |
| ArduinoJson | 7.4.2 | **7.4.3** |
| WakeOnLan | 1.1.7 | 1.1.7 (already latest) |
| Flash (OTA slot 1,310,720 B) | 1,290,374 B | 1,291,110 B (98.5%, ~19.5 KB free) |
| RAM | 50,752 B | 50,744 B |

The build succeeds with no warnings left in the project's own code. **Not yet tested on real hardware.**

---

## 2. PlatformIO configuration (`platformio.ini`)

- **Platform:** `pioarduino/platform-espressif32` version `54.03.20` = Arduino-ESP32 core **3.2.0** / ESP-IDF 5.4.1. PlatformIO's official `espressif32` platform only supports core 2.x, so it can't be used.
- **Flash settings match the old Arduino IDE build.** I got them by reading the header of `build/esp32.esp32.esp32/main.ino.bin` and decoding `main.ino.partitions.bin`:
  - `board = esp32dev`, 4MB, `flash_mode = dio`, `f_flash = 80MHz`
  - `partitions = default.csv`: nvs, otadata, app0/app1 at 0x140000 (1.25MB) each, spiffs, coredump
- **The partition table must not change.** OTA can only write to an app slot, not to the partition table. Devices in the field will not accept any OTA image larger than 1.25MB.
- `monitor_speed = 115200`, `monitor_filters = esp32_exception_decoder`, `upload_speed = 921600`.
- **Every library is pinned to an exact version** so builds are reproducible.
- `build_flags`:
  - `-Wall`
  - `-D SINRICPRO_NOMDNS` (see section 6)
  - Commented-out lines are ready for `ENABLE_DEBUG` and `SINRICPRO_NOSSL`. These two `#define`s used to sit in `main.ino` **after** the `#include` lines, so uncommenting them had no effect. In `build_flags` they actually work.

---

## 3. File layout

Every move was done with `git mv`, so each file keeps its history.

### Moved

| Old | New |
|---|---|
| `main.ino` | `src/main.cpp` |
| `ESP32OTAHelper.h` | `src/ota.cpp` (+ new header `include/ota.h`) |
| `SemVer.h` | `include/SemVer.h` (declarations only) + new `src/SemVer.cpp` |
| Remaining `*.h` | `include/` |
| Remaining `*.cpp` | `src/` |

### New files

| File | Contents |
|---|---|
| `platformio.ini` | Build configuration |
| `include/sinric.h`, `src/sinric.cpp` | All SinricPro code |
| `include/button.h`, `src/button.cpp` | Physical button handling |
| `include/ota.h` | Declares `startOtaUpdate()` |
| `src/SemVer.cpp` | Implementation of the `Version` class |

### Final structure

```
platformio.ini
include/  HealthDiagnostics.h SemVer.h apmode.h button.h configinit.h handleUsbWeb.h
          htmlhelper.h ledbuz.h ota.h pins.h prefs.h setupwifi.h sinric.h var.h
          web.h wol.h
src/      HealthDiagnostics.cpp SemVer.cpp apmode.cpp button.cpp configinit.cpp
          handleUsbWeb.cpp htmlhelper.cpp ledbuz.cpp main.cpp ota.cpp prefs.cpp
          setupwifi.cpp sinric.cpp var.cpp web.cpp wol.cpp
```

---

## 4. Code changes (behavior unchanged)

### `src/main.cpp`
- Now only holds `setup()`, `loop()` and one `static handleWiFiStatus()` function. Its WiFi-status LED logic is unchanged.
- The order of steps in `loop()` is exactly as before: `handleUsbWeb` → (AP mode: `handleAPMode`) → `handleSinricPro` → `processButtonPress` → `handleWiFiStatus` → `handleWakeTimers`.
- `setup()` is unchanged.
- **The firmware version is still on line 1 of `main.cpp`** (`#define FIRMWARE_VERSION`), just like the old `main.ino`. Other modules read it through `const char* const firmwareVersion` (defined in `main.cpp`, declared `extern` in `apmode.h`, `handleUsbWeb.h`, `sinric.h`).
- The `server` and `healthDiagnostics` globals are gone from `main`. Each now belongs to the module that uses it.

### `src/sinric.cpp`
- Holds `handleOTAUpdate`, `onPowerState1`, `setSinricState`, `setupSinricPro`, and the new `handleSinricPro()` (wraps `SinricPro.handle()`).
- **Why it has to be one file:** `SinricPro.h` defines the global `SinricPro` object right in the header. Including it from two or more `.cpp` files causes a *multiple definition* error at link time. Other modules reach SinricPro through `sinric.h`.
- **Version reporting to the server is kept.** The SinricPro SDK only sends the `firmwareVersion` header on connect if `FIRMWARE_VERSION` is `#define`d *before* `#include "SinricPro.h"`. That was true in the old `main.ino`. After the module split the macro was no longer visible in `sinric.cpp`, so the first refactor **stopped reporting the version to SinricPro** (bug since fixed). The fix: in `sinric.cpp`, put `#define FIRMWARE_VERSION firmwareVersion` before the include, so the SDK reads the version string defined in `main.cpp`. Verified that `firmware.bin` contains the `firmwareVersion:` string.
- `healthDiagnostics` is now a file-`static` variable. The health-report lambda changed from `[&]` to `[]` since it no longer needs to capture anything.
- In `onPowerState1`, the redundant `if (wolmode != "physical")` branch (the previous branch already `return`s) was removed. Behavior is unchanged.

### `src/wol.cpp` (shared PC wake-up logic)
- The SinricPro callback and the button used to carry copies of the same code. Both now call `startWakeSequence(bool notifySinric)`, with the steps in the same order:
  relay HIGH → `isWOLActive` → red LED → buzzer → (`setSinricState(true)` if triggered by the button) → `wolExec()` → arm timers.
  - The SinricPro callback calls `startWakeSequence(false)`.
  - The button calls `startWakeSequence(true)`, as in the old code.
- `handleWakeTimers()` is the block that turns off the buzzer and switches the relay/SinricPro state off, moved verbatim from the end of `loop()`.

### `src/button.cpp`
- `processButtonPress()` moved as-is from `main.ino`; only the wake-up block was replaced with `startWakeSequence(true)`.

### `src/apmode.cpp`
- `WebServer server(80)` moved here from `main.ino` as a `static` variable.
- `handleSave`, `handleRoot`, `handleFavicon`, `handleAPModeButton` are now `static` (internal only).
- Adds `handleAPMode()` = `server.handleClient()` + `handleAPModeButton()`, exactly the two calls `loop()` used to make.

### `src/ota.cpp`
- The whole function used to be defined in the header `ESP32OTAHelper.h`. It is now its own `.cpp` file, without the `#if defined(ESP32)` guard.
- Adds `#include <WiFi.h>`. Previously the file only compiled because `main.ino` included `WiFi.h` before it.
- Casts `(size_t)contentLength` in the comparison to fix a signed/unsigned warning.

### `src/handleUsbWeb.cpp`
- `wifiPrevConnected` is now `static`.
- `StaticJsonDocument<512>` → `JsonDocument`. In ArduinoJson 7 these are the same type, so behavior is unchanged.
- `doc.containsKey("x")` → `doc["x"].is<JsonVariantConst>()`. I read the ArduinoJson 7.4.3 source: both return true if and only if the key exists, even when its value is null.

### Headers
- `apmode.h`: drops `extern WebServer server` and the internal handlers. Only `extern firmwareVersion`, `startAPMode()` and `handleAPMode()` remain.
- `wol.h`: drops `#include <WakeOnLan.h>` (moved into `wol.cpp`), adds `startWakeSequence()` and `handleWakeTimers()`.

### Unchanged
`HealthDiagnostics.*`, `configinit.*`, `htmlhelper.*`, `ledbuz.*`, `pins.h`, `prefs.*`, `setupwifi.*`, `var.*`, `web.*`: moved only, contents untouched.

---

## 5. Bug fixes (intentional behavior changes)

1. **Wrong OTA version comparison** (`SemVer.cpp`). The old `operator>` compared each part on its own, so `3.6.9 > 3.7.6` returned `true` (because patch 9 > 6) and a device could be OTA-downgraded. It now compares major → minor → patch in order. `operator<` is redefined as `other > *this`.
2. **USB command lost after `SCAN_WIFI`** (`handleUsbWeb.cpp`). The `SCAN_WIFI` branch `return`ed without clearing `serialBuffer`, so the next command was appended to the old string (e.g. `SCAN_WIFIGET_CONFIG`) and ignored. The line is now taken out and the buffer cleared **before** it is handled.
3. **Empty log line when firmware is up to date** (`sinric.cpp`). The old code called `Serial.println(result)` **after** `std::move(result)`, so it printed an empty line. The order is fixed.

---

## 6. Library updates

### SinricPro 4.1.0 → 5.1.0
- Read the library's `changelog.md`. No breaking changes for this code: the `onPowerState` callback is shared by cloud and LAN commands.
- **New feature: Local Control (LAN/UDP).** The device listens for commands on multicast `224.9.9.9:3333` and unicast port 3333. Every command is signed with HMAC-SHA256 using `APP_SECRET`; a bad signature is rejected. This feature is **enabled**.
- **Behavior change from the library:** commands received over UDP are only answered over UDP, not reported back to the cloud. The firmware's "off" state after 2 seconds is still sent to the cloud via `sendPowerStateEvent` as before.
- **mDNS announcement disabled** with `-D SINRICPRO_NOMDNS`. It pulls in `ESPmDNS` + `libespressif__mdns`, costs about 7.5KB, and isn't needed because the app sends unicast commands to the IP reported by the cloud. LAN control still works.
- Version 4.1.0 is not on the PlatformIO Registry (the registry only has 3.5.x and 5.x). During the migration I temporarily pinned it by git tag, then dropped that when moving to 5.1.0.

### WebSockets 2.7.3, ArduinoJson 7.4.3
- Minor patches, no code changes needed. WebSockets still has 1 deprecation warning (`NetworkClient::flush()`) inside the library's code, not the project's.

---

## 7. Why not core 3.3.12

I tried building with the latest core (pioarduino `55.03.312-1` = Arduino-ESP32 3.3.12 / IDF 5.5.5):

| Configuration | Size | Free in OTA slot |
|---|---|---|
| Core 3.3.12 + new libs (mDNS on) | 1,318,110 B | **overflows by 7.4 KB** |
| Core 3.3.12 + new libs + `SINRICPRO_NOMDNS` | 1,310,590 B | 130 B |
| Core 3.3.12 + new libs + `SINRICPRO_NO_LOCAL_CONTROL` | 1,310,054 B | 666 B |
| **Core 3.2.0 + new libs + `SINRICPRO_NOMDNS`** (in use) | **1,291,110 B** | **~19.5 KB** |

Comparing symbols between the two builds shows the growth is mostly in core/IDF 5.5: lwip +24KB, Arduino core +11KB, WiFi blob +10KB. SinricPro 5 only adds about 1.5KB. The core/IDF parts are precompiled libraries and can't be trimmed without rebuilding the whole IDF with `custom_sdkconfig`, which is slower and riskier.

**To move to core 3.3.x later:** change `board_build.partitions = min_spiffs.csv` (1.9MB app slot; the code doesn't use SPIFFS). After that, every existing device has to be reflashed once over USB/Web Manager. Without reflashing, OTA will fail with "Not enough space to begin OTA" (the device is not bricked).

---

## 8. Verification

- **The refactor doesn't change the generated code:** built the original code (HEAD before the refactor, taken with `git archive`) and the new code with the same `platformio.ini` (core 3.2.0, SinricPro 4.1.0):
  - Original: Flash 1,290,374 B, RAM 50,752 B
  - Refactor: Flash 1,290,358 B, RAM 50,744 B (16 bytes smaller)
- The Arduino IDE `build/` folder (04/08/2025) **can't be used as a baseline**. It is older than the current code (no `handleUsbWeb` yet) and was built with SinricPro 3.5.0 / ArduinoJson 7.3.1.
- Final build: `pio run` succeeds, no warnings left in `src/`.

---

## 9. Environment issues (dev machine, unrelated to the code)

- **PlatformIO's Python penv got corrupted** when the pioarduino 55.x platform installed its Python dependencies. The platform installs into `~/.platformio/penv` while `pio` itself is running, so Windows locked `charset-normalizer`'s `.pyd` file and the package was left half-installed. Fixed by:
  - rerunning `uv pip install ...` with no `pio` process running;
  - deleting the leftover `charset_normalizer-3.5.1.dist-info` folder (missing its `RECORD` file).
- **pioarduino 55.x can't install its toolchain under Git Bash/MSYS** (error `MSys/Mingw is not supported`). Running from PowerShell or from the PlatformIO extension in VSCode works fine.
- Platform 55.03.312-1 (core 3.3.12) is still in `~/.platformio/platforms` and can be deleted if not needed.

---

## 10. Other files

- `.gitignore`: adds `.env` / `.env.*` (keeping `.env.example`), `.pio/` and generated files under `.vscode/` (`.browse.c_cpp.db*`, `c_cpp_properties.json`, `launch.json`, `ipch`).
- `tools/test.py` (new): sends an HMAC-signed `setPowerState` command to the device over UDP 3333 to test SinricPro 5 Local Control. Use `--bad-signature` to check that the device rejects badly signed commands. The IP, Device ID and App Secret are read automatically from `.env` in the root folder (template in `.env.example`). **Tested on a real ESP32:** a correctly signed command returns `success: true` after about 312 ms. Most of that time is `wolExec()` sending the magic packet 3 times, 100 ms apart, before the device replies. A badly signed command is rejected with `Signature is invalid` after 16 ms. So the SDK opening the UDP port before WiFi finishes connecting causes no problems.
- `README.md`:
  - The *Manual Flash* section is rewritten for PlatformIO, with a folder diagram and the `pio run -t upload` / `pio device monitor` commands.
  - The *Advanced Configuration* section points to `include/pins.h`, `src/var.cpp` and `build_flags` in `platformio.ini`.

---

## 11. Remaining work

- [ ] Flash a real ESP32 and check: WiFi connection, power on through SinricPro (Google Home/Alexa), short press, 5-second hold to enter AP mode, config page at `192.168.4.1`, USB Web Manager (`GET_CONFIG`, `SCAN_WIFI` followed immediately by another command, saving config), OTA, Local Control (`tools/test.py`).
- [ ] Check the Web Manager (`daongochuy2516.github.io/esp32sinric`) with the new build files. PlatformIO output is in `.pio/build/esp32dev/` (`firmware.bin`, `bootloader.bin`, `partitions.bin`), with different names and paths from the Arduino IDE build. With core 3.2.0, PlatformIO does **not** produce a merged image (like Arduino IDE's `main.ino.merged.bin`). If the Web Manager needs one, build it with `esptool.py merge_bin`.
- [x] Bump `FIRMWARE_VERSION` to `4.0.2` (line 1 of `src/main.cpp` and the README badge). Devices running 3.7.6 still accept this build over OTA: the old (buggy) version comparison checks major first, and `4 > 3`, so it gives the right result.
- [x] Commit the `bigrefactor` branch (merged into `main` via #2).
