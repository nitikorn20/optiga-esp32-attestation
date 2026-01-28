/********************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2024-2025 TESA
 * All rights reserved.</center></h2>
 *
 * This source code and any compilation or derivative thereof is the
 * proprietary information of TESA and is confidential in nature.
 *
 ********************************************************************************
 * Project : OPTIGA Trust M Connectivity Tutorial Series
 ********************************************************************************
 * Module  : Part 1 - Device Attestation Demo
 * Purpose : Demonstrate OPTIGA Trust M device identity using the on-chip
 *           certificate to prove hardware-rooted identity.
 * Design  : See blog-01-attestation.md for detailed explanation
 ********************************************************************************
 * @file    main.c
 * @brief   OPTIGA Trust M attestation demo for ESP32
 * @author  TESA Workshop Team
 * @date    January 10, 2026
 * @version 1.0.0
 *
 * @note    Based on OPTIGA Trust M examples adapted for ESP-IDF
 *          Modified for autonomous demo pattern (no external scripts required)
 *
 * @see     https://github.com/TESA-Workshops/optiga-esp32-attestation
 ********************************************************************************
 * Original Copyright Notice:
 * (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG.  SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/* -------------------------------------------------------------------- */
/* Includes                                                             */
/* -------------------------------------------------------------------- */
#include <stdbool.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "optiga_demo_attestation.h"

// Provided by
// components/optiga/optiga-trust-m/examples/utilities/optiga_trust.c
extern void optiga_trust_init(void);

static const char *TAG = "ATTN_MAIN";

void app_main(void) {
  ESP_LOGI(TAG, "==============================");
  ESP_LOGI(TAG, "OPTIGA Trust M - Attestation");
  ESP_LOGI(TAG, "ESP32 Demo");
  ESP_LOGI(TAG, "==============================");

  ESP_LOGI(TAG, "Initializing OPTIGA Trust M...");

    bool wdt_disabled = false;
  if (esp_task_wdt_status(NULL) == ESP_OK) {
    if (esp_task_wdt_delete(NULL) == ESP_OK) {
      wdt_disabled = true;
    }
  }
  optiga_trust_init();
  if (wdt_disabled) {
    esp_task_wdt_add(NULL);
  }
  optiga_demo_attestation();

  ESP_LOGI(TAG, "Demo completed.");
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
