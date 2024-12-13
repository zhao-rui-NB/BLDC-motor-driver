# BLDC 馬達驅動

## 目前功能
* 馬達控制UI [軟體](2.Software/main.py)
  * 啟動停止
  * 正反轉
  * PWM
  * PID 參數
  * 轉速顯示
* P2.1 按鈕啟用油門把手控制馬達


## mcu
* [MSP430F5529](https://www.ti.com/product/zh-tw/MSP430F5529)
* [MSP-EXP430F5529LP](https://www.ti.com/tool/MSP-EXP430F5529LP)

### GPIO 分配

| **GPIO 引腳** | **硬體連接**        | **用途**              |
|---------------|--------------------|----------------------|
| P1.2 (TA0.1)  | MOS CH         | PWM 信號輸出         |
| P1.3          | MOS CL         | 信號輸出             |
| P1.4 (TA0.3)  | MOS BH         | PWM 信號輸出         |
| P1.5          | MOS BL         | 信號輸出             |
| P2.4 (TA2.1)  | MOS AH         | PWM 信號輸出         |
| P2.5          | MOS AL         | 信號輸出             |
| P2.0          | Y霍爾傳感器信號輸入| 捕獲電機位置, 外部中斷量測轉速速度 |
| P2.6          | G霍爾傳感器信號輸入| 捕獲電機位置 |
| P2.3          | B霍爾傳感器信號輸入| 捕獲電機位置 |
| P2.1          | 按鈕              | 啟用/禁用 油門把手輸入 |
| P1.0          | RED LED           | 油門把手啟用指示燈  |
| P6.1 (A1)     | 油門把手          | 油門類比輸入         |

### MCU 資源分配

| **資源** |  **用途** | **詳細描述** |
|---|---|---|
| UCS MCLK|                    | 25MHZ |
| UCS SMCLK|                 | 12.5MHZ |
| Timer A0 | UP MODE |  MOS上管 PWM產生 |
| Timer A0 | CCR0 interrupt | 用來更新馬達PWM duty |
| Timer A2 | UP MODE | MOS上管 PWM產生 |
| Timer B0 | UP MODE interrupt | 用來分割uart封包的間隔 |
| USCI A1  | uart 電腦通訊 | baud 115200 |



## BLDC馬達驅動器通訊協議

### 指令格式

每個指令組包含以下元素：
- **Frame Head (1 byte)**: 固定為 `0xAA`，表示指令的開始。
- **Command (1 byte)**: 指令碼，代表要執行的操作。
- **Parameters (可選, 多個bytes)**: 根據指令碼需要傳遞的參數。
- **Checksum (1 byte)**: 透過 XOR 計算所有前面數據得到的校驗碼。

#### 指令範例

- **無參數指令**
  - **指令**: 啟用/禁用馬達
  - **數據**: `0xAA 0x03 0xXX 0xYY`
    - `0xAA`: Frame Head
    - `0x03`: Command (啟動或停止馬達)
    - `0xXX`: Parameter (1 啟動, 0 停止)
    - `0xYY`: Checksum

- **帶參數指令**
  - **指令**: 設定目標轉速
  - **數據**: `0xAA 0x01 0xXX 0xYY 0xZZ`
    - `0xAA`: Frame Head
    - `0x01`: Command (設定轉速)
    - `0xXX 0xYY`: Parameter (目標轉速值)
    - `0xZZ`: Checksum

### 回應格式

每個回應組包含以下元素：
- **Frame Head (1 byte)**: 固定為 `0xAA`，表示回應的開始。
- **Status Code (1 byte)**: 指示操作成功或錯誤類型的狀態碼。
- **Reply Value (可選, 多個bytes)**: 根據指令類型可能提供的回應數據。
- **Checksum (1 byte)**: 透過 XOR 計算所有前面數據得到的校驗碼。

#### 回應範例

- **無回應數據**
  - **功能**: 停止馬達
  - **數據**: `0xAA 0x00 0xAA`
    - `0xAA`: Frame Head
    - `0x00`: Status Code (STATUS_OK)
    - `0xAA`: Checksum

- **帶回應數據**
  - **功能**: 查詢目標轉速
  - **數據**: `0xAA 0x00 0xXX 0xYY 0xZZ`
    - `0xAA`: Frame Head
    - `0x00`: Status Code (STATUS_OK)
    - `0xXX 0xYY`: Reply Value (當前轉速值)
    - `0xZZ`: Checksum

### 功能碼說明

| 功能碼 | 描述                | 參數類型         | 回應類型         |
|--------|-------------------|----------------|----------------|
| `0x01` | 設定目標轉速       | 16-bit 數值 (RPM) | 無             |
| `0x02` | 查詢目標轉速       | 無              | 16-bit 數值 (RPM) |
| `0x03` | 啟動/停止馬達      | 1 byte (0/1)    | 無             |
| `0x04` | 設定控制模式       | 1 byte (模式)    | 無             |
| `0x05` | 查詢控制模式       | 無              | 1 byte (模式)    |

以下是所有可用的功能碼及其對應的操作說明：

| 功能碼 | 功能名稱               | 功能說明                        | 參數類型            | 回應類型         |
|--------|----------------------|---------------------------------| --------------------| ----------------|
| `0x01` | CMD_SET_POWER_EN     | 設定馬達電源啟用狀態              | bool                | 無             |
| `0x02` | CMD_GET_POWER_EN     | 查詢馬達電源啟用狀態              | 無                  | bool           |
| `0x03` | CMD_SET_TARGET_SPEED | 設定目標轉速                      | 16-bit 數值 (RPM)  | 無              |
| `0x04` | CMD_GET_TARGET_SPEED | 查詢目標轉速                      | 無                 | 16-bit 數值(RPM) |
| `0x05` | CMD_SET_IS_REVERSE   | 設定馬達是否反轉                  | bool               | 無                |
| `0x06` | CMD_GET_IS_REVERSE   | 查詢馬達是否反轉                  | 無                 | bool              |
| `0x07` | CMD_SET_CONTROL_MODE | 設定馬達控制模式                  | uint8 0:PWM 1:PID  | 無                |
| `0x08` | CMD_GET_CONTROL_MODE | 查詢馬達控制模式                  | 無                 |  uint8 0:PWM 1:PID |
| `0x09` | CMD_SET_P_VALUE      | 設定PID控制器的比例增益P值        | 4byte float        | 無                 |
| `0x0A` | CMD_GET_P_VALUE      | 查詢PID控制器的比例增益P值        | 無                 |  4byte float       |
| `0x0B` | CMD_SET_I_VALUE      | 設定PID控制器的積分增益I值        | 4byte float        | 無                 |
| `0x0C` | CMD_GET_I_VALUE      | 查詢PID控制器的積分增益I值        | 無                 |  4byte float       |
| `0x0D` | CMD_SET_PWM_COMPARE  | 設定PWM比較值                    | uint16            | 無                  |
| `0x0E` | CMD_GET_PWM_COMPARE  | 查詢PWM比較值                    | 無                | uint16              |
| `0x0F` | CMD_GET_CURRENT_SPEED| 查詢當前轉速                     | 無                | 4byte float         |
### 狀態碼說明

| 狀態碼 | 說明               |
|--------|-------------------|
| `0x00` | 指令執行成功       |
| `0x01` | 無效的數據幀       |
| `0x02` | 校驗碼錯誤         |
| `0x03` | 不支持的功能碼     |
| `0x04` | 無效的參數         |


## 校驗碼計算方法

校驗碼的計算過程包括對指令的每一部分進行XOR運算。以下是計算校驗碼的一個例子：

- **指令**: `0xAA 0x02 0x01`
  - Frame Head = `0xAA`
  - Command = `0x02`
  - Parameter = `0x01`
  - **XOR 運算**: `0xAA ^ 0x02 ^ 0x01 = 0xA9`
  - 校驗碼: `0xA9`

