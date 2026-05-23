# OTA 系统调试记录 — 2025-05-24

> 项目: GS_ETERNALCHIP STM32F411CEUx 双 Bank OTA 固件升级
> 目标: 实现 AES-256-CTR 加密传输 + ECDSA 签名验证全链路

---

## 1. 问题总览

| # | 严重度 | 模块 | 问题简述 | 根因 | 状态 |
|---|--------|------|----------|------|------|
| P0-1 | CRITICAL | Flash | WriteConfig 读-改-写破坏相邻数据 | 直接覆盖 Sector，未保留已有内容 | ✅ 已修复 |
| P0-2 | CRITICAL | Flash | EraseSlot 一次性擦除整 Bank | 应逐扇区擦除，避免长时间阻塞 | ✅ 已修复 |
| P0-3 | HIGH | Boot | VerifyAndJumpRelaxed 缺失 | 升级后首次跳转无降级验证路径 | ✅ 已修复 |
| P1-1 | HIGH | 全局 | static 缓冲区栈溢出 | Boot 栈仅 1KB，大缓冲区未用 static | ✅ 已修复 |
| P1-2 | HIGH | YMODEM | fw_size 赋值时序错误 | `*p_filesize` 在 `done:` 标签赋值，DataCallback 期间为 0 | ✅ 已修复 |
| P1-3 | HIGH | Transport | 签名提取死循环 | RECV_STATE_SIGNATURE 完成后未设置 `consumed = len` | ✅ 已修复 |
| P2-1 | CRITICAL | AES | 手写 AES state 矩阵转置错误 | 加密结果与标准库不一致 | ✅ 已修复 |
| P2-2 | CRITICAL | AES | CTR 模式跨调用密钥流丢失 | `bi` 每次调用重置，非对齐 chunk 导致 counter 失步 | ✅ 已修复 |

---

## 2. 详细分析

### P0-1: WriteConfig 读-改-写破坏 Flash

**现象**: 写入 OTA 配置后，Sector 1 中的 Trace 日志、Crash Dump 被清零。

**根因**: `WriteConfig()` 直接调用 `BspFlash_Write()` 写入 `ota_config_t`，但 Sector 1 还存储了其他数据（Trace、Crash Dump）。Flash 写入前必须先读出整个扇区、修改目标区域、擦除扇区、再写回。

**修复**: 实现 read-modify-write 流程：
1. 读出 Sector 1 全部数据到 RAM 缓冲区
2. 在缓冲区中修改 `ota_config_t` 对应区域
3. 擦除 Sector 1
4. 写回缓冲区

**涉及文件**: APP `App/task_ota.c`, Boot `main.c`

**提交**: APP `4e79200`, Boot `3b7ff75`

---

### P0-2: EraseSlot 一次性擦除整 Bank

**现象**: 擦除目标 Bank 耗时过长，且 Watchdog 可能超时。

**根因**: `EraseSlot()` 调用 `HAL_FLASHEx_Erase()` 一次性擦除多个扇区。STM32F4 的 Flash 擦除是阻塞操作，16KB 扇区擦除约 250ms~1s，4 个扇区可能阻塞数秒。

**修复**: 改为逐扇区擦除，每个扇区之间喂看门狗：
```c
for (sector = start_sector; sector <= end_sector; sector++) {
    EraseInitStruct.Sector = sector;
    HAL_FLASHEx_Erase(&EraseInitStruct, &sect_error);
    HAL_IWDG_Refresh(&hiwdg);  // 每扇区喂狗
}
```

**涉及文件**: APP `App/task_ota.c`, Boot `main.c`

**提交**: APP `c4dcbe8`, Boot `d26714d`

---

### P0-3: Boot VerifyAndJumpRelaxed 缺失

**现象**: 升级完成后首次跳转到新 APP 时，Boot 缺少"宽松验证"路径。

**根因**: Boot 原有验证逻辑过于严格，新 APP 可能暂时无法通过完整验证，需要一个降级验证路径确保升级后能正常跳转。

**修复**: 新增 `VerifyAndJumpRelaxed()` 函数，提供降级验证路径。

**涉及文件**: Boot `main.c`

**提交**: Boot `9d335ba`

---

### P1-1: static 缓冲区栈溢出

**现象**: Boot 运行时偶发 HardFault。

**根因**: Boot 裸机栈仅 1KB（`Stack_Size EQU 0x400`），代码中存在大型局部缓冲区（如 1024 字节 YMODEM 包缓冲区），直接放在栈上导致栈溢出。

**修复**: 所有大缓冲区改为 `static`，放置在 BSS 段而非栈：
```c
// 修复前
uint8_t packet_buf[1024];  // 栈上分配，危险

// 修复后
static uint8_t packet_buf[1024];  // BSS 段，安全
```

**涉及文件**: APP + Boot 多个文件

---

### P1-2: YMODEM fw_size 赋值时序错误

**现象**: `OtaTransport_DataCallback()` 中 `*s_p_fw_size` 始终为 0，导致固件大小计算异常。

**根因**: YMODEM 协议在 Packet 0 解析出 `file_size`，但原代码在 `done:` 标签处才执行 `*p_filesize = file_size`。而 DataCallback 在 Packet 1 开始就已经被调用，此时 `fw_size` 仍为 0。

**修复**: 将 `*p_filesize = file_size` 提前到 Packet 0 解析成功后立即赋值：
```c
// Packet 0 解析后
*p_filesize = file_size;  // 立即赋值，DataCallback 需要用
// ...
// done: 标签处
*p_filesize = file_size;  // 重复赋值，无副作用
```

**涉及文件**: `Middlewares/Third_Party/YMODEM/ymodem.c`

---

### P1-3: 签名提取死循环

**现象**: `DataCallback` 进入 `RECV_STATE_SIGNATURE` 后无限循环，串口不再输出日志。

**根因**: `RECV_STATE_SIGNATURE` 收满 64 字节签名后，执行 `break` 跳出 switch，但外层 `while (consumed < len)` 条件仍满足（`consumed` 未更新），导致无限循环进入该 case。

**修复**: 签名提取完成后设置 `consumed = len` 跳出 while 循环：
```c
case RECV_STATE_SIGNATURE:
{
    // ... collect signature ...
    if (s_sig_collected >= OTA_ECDSA_SIG_SIZE) {
        memcpy(s_extracted_sig, s_sig_buf, OTA_ECDSA_SIG_SIZE);
        consumed = len;  // 跳出 while 循环
    }
    break;
}
```

**涉及文件**: `Module/ota/ota_transport.c`

---

### P2-1: 手写 AES state 矩阵转置错误 ⚠️ 关键

**现象**: AES-256-CTR 解密后的固件数据完全错误，ECDSA 验签失败。

**根因**: 手写 AES 实现中，`state_t` 定义为 `uint8_t[4][4]`，但 SubBytes/ShiftRows/MixColumns 等操作对 state 矩阵的行列访问顺序与 AES 标准（FIPS-197）不一致。具体表现为 state 的转置处理错误，导致加密输出与标准 AES 库完全不同。

**影响**: PC 端使用 Python `cryptography` 库（标准 AES），STM32 端手写 AES 输出不匹配 → CTR 密钥流不同 → 解密结果全部错误。

**修复**: 整体替换为原版 [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c) 库，仅保留 AES-256 + CTR 模式：
- `Vendor/tiny-AES-c/aes.c` — 原版 AES-256+CTR 实现
- `Vendor/tiny-AES-c/aes.h` — AES_ctx 结构体定义
- `Module/crypto/ota_aes.c` — 封装层（OtaAes_Init/DecryptChunk/Deinit）

**教训**: 密码学算法不要手写，直接用经过广泛验证的开源实现。

**涉及文件**: `Vendor/tiny-AES-c/aes.c`, `Vendor/tiny-AES-c/aes.h`, `Module/crypto/ota_aes.c`

---

### P2-2: AES-CTR 跨调用密钥流丢失 ⚠️ 关键

**现象**: 使用原版 kokke/tiny-AES-c 后，ECDSA 验签仍然失败。日志显示固件大小正确、签名提取正确，但 hash 不匹配。

**根因**: 原版 `AES_CTR_xcrypt_buffer()` 中，密钥流索引 `bi` 每次调用从 `AES_BLOCKLEN`（16）重新开始，生成的密钥流块存储在局部变量 `buffer[16]` 中。函数返回后，未用完的密钥流字节丢失。

OTA 场景下的数据流：
```
第一个 YMODEM packet: 1024 字节
├── 12 字节 nonce（被截取）
└── 1012 字节加密固件 → AES_CTR_xcrypt_buffer(ctx, buf, 1012)
    └── 1012 % 16 = 4 → 最后 4 字节密钥流被丢弃
```

第二次调用时，`bi` 从 16 重置，生成新的密钥流块（counter+1），但 PC 端 CTR 还在上一个 counter 的偏移位置。从此 STM32 端的 counter 与 PC 端**永久失步**，所有后续解密数据全部错误。

**修复**: 修改 `AES_ctx` 结构体，增加密钥流缓存字段：
```c
struct AES_ctx {
    uint8_t RoundKey[AES_keyExpSize];
    uint8_t Iv[AES_BLOCKLEN];
    uint8_t keystream[AES_BLOCKLEN];  // 新增：缓存密钥流块
    uint8_t block_offset;              // 新增：当前密钥流位置 (0..15)
};
```

重写 `AES_CTR_xcrypt_buffer()`：
- 从 `ctx->block_offset` 恢复密钥流位置
- 密钥流缓存到 `ctx->keystream`
- `AES_init_ctx_iv()` 初始化 `block_offset = AES_BLOCKLEN`（触发首次生成）

**关键代码变更**:
```c
// 修复前（原版）
void AES_CTR_xcrypt_buffer(struct AES_ctx *ctx, uint8_t *buf, uint32_t length) {
    uint8_t buffer[AES_BLOCKLEN];  // 局部变量，调用后丢失
    int bi;
    for (i = 0, bi = AES_BLOCKLEN; i < length; ++i, ++bi) {
        // bi 每次从 16 重新开始
    }
}

// 修复后（流式）
void AES_CTR_xcrypt_buffer(struct AES_ctx *ctx, uint8_t *buf, uint32_t length) {
    for (i = 0; i < length; ++i) {
        if (ctx->block_offset == AES_BLOCKLEN) {
            // 生成新密钥流块，缓存到 ctx->keystream
            memcpy(ctx->keystream, ctx->Iv, AES_BLOCKLEN);
            Cipher((state_t *)ctx->keystream, ctx->RoundKey);
            // 递增 counter
            ctx->block_offset = 0;
        }
        buf[i] ^= ctx->keystream[ctx->block_offset];
        ctx->block_offset++;  // 保持位置，跨调用
    }
}
```

**验证结果**:
```
I/ECDSA  [05584] ECDSA verify OK.
I/OTA    [05585] SHA-256: 921E0B91...
[BOOT]   SHA-256 OK.
```

**涉及文件**: `Vendor/tiny-AES-c/aes.h`, `Vendor/tiny-AES-c/aes.c`

---

## 3. 架构改进

### 3.1 目录重构

```
Module/
├── crypto/           ← 新增
│   ├── ota_aes.c/h        AES-256-CTR 封装层
│   ├── ota_aes_key.c/h    预共享密钥存储
│   └── ota_ecdsa.c/h      ECDSA 签名验证（从 ota/ 移来）
├── diag/             ← 原 crash_dump 重命名
│   ├── crash_dump.c/h
│   └── ota_trace.c/h
└── ota/
    ├── ota_transport.c/h  三阶段状态机（NONCE→FIRMWARE→SIGNATURE）
    └── task_ota.c         OTA 任务主逻辑

Vendor/
└── tiny-AES-c/       ← 新增
    ├── aes.c              原版 kokke/tiny-AES-c（AES-256+CTR）
    └── aes.h
```

### 3.2 加密包格式

```
┌──────────┬────────────────────────┬──────────────┐
│ Nonce    │ Encrypted Firmware     │ ECDSA Sig    │
│ 12 bytes │ N bytes                │ 64 bytes     │
└──────────┴────────────────────────┴──────────────┘
总 overhead = 76 bytes
```

- **密钥**: AES-256 PSK（嵌入 `ota_aes_key.c`）
- **模式**: AES-256-CTR（流式，无填充，固件大小不变）
- **IV 构造**: `nonce(12B) || 0x00000000(4B)`
- **签名**: 对明文固件 SHA-256 hash 做 ECDSA 签名

### 3.3 传输状态机

```
RECV_STATE_NONCE ──(12B)──→ RECV_STATE_FIRMWARE ──(N bytes)──→ RECV_STATE_SIGNATURE
    │                           │                                    │
    └─ OtaAes_Init()            └─ OtaAes_DecryptChunk()             └─ OtaTransport_GetSignature()
                                  └─ WriteDecryptedFirmware()
```

---

## 4. 调试经验总结

### 4.1 Flash 操作要点

1. **读-改-写**: Flash 扇区是最小擦除单位。修改扇区内部分数据时，必须读出全部 → 修改 → 擦除 → 写回。
2. **逐扇区擦除**: 多扇区擦除要逐个进行，每扇区后喂看门狗，避免 WDT 复位。
3. **Boot 栈极小**: 裸机 Boot 栈仅 1KB，所有 >64B 的缓冲区必须用 `static`。

### 4.2 AES-CTR 流式解密要点

1. **CTR 是流密码**: 密钥流必须连续，不能有断点。每次调用的未用完字节必须缓存。
2. **chunk 对齐问题**: 数据分块传输时，块大小几乎不可能总是 16 字节对齐。AES 库必须支持跨调用状态保持。
3. **不要手写密码学算法**: 用经过验证的开源库（如 kokke/tiny-AES-c），但要注意原始库是否支持流式调用。

### 4.3 调试方法

1. **先跑通明文，再加密**: 确保基础传输链路正确后再引入加密。
2. **对比 PC 和 STM32 的中间数据**: 打印 nonce、counter、解密后第一个字节，与 Python 端对比。
3. **逐步缩小问题范围**: 从"验签失败"→"hash 不匹配"→"解密数据错误"→"CTR counter 失步"→"chunk 对齐 bug"。

---

## 5. 提交记录

| Tag | 工程 | 说明 |
|-----|------|------|
| `v1.0.2` | APP | 明文 OTA 全链路通过 |
| `boot-v1.0.0` | Boot | VerifyAndJumpRelaxed + 逐扇区擦除 |

**待提交**: AES-256-CTR 加密传输全链路通过（含 P2-1/P2-2 修复）

---

## 6. 已知限制与后续工作

1. **密钥管理**: 当前 AES 密钥硬编码在 `ota_aes_key.c`，生产环境需改用安全存储（如 STM32 OTP 或外部安全元件）。
2. **Nonce 重放**: 当前 nonce 由 PC 端随机生成，每次 OTA 不同。可考虑加时间戳防重放。
3. **Boot ECDSA 验签**: Boot 端目前只做 SHA-256 完整性校验，未做 ECDSA 签名验证。APP 端已实现完整验签。
4. **回滚保护**: `VerifyAndJumpRelaxed` 提供了基本回滚路径，但缺少版本号防降级机制。

---

*文档生成时间: 2025-05-24*
*最终测试结果: AES-256-CTR 加密 OTA + ECDSA 验签 — ✅ PASS*
