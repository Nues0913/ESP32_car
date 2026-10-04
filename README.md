# ESP32 Car

大學專題 《[Mixed Reality Teleoperation Pipeline for Resource-Constrained Mini-car via ROS2 Server](https://drive.google.com/file/d/1X-1db4s7sK-hg7P2BRPSYf7viPQ1V05r/view)》
ESP32-CAM 小車韌體，以 Wi-Fi／UDP 接收油門與轉向指令，透過 PCA9685 控制馬達驅動電路與舵機，並傳送 JPEG 影像。另有 BMI088 加速度與角速度串流(未使用)，可供外部定位或視覺慣性系統使用。


## 專案結構

| 檔案 | 職責 |
| --- | --- |
| `ESP32_car.ino` | 初始化、控制封包接收、逾時停車 |
| `pinConfig.h` | 相機與 I2C 腳位 |
| `NetworkConfig.h/.cpp` | Wi-Fi、UDP socket、位址與連接埠 |
| `CameraConfig.h/.cpp` | 相機設定、訂閱管理、JPEG 分包 |
| `MotorControl.h/.cpp` | 油門映射、馬達方向、舵機控制 |
| `IMUControl.h/.cpp` | BMI088 初始化、資料檢查與 UDP 任務 |
| `I2cLock.h/.cpp` | 遞迴 mutex、I2C 匯流排復原 |
| `DebugConfig.h/.cpp` | UDP 除錯文字輸出 |

I2C 透過遞迴 mutex 避免馬達與 IMU 任務同時存取。相機任務配置在 Core 0，IMU 任務啟用後配置在 Core 1。


## 目前功能

| 功能 | 狀態與行為 |
| --- | --- |
| 車輛控制 | 接收 UDP 指令；超過 300 ms 未收到通過來源檢查的非空控制封包時停止馬達 |
| 前輪轉向 | 依 `hd` 控制 PCA9685 通道 8 |
| 後輪舵機 | 啟動時置中；行駛中的通道 9 更新目前被註解 |
| 影像串流 | 收到 `SUB` 訂閱後，以 UDP 分包傳送 VGA JPEG |
| IMU 串流 | 已實作 BMI088 讀取與目標 200 Hz 傳送，但主程式尚未啟用 |
| 除錯訊息 | Wi-Fi 連線後由 UDP 4002 傳送文字訊息 |


## 硬體與接線

需要 ESP32-CAM、相機、PCA9685、相容的馬達驅動電路、馬達及舵機；BMI088 為選配。PCA9685 提供控制訊號，馬達供電須經適當的驅動電路，所有模組需共地。

### I2C

PCA9685 與 BMI088 共用 `Wire`，程式設定為 400 kHz。

| 訊號 | ESP32 GPIO |
| --- | --- |
| SDA | 1 |
| SCL | 3 |

GPIO 1／3 也用於 ESP32 的 UART0。使用 USB-UART 燒錄時需避免外接 I2C 裝置干擾這兩個腳位；此韌體的除錯輸出使用 UDP。

BMI088 使用 3.3 V 電源與相容的 I2C 電位。程式掃描加速度計位址 `0x18`／`0x19` 及陀螺儀位址 `0x68`／`0x69`，實際位址取決於模組接線。使用目前的資料同步模式時，須將 **Gyro INT3 接至 Accel INT1**；這條同步線不需接到 ESP32 GPIO。

### 相機腳位

腳位集中於 [pinConfig.h](pinConfig.h)，依實際硬體調整。

| 相機訊號 | GPIO |
| --- | --- |
| PWDN | 32 |
| RESET | -1（未使用） |
| XCLK | 0 |
| SIOD／SIOC | 26／27 |
| Y2、Y3、Y4、Y5 | 5、18、19、21 |
| Y6、Y7、Y8、Y9 | 36、39、34、35 |
| VSYNC／HREF／PCLK | 25／23／22 |


### PCA9685 通道

PWM 頻率為 50 Hz。通道分工如下；依實際硬體調整。

| 通道 | 用途 |
| --- | --- |
| 0、1、5、6 | 馬達方向訊號 |
| 2、3、4、7 | 馬達速度 PWM |
| 8 | 前輪舵機，計數範圍 292–462 |
| 9 | 後輪舵機，計數範圍 279–449 |

舵機計數是 PCA9685 的 PWM 計數值。`head_delta`、`tail_delta` 與左右界限依實際硬體校正，見 [MotorControl.cpp](MotorControl.cpp)。

## 安裝與啟動

使用 Arduino IDE 編譯與燒錄 ESP32-CAM 韌體。Wi-Fi 連接後，由電腦端透過 UDP 控制小車並接收影像。

### 1. 安裝 ESP32 開發板套件

在 Arduino IDE 的偏好設定中，將以下網址加入 Additional Boards Manager URLs／額外的開發板管理員網址：

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

開啟開發板管理員，搜尋並安裝 esp32 by Espressif Systems。

### 2. 安裝本專案需要的函式庫

在函式庫管理員搜尋下列名稱，選擇對應版本並安裝。

| 函式庫 | 版本 | 用途 |
| --- | --- | --- |
| Adafruit PWM Servo Driver Library | 3.0.3 | PCA9685 控制 |
| Adafruit BusIO | 1.17.4 | Adafruit PWM 3.0.3 的相依套件 |
| Bolder Flight Systems BMI088 | 1.0.1 | BMI088 加速度與角速度讀取 |


### 3. 開啟、設定與燒錄

1. 依 Wi-Fi 設定，替換 `NetworkConfig.cpp` 中的 `XXXXXX`。
2. 在工具選擇與實體模組相符的 ESP32-CAM 板型及 COM port，核對 `pinConfig.h` 的相機與 I2C 腳位。
3. GPIO 1／3 在執行時供 I2C 使用，也會與 UART 燒錄共用。注意避免外接裝置干擾上傳。
5. 上傳完成後恢復正常執行接線並重啟。


### 4. 連線與啟動接收端

1. 將控制端與 ESP32 連到同一區域網路，取得 ESP32 的 DHCP 位址；韌體主機名稱為 `esp-car`。
2. 在 ESP32 重啟前先監聽 UDP 4002，才能收到 Wi-Fi 連線後的啟動訊息。
3. 向 ESP32 的 UDP 4000 週期性送出 `th=128,hd=128`；每 50 ms 傳送一次，300 ms 逾時觸發停車。
4. 影像接收端向 UDP 4003 傳送 `SUB`，並使用同一個 socket 接收與重組影像。
5. 若已啟用 IMU，另外監聽 UDP 4001。


### Wi-Fi 設定


```cpp
const char *WIFI_SSID = "XXXXXX";
const char *WIFI_PSK  = "XXXXXX";
```

使用前需替換為實際設定。


## UDP 通訊

| ESP32 連接埠 | 用途 | 方向與格式 |
| --- | --- | --- |
| 4000 | 車輛控制 | 接收文字 `th=128,hd=128` |
| 4001 | IMU | 啟用後傳送 36-byte 二進位封包至接收端的 4001 |
| 4002 | 除錯 | 廣播文字訊息至接收端的 4002 |
| 4003 | 相機 | 接收 `SUB`，向訂閱封包的來源 IP／port 傳送影像 |

啟動時 IMU 與除錯訊息的目的位址由本機 IP 前三段加上 `.255` 組成，若子網遮罩與/24不同，調整 `initNetwork()` 的廣播位址計算。

收到影像訂閱後，`IMU_IP` 隨即改成該訂閱者 IP，IMU 目的 port 固定為 4001。

### 控制指令

每個封包使用文字欄位：

```text
th=128,hd=128
```

| 欄位 | 範圍 | 行為 |
| --- | --- | --- |
| `th` | 0–122 | 前進，越小越快 |
| `th` | 123–133 | 停止 |
| `th` | 134–255 | 後退，越大越快 |
| `hd` | 0–255 | 前輪轉向；程式設定 0 為右端、255 為左端，128 為中間 |

控制端應持續送出指令，建議每 50 ms 一次；停止傳送 300 ms 後，韌體會將馬達速度 PWM 設為 0，舵角則保持原值。前進與後退切換時，會先停車並等待 20 ms。


### 影像訂閱與重組

相機設定為 VGA `640 × 480`、JPEG 品質參數 15、20 MHz XCLK、兩個 frame buffer，使用 `CAMERA_GRAB_LATEST`。沒有固定 FPS 限制，實際速率取決於擷取及網路傳輸。

接收流程：

1. 接收端建立 UDP socket，向 ESP32 的 4003 傳送 ASCII `SUB`。
2. 保持同一個 socket 接收封包；ESP32 會回傳到送出訂閱時的來源 port。
3. 驗證標頭，依 `frame_id` 分組，依 `chunk_id` 排序 JPEG 片段。
4. 收齊 `total_chunks` 且總長度等於 `total_size` 後，將資料交給 JPEG 解碼器。
5. 對遺失片段設定接收逾時，丟棄不完整影格；韌體沒有重傳機制。

同時只記錄一個訂閱者，新的 `SUB` 會取代前一個。沒有取消訂閱指令或訂閱逾時。

每包由 **26-byte 標頭 + 最多 1400-byte JPEG 資料**組成。標頭使用 packed 結構，ESP32 直接傳送記憶體內容，接收端須以 little-endian 解析。

| Offset（byte） | 型別 | 欄位 | 說明 |
| --- | --- | --- | --- |
| 0 | `uint32_t` | `magic` | `0xCAFEFACE` |
| 4 | `uint64_t` | `timestamp_us` | 相機 frame buffer 時間戳加上校正偏移，單位 μs |
| 12 | `uint32_t` | `frame_id` | 影格序號 |
| 16 | `uint32_t` | `total_size` | 完整 JPEG 大小 |
| 20 | `uint16_t` | `chunk_id` | 片段編號，從 0 開始 |
| 22 | `uint16_t` | `total_chunks` | 該影格總片段數 |
| 24 | `uint16_t` | `payload_size` | 本包 JPEG 資料長度 |

Python 可使用 `struct.Struct("<IQIIHHH")` 解析標頭。每包長度應等於 `26 + payload_size`。`UDP_CHUNK_GAP_US` 目前為 0。


### IMU 啟用與封包

BMI088 UDP 任務以 5 ms 為目標週期讀取及傳送資料，排程、I2C 及網路負載可能影響實際間隔。

每包固定 **36 bytes**，little-endian，浮點數為 32-bit IEEE 754：

| Offset（byte） | 型別 | 欄位 | 單位／說明 |
| --- | --- | --- | --- |
| 0 | `uint64_t` | `timestamp` | 讀取前取得的 ESP32 時間，μs |
| 8、12、16 | `float` | `ax`、`ay`、`az` | 加速度，m/s² |
| 20、24、28 | `float` | `gx`、`gy`、`gz` | 角速度，rad/s |
| 32 | `uint32_t` | `seq` | 發送序號，從 0 開始 |

Python 可使用 `struct.Struct("<Q6fI")` 解析。

相機與 IMU 使用 ESP32 本機時間基準。


## 授權

本專案貢獻者有權授權的原創內容採用 [MIT License](LICENSE)，版權署名為 ESP32 Car contributors。

第三方函式庫與其他第三方內容保留各自的版權及授權條件。此聲明不代表已取得該部分的公開散布許可。
