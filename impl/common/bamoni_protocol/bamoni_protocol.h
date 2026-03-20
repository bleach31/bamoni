#pragma once
#include <stdint.h>

// --- デバイス識別 ---
#define BAMONI_DEVICE_NAME   "bamoni-P"
#define BAMONI_COMPANY_ID    0xFFFF

// --- タイミング ---
#define BAMONI_MEASURE_INTERVAL_SEC  30   // 計測間隔 (12分)
#define BAMONI_ADVERTISE_DUR_SEC     10    // アドバタイズ送信時間

// --- 履歴バッファ ---
#define BAMONI_MAX_HISTORY_LEN  48  // 12分 x 48 = 9.6時間分

// --- パケット構造 ---
// Manufacturer Data レイアウト:
//   CompanyID  (2B, uint16_t) = BAMONI_COMPANY_ID
//   CurVolt    (2B, uint16_t) mV単位
//   HistCount  (1B, uint8_t)  履歴件数 (0..BAMONI_MAX_HISTORY_LEN)
//   History[]  (2B x N, uint16_t[]) mV単位, 最新→過去順

#define BAMONI_PACKET_HEADER_SIZE  (2 + 2 + 1)  // CompanyID + CurVolt + HistCount

struct BamoniPacket {
    uint16_t companyId;
    uint16_t currentVoltage;  // mV
    uint8_t  historyCount;
    uint16_t history[BAMONI_MAX_HISTORY_LEN];  // mV, 最新→過去順
} __attribute__((packed));
