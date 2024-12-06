# BLDC馬達驅動器通訊協議

## 指令格式

每個指令組包含以下元素：
- **Frame Head (1 byte)**: 固定為 `0xAA`，表示指令的開始。
- **Command (1 byte)**: 指令碼，代表要執行的操作。
- **Parameters (可選, 多個bytes)**: 根據指令碼需要傳遞的參數。
- **Checksum (1 byte)**: 透過 XOR 計算所有前面數據得到的校驗碼。

### 指令範例

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

## 回應格式

每個回應組包含以下元素：
- **Frame Head (1 byte)**: 固定為 `0xAA`，表示回應的開始。
- **Status Code (1 byte)**: 指示操作成功或錯誤類型的狀態碼。
- **Reply Value (可選, 多個bytes)**: 根據指令類型可能提供的回應數據。
- **Checksum (1 byte)**: 透過 XOR 計算所有前面數據得到的校驗碼。

### 回應範例

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

## 功能碼說明

| 功能碼 | 描述                | 參數類型         | 回應類型         |
|--------|-------------------|----------------|----------------|
| `0x01` | 設定目標轉速       | 16-bit 數值 (RPM) | 無             |
| `0x02` | 查詢目標轉速       | 無              | 16-bit 數值 (RPM) |
| `0x03` | 啟動/停止馬達      | 1 byte (0/1)    | 無             |
| `0x04` | 設定控制模式       | 1 byte (模式)    | 無             |
| `0x05` | 查詢控制模式       | 無              | 1 byte (模式)    |

## 狀態碼說明

| 狀態碼 | 說明               |
|--------|-------------------|
| `0x00` | 指令執行成功       |
| `0x01` | 無效的數據幀       |
| `0x02` | 校驗碼錯誤         |
| `0x03` | 不支持的功能碼     |
| `0x04` | 無效的參數         |

# BLDC馬達驅動器通訊協議

## 功能碼與對應操作

以下是所有可用的功能碼及其對應的操作說明：

| 功能碼 | 功能名稱               | 功能說明                           |
|--------|----------------------|------------------------------------|
| `0x00` | CMD_SET_POWER_EN     | 設定馬達電源啟用狀態                 |
| `0x01` | CMD_GET_POWER_EN     | 查詢馬達電源啟用狀態                 |
| `0x02` | CMD_SET_TARGET_SPEED | 設定目標轉速                         |
| `0x03` | CMD_GET_TARGET_SPEED | 查詢目標轉速                         |
| `0x04` | CMD_SET_IS_REVERSE   | 設定馬達是否反轉                     |
| `0x05` | CMD_GET_IS_REVERSE   | 查詢馬達是否反轉                     |
| `0x06` | CMD_SET_CONTROL_MODE | 設定馬達控制模式                     |
| `0x07` | CMD_GET_CONTROL_MODE | 查詢馬達控制模式                     |
| `0x08` | CMD_SET_P_VALUE      | 設定PID控制器的比例增益P值           |
| `0x09` | CMD_GET_P_VALUE      | 查詢PID控制器的比例增益P值           |
| `0x0A` | CMD_SET_I_VALUE      | 設定PID控制器的積分增益I值           |
| `0x0B` | CMD_GET_I_VALUE      | 查詢PID控制器的積分增益I值           |
| `0x0C` | CMD_SET_PWM_COMPARE  | 設定PWM比較值                        |
| `0x0D` | CMD_GET_PWM_COMPARE  | 查詢PWM比較值                        |
| `0x0E` | CMD_GET_CURRENT_SPEED| 查詢當前轉速                         |

## 校驗碼計算方法

校驗碼的計算過程包括對指令的每一部分進行XOR運算。以下是計算校驗碼的一個例子：

- **指令**: `0xAA 0x02 0x01`
  - Frame Head = `0xAA`
  - Command = `0x02`
  - Parameter = `0x01`
  - **XOR 運算**: `0xAA ^ 0x02 ^ 0x01 = 0xA9`
  - 校驗碼: `0xA9`

這份詳細的功能碼列表將有助於開發者或系統集成者準確地實施與BLDC馬達的通訊。
