# Refactor: Arduino IDE → PlatformIO

Nhánh: `bigrefactor` · Firmware: `3.7.6` → **`4.0.2`**

Ghi lại toàn bộ thay đổi khi chuyển project từ sketch Arduino IDE sang cấu trúc PlatformIO và cập nhật thư viện. Mục tiêu: **giữ nguyên hành vi và độ ổn định** của firmware đang chạy ngoài thực tế.

---

## 1. Tóm tắt

| Hạng mục | Trước | Sau |
|---|---|---|
| Firmware version | 3.7.6 | **4.0.2** |
| Build system | Arduino IDE (`main.ino`) | PlatformIO (`platformio.ini`) |
| Bố cục | Tất cả file ở thư mục gốc | `include/` + `src/` |
| Arduino-ESP32 core | 3.2.0 (cài trong IDE) | 3.2.0 (pioarduino `54.03.20`, pin cố định) |
| SinricPro | 4.1.0 | **5.1.0** |
| WebSockets | 2.7.2 | **2.7.3** |
| ArduinoJson | 7.4.2 | **7.4.3** |
| WakeOnLan | 1.1.7 | 1.1.7 (đã mới nhất) |
| Flash (slot OTA 1,310,720 B) | 1,290,374 B | 1,291,110 B (98.5%, còn ~19.5 KB) |
| RAM | 50,752 B | 50,744 B |

Build thành công, không còn warning nào trong code của project. **Chưa test trên phần cứng thật.**

---

## 2. Cấu hình PlatformIO (`platformio.ini`)

- **Platform:** `pioarduino/platform-espressif32` bản `54.03.20` = Arduino-ESP32 core **3.2.0** / ESP-IDF 5.4.1. Platform `espressif32` chính thức của PlatformIO chỉ hỗ trợ core 2.x nên không dùng được.
- **Thông số flash khớp với bản build Arduino IDE cũ.** Mình lấy các thông số này bằng cách đọc header của `build/esp32.esp32.esp32/main.ino.bin` và giải mã `main.ino.partitions.bin`:
  - `board = esp32dev`, 4MB, `flash_mode = dio`, `f_flash = 80MHz`
  - `partitions = default.csv`: nvs, otadata, app0/app1 mỗi slot 0x140000 (1.25MB), spiffs, coredump
- **Không được đổi partition table.** OTA chỉ ghi được vào slot app, không ghi được partition table. Thiết bị ngoài thực tế sẽ không nhận được bản OTA nào lớn hơn 1.25MB.
- `monitor_speed = 115200`, `monitor_filters = esp32_exception_decoder`, `upload_speed = 921600`.
- **Tất cả thư viện đều pin version chính xác** để build lặp lại được.
- `build_flags`:
  - `-Wall`
  - `-D SINRICPRO_NOMDNS` (xem mục 6)
  - Có sẵn dòng comment cho `ENABLE_DEBUG` và `SINRICPRO_NOSSL`. Trước đây hai `#define` này nằm trong `main.ino` **sau** các dòng `#include`, nên dù bỏ comment cũng không có tác dụng. Giờ chúng nằm trong build_flags nên dùng được thật.

---

## 3. Bố cục file

Mọi thao tác di chuyển đều dùng `git mv` nên lịch sử từng file vẫn còn.

### Di chuyển

| Cũ | Mới |
|---|---|
| `main.ino` | `src/main.cpp` |
| `ESP32OTAHelper.h` | `src/ota.cpp` (+ header mới `include/ota.h`) |
| `SemVer.h` | `include/SemVer.h` (chỉ còn khai báo) + `src/SemVer.cpp` mới |
| `*.h` còn lại | `include/` |
| `*.cpp` còn lại | `src/` |

### File mới

| File | Nội dung |
|---|---|
| `platformio.ini` | Cấu hình build |
| `include/sinric.h`, `src/sinric.cpp` | Toàn bộ code SinricPro |
| `include/button.h`, `src/button.cpp` | Xử lý nút vật lý |
| `include/ota.h` | Khai báo `startOtaUpdate()` |
| `src/SemVer.cpp` | Phần cài đặt của class `Version` |

### Cấu trúc cuối

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

## 4. Thay đổi trong code (giữ nguyên hành vi)

### `src/main.cpp`
- Chỉ còn `setup()`, `loop()` và một hàm `static handleWiFiStatus()`. Logic LED theo trạng thái WiFi trong hàm này giữ nguyên.
- Thứ tự các bước trong `loop()` y như cũ: `handleUsbWeb` → (AP mode: `handleAPMode`) → `handleSinricPro` → `processButtonPress` → `handleWiFiStatus` → `handleWakeTimers`.
- `setup()` giữ nguyên.
- **Version firmware vẫn nằm ở dòng 1 của `main.cpp`** (`#define FIRMWARE_VERSION`), giống `main.ino` cũ. Các module khác đọc qua `const char* const firmwareVersion` (định nghĩa trong `main.cpp`, khai báo `extern` trong `apmode.h`, `handleUsbWeb.h`, `sinric.h`).
- Bỏ các global `server` và `healthDiagnostics` khỏi `main`. Mỗi biến giờ thuộc về module dùng nó.

### `src/sinric.cpp`
- Chứa `handleOTAUpdate`, `onPowerState1`, `setSinricState`, `setupSinricPro`, cùng `handleSinricPro()` mới (bọc `SinricPro.handle()`).
- **Vì sao phải gom vào một file:** `SinricPro.h` định nghĩa object global `SinricPro` ngay trong header. Nếu include ở hai file `.cpp` trở lên sẽ bị lỗi *multiple definition* khi link. Các module khác gọi SinricPro thông qua `sinric.h`.
- **Giữ việc báo version lên server.** SDK SinricPro chỉ gửi header `firmwareVersion` khi kết nối nếu `FIRMWARE_VERSION` đã được `#define` *trước* `#include "SinricPro.h"`. Ở `main.ino` cũ điều kiện này đúng. Sau khi tách module thì macro không còn thấy được trong `sinric.cpp`, nên bản refactor đầu tiên **ngừng báo version lên SinricPro** (lỗi đã sửa). Cách sửa: trong `sinric.cpp` đặt `#define FIRMWARE_VERSION firmwareVersion` trước dòng include, để SDK đọc chuỗi version định nghĩa ở `main.cpp`. Đã kiểm tra trong `firmware.bin` có chuỗi `firmwareVersion:`.
- `healthDiagnostics` thành biến `static` của file. Lambda health-report đổi từ `[&]` sang `[]` vì không còn cần capture.
- Trong `onPowerState1`, nhánh `if (wolmode != "physical")` thừa (nhánh trước đó đã `return`) được bỏ. Hành vi không đổi.

### `src/wol.cpp` (gộp logic đánh thức PC)
- Callback SinricPro và nút bấm trước đây cùng chép một đoạn code. Giờ cả hai gọi chung `startWakeSequence(bool notifySinric)`, thứ tự các bước giữ nguyên:
  relay HIGH → `isWOLActive` → LED đỏ → buzzer → (`setSinricState(true)` nếu là nút bấm) → `wolExec()` → đặt timer.
  - Callback SinricPro gọi `startWakeSequence(false)`.
  - Nút bấm gọi `startWakeSequence(true)`, giống code cũ.
- `handleWakeTimers()` là đoạn tắt buzzer và tắt relay/trạng thái SinricPro, chuyển nguyên văn từ cuối `loop()` sang.

### `src/button.cpp`
- `processButtonPress()` chuyển nguyên từ `main.ino`, chỉ thay đoạn code đánh thức bằng `startWakeSequence(true)`.

### `src/apmode.cpp`
- `WebServer server(80)` chuyển từ `main.ino` vào đây thành biến `static`.
- `handleSave`, `handleRoot`, `handleFavicon`, `handleAPModeButton` thành `static` (chỉ dùng nội bộ).
- Thêm `handleAPMode()` = `server.handleClient()` + `handleAPModeButton()`, đúng hai lệnh mà `loop()` gọi trước đây.

### `src/ota.cpp`
- Trước đây toàn bộ hàm được định nghĩa trong header `ESP32OTAHelper.h`. Giờ là file `.cpp` riêng, bỏ guard `#if defined(ESP32)`.
- Thêm `#include <WiFi.h>`. Trước đây file này chỉ chạy được nhờ `main.ino` include `WiFi.h` trước nó.
- Ép kiểu `(size_t)contentLength` khi so sánh để hết warning signed/unsigned.

### `src/handleUsbWeb.cpp`
- `wifiPrevConnected` thành `static`.
- `StaticJsonDocument<512>` → `JsonDocument`. Ở ArduinoJson 7 hai kiểu này là một, nên hành vi không đổi.
- `doc.containsKey("x")` → `doc["x"].is<JsonVariantConst>()`. Mình đã đọc source ArduinoJson 7.4.3: cả hai đều trả về true khi và chỉ khi key tồn tại, kể cả khi giá trị là null.

### Header
- `apmode.h`: bỏ `extern WebServer server` và các handler nội bộ. Chỉ còn `extern firmwareVersion`, `startAPMode()`, `handleAPMode()`.
- `wol.h`: bỏ `#include <WakeOnLan.h>` (chuyển vào `wol.cpp`), thêm `startWakeSequence()` và `handleWakeTimers()`.

### Không đổi
`HealthDiagnostics.*`, `configinit.*`, `htmlhelper.*`, `ledbuz.*`, `pins.h`, `prefs.*`, `setupwifi.*`, `var.*`, `web.*`: chỉ di chuyển, không sửa nội dung.

---

## 5. Sửa lỗi (thay đổi hành vi có chủ đích)

1. **So sánh version OTA sai** (`SemVer.cpp`). `operator>` cũ so từng phần riêng lẻ, nên `3.6.9 > 3.7.6` cho ra `true` (do patch 9 > 6) và thiết bị có thể bị OTA về bản cũ hơn. Giờ so theo thứ tự major → minor → patch. `operator<` được định nghĩa lại bằng `other > *this`.
2. **Mất lệnh USB sau `SCAN_WIFI`** (`handleUsbWeb.cpp`). Nhánh `SCAN_WIFI` `return` mà không xoá `serialBuffer`, nên lệnh kế tiếp bị dính vào chuỗi cũ (ví dụ `SCAN_WIFIGET_CONFIG`) và bị bỏ qua. Giờ dòng lệnh được lấy ra và buffer được xoá **trước** khi xử lý.
3. **Log trống khi firmware đã mới nhất** (`sinric.cpp`). Code cũ gọi `Serial.println(result)` **sau** `std::move(result)`, nên dòng in ra rỗng. Đã đổi thứ tự.

---

## 6. Cập nhật thư viện

### SinricPro 4.1.0 → 5.1.0
- Đã đọc `changelog.md` của thư viện. Không có breaking change với code này: callback `onPowerState` dùng chung cho lệnh từ cloud và lệnh từ LAN.
- **Tính năng mới: Local Control (LAN/UDP).** Thiết bị nghe lệnh tại multicast `224.9.9.9:3333` và unicast cổng 3333. Mọi lệnh được ký HMAC-SHA256 bằng `APP_SECRET`, sai chữ ký sẽ bị từ chối. Tính năng này đang **bật**.
- **Behavior change từ thư viện:** lệnh đến qua UDP chỉ được trả lời qua UDP, không gửi lại lên cloud. Trạng thái tắt sau 2 giây của firmware vẫn được gửi lên cloud qua `sendPowerStateEvent` như cũ.
- **Tắt quảng bá mDNS** bằng `-D SINRICPRO_NOMDNS`. Phần này kéo theo `ESPmDNS` + `libespressif__mdns`, tốn khoảng 7.5KB, và không cần thiết vì app gửi lệnh unicast tới IP mà cloud báo về. Điều khiển LAN vẫn hoạt động.
- Bản 4.1.0 không có trên PlatformIO Registry (registry chỉ có 3.5.x và 5.x). Lúc chuyển đổi mình đã tạm pin theo git tag, sau đó bỏ khi lên 5.1.0.

### WebSockets 2.7.3, ArduinoJson 7.4.3
- Bản vá nhỏ, không phải sửa code. WebSockets còn 1 warning deprecated (`NetworkClient::flush()`) nằm trong code của thư viện, không phải code project.

---

## 7. Vì sao không lên core 3.3.12

Mình đã thử build với core mới nhất (pioarduino `55.03.312-1` = Arduino-ESP32 3.3.12 / IDF 5.5.5):

| Cấu hình | Kích thước | Còn trống trong slot OTA |
|---|---|---|
| Core 3.3.12 + libs mới (mDNS bật) | 1,318,110 B | **tràn 7.4 KB** |
| Core 3.3.12 + libs mới + `SINRICPRO_NOMDNS` | 1,310,590 B | 130 B |
| Core 3.3.12 + libs mới + `SINRICPRO_NO_LOCAL_CONTROL` | 1,310,054 B | 666 B |
| **Core 3.2.0 + libs mới + `SINRICPRO_NOMDNS`** (đang dùng) | **1,291,110 B** | **~19.5 KB** |

So sánh symbol giữa hai bản build cho thấy phần tăng chủ yếu nằm trong core/IDF 5.5: lwip +24KB, Arduino core +11KB, WiFi blob +10KB. SinricPro 5 chỉ thêm khoảng 1.5KB. Các phần của core/IDF là thư viện biên dịch sẵn, không cắt được nếu không build lại toàn bộ IDF với `custom_sdkconfig`, cách đó chậm và rủi ro hơn.

**Muốn lên core 3.3.x sau này:** đổi `board_build.partitions = min_spiffs.csv` (slot app 1.9MB; code không dùng SPIFFS). Đổi xong, mọi thiết bị cũ phải nạp lại qua USB/Web Manager một lần. Không nạp lại thì OTA sẽ báo lỗi "Not enough space to begin OTA" (thiết bị không bị brick).

---

## 8. Cách kiểm chứng

- **Refactor không làm thay đổi code sinh ra:** build code gốc (HEAD trước refactor, lấy bằng `git archive`) và code mới với cùng một `platformio.ini` (core 3.2.0, SinricPro 4.1.0):
  - Gốc: Flash 1,290,374 B, RAM 50,752 B
  - Refactor: Flash 1,290,358 B, RAM 50,744 B (nhỏ hơn 16 byte)
- Thư mục `build/` của Arduino IDE (04/08/2025) **không dùng làm mốc so sánh được**. Bản đó cũ hơn code hiện tại (chưa có `handleUsbWeb`) và được build với SinricPro 3.5.0 / ArduinoJson 7.3.1.
- Build cuối: `pio run` thành công, không còn warning trong `src/`.

---

## 9. Sự cố môi trường (máy dev, không liên quan đến code)

- **Python penv của PlatformIO bị hỏng** khi platform pioarduino 55.x cài Python dependencies. Platform cài vào `~/.platformio/penv` trong lúc chính `pio` đang chạy, nên Windows khoá file `.pyd` của `charset-normalizer` và gói này bị cài dở. Mình đã sửa bằng cách:
  - chạy lại `uv pip install ...` khi không có tiến trình `pio` nào đang chạy;
  - xoá thư mục `charset_normalizer-3.5.1.dist-info` còn sót (thiếu file `RECORD`).
- **pioarduino 55.x không cài được toolchain dưới Git Bash/MSYS** (lỗi `MSys/Mingw is not supported`). Chạy từ PowerShell hoặc từ extension PlatformIO trong VSCode thì bình thường.
- Platform 55.03.312-1 (core 3.3.12) vẫn còn trong `~/.platformio/platforms`, có thể xoá nếu không cần.

---

## 10. File khác

- `.gitignore`: thêm `.env` / `.env.*` (giữ lại `.env.example`), `.pio/` và các file sinh tự động trong `.vscode/` (`.browse.c_cpp.db*`, `c_cpp_properties.json`, `launch.json`, `ipch`).
- `tools/test.py` (mới): gửi lệnh `setPowerState` có ký HMAC tới thiết bị qua UDP 3333 để test Local Control của SinricPro 5. Dùng `--bad-signature` để kiểm tra thiết bị từ chối lệnh ký sai. IP, Device ID và App Secret được đọc tự động từ `.env` ở thư mục gốc (mẫu ở `.env.example`). **Đã test trên ESP32 thật:** lệnh ký đúng trả về `success: true` sau khoảng 312 ms. Phần lớn thời gian này là do `wolExec()` gửi magic packet 3 lần, cách nhau 100 ms, trước khi thiết bị trả lời. Lệnh ký sai bị từ chối với `Signature is invalid` sau 16 ms. Như vậy việc SDK mở cổng UDP trước khi WiFi kết nối xong không gây lỗi.
- `README.md`:
  - Mục *Manual Flash* viết lại cho PlatformIO, có sơ đồ thư mục và lệnh `pio run -t upload` / `pio device monitor`.
  - Mục *Advanced Configuration* trỏ tới `include/pins.h`, `src/var.cpp` và `build_flags` trong `platformio.ini`.

---

## 11. Việc còn lại

- [ ] Nạp lên ESP32 thật và kiểm tra: kết nối WiFi, bật qua SinricPro (Google Home/Alexa), nút ngắn, nút giữ 5 giây vào AP mode, trang cấu hình `192.168.4.1`, Web Manager qua USB (`GET_CONFIG`, `SCAN_WIFI` rồi gửi lệnh khác ngay sau, lưu cấu hình), OTA, Local Control (`tools/test.py`).
- [ ] Kiểm tra Web Manager (`daongochuy2516.github.io/esp32sinric`) với file build mới. Output của PlatformIO nằm ở `.pio/build/esp32dev/` (`firmware.bin`, `bootloader.bin`, `partitions.bin`), tên và đường dẫn khác với bản Arduino IDE. Với core 3.2.0, PlatformIO **không** tạo file merged (`main.ino.merged.bin` như Arduino IDE). Nếu Web Manager cần file này thì phải ghép bằng `esptool.py merge_bin`.
- [x] Tăng `FIRMWARE_VERSION` lên `4.0.2` (dòng 1 của `src/main.cpp` và badge README). Thiết bị cũ chạy 3.7.6 vẫn nhận bản này qua OTA: hàm so sánh version cũ (còn lỗi) xét major trước, mà `4 > 3`, nên vẫn ra kết quả đúng.
- [ ] Commit nhánh `bigrefactor` (chưa commit).
