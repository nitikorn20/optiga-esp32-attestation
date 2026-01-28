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
 * Purpose : Read and display the OPTIGA Trust M device certificate.
 * Design  : See blog-01-attestation.md for detailed explanation
 ********************************************************************************
 * @file    optiga_demo_attestation.c
 * @brief   OPTIGA Trust M attestation helper for ESP32
 * @author  TESA Workshop Team
 * @date    January 10, 2026
 * @version 1.0.0
 *
 * @note    Based on OPTIGA Trust M examples adapted for ESP-IDF
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// --------------------
// User cert config
// --------------------
// Slot for user-provisioned cert (writable object)
#ifndef USER_CERT_SLOT
#define USER_CERT_SLOT 0xE0E1
#endif

// 0 = read-only demo, 1 = write PEM to USER_CERT_SLOT
#ifndef ENABLE_USER_CERT_WRITE
#define ENABLE_USER_CERT_WRITE 0
#endif

// PEM comes from main/cert/servercert.pem (generated at build time)
#ifndef USER_CERT_PEM
#define USER_CERT_PEM ""
#endif

// 1 = read metadata before writing (recommended)
#ifndef ENABLE_USER_CERT_METADATA_CHECK
#define ENABLE_USER_CERT_METADATA_CHECK 1
#endif


#include "esp_log.h"
#include "sdkconfig.h"
#include "optiga_demo_attestation.h"
#include "mbedtls/sha256.h"
#include "mbedtls/x509_crt.h"
#include "user_cert_pem.h"
#include "optiga/pal/pal_os_timer.h"
#include "optiga/common/optiga_lib_return_codes.h"
#include "optiga/optiga_util.h"

// Provided by components/optiga/optiga-trust-m/examples/utilities/optiga_trust.c
extern void read_trust_anchor_from_optiga(uint16_t oid, char *cert_pem,
                                          uint16_t *cert_pem_length);
extern void write_data_object(uint16_t oid, const uint8_t *p_data, uint16_t length);

// --------------------
// Globals
// --------------------
static const char *TAG = "ATTN_DEMO";
static char s_attestation_cert[2048];

static volatile optiga_lib_status_t s_optiga_util_status;
static bool s_user_slot_metadata_ok = false;

static uint8_t s_user_cert_hash[32];
static bool s_user_cert_hash_valid = false;

// --------------------
// Helpers
// --------------------
static void optiga_util_event_callback(void *context, optiga_lib_status_t status)
{
  (void)context;
  s_optiga_util_status = status;
}

static bool read_slot_metadata(uint16_t oid, const char *label)
{
  uint8_t metadata[128] = {0};
  uint16_t metadata_len = sizeof(metadata);
  optiga_util_t *util = optiga_util_create(0, optiga_util_event_callback, NULL);
  if (util == NULL) {
    ESP_LOGE(TAG, "%s: optiga_util_create failed", label);
    return false;
  }

  s_optiga_util_status = OPTIGA_LIB_BUSY;
  optiga_lib_status_t ret = optiga_util_read_metadata(util, oid, metadata, &metadata_len);
  if (ret != OPTIGA_LIB_SUCCESS) {
    ESP_LOGW(TAG, "%s: read metadata start failed (0x%04X)", label, ret);
    optiga_util_destroy(util);
    return false;
  }

  while (s_optiga_util_status == OPTIGA_LIB_BUSY) {
    pal_os_timer_delay_in_milliseconds(10);
  }

  if (s_optiga_util_status != OPTIGA_LIB_SUCCESS) {
    ESP_LOGW(TAG, "%s: read metadata failed (0x%04X)", label, s_optiga_util_status);
    optiga_util_destroy(util);
    return false;
  }

  ESP_LOGI(TAG, "%s length: %u bytes", label, metadata_len);
  if (metadata_len > 0) {
    uint16_t dump_len = (metadata_len > 16) ? 16 : metadata_len;
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, metadata, dump_len, ESP_LOG_INFO);
  }

  optiga_util_destroy(util);
  return true;
}



static void log_cert_fingerprint(const char *label, const char *pem, uint16_t pem_len)
{
  unsigned char hash[32];
  if (mbedtls_sha256((const unsigned char *)pem, pem_len, hash, 0) == 0) {
    char fp[3 * sizeof(hash)] = {0};
    size_t pos = 0;
    for (size_t i = 0; i < sizeof(hash); ++i) {
      int written = snprintf(fp + pos, sizeof(fp) - pos, "%02X%s",
                             hash[i], (i + 1 < sizeof(hash)) ? ":" : "");
      if (written < 0) {
        break;
      }
      pos += (size_t)written;
      if (pos >= sizeof(fp)) {
        break;
      }
    }
    ESP_LOGI(TAG, "%s SHA-256: %s", label, fp);
  } else {
    ESP_LOGW(TAG, "Failed to compute %s fingerprint", label);
  }
}

static void read_and_report_cert(uint16_t oid, const char *label)
{
  uint16_t cert_len = 0;
  read_trust_anchor_from_optiga(oid, s_attestation_cert, &cert_len);
  if (cert_len > 0) {
    ESP_LOGI(TAG, "%s size: %u bytes", label, cert_len);
    log_cert_fingerprint(label, s_attestation_cert, cert_len);
    if (s_user_cert_hash_valid && strcmp(label, "User cert") == 0) {
      mbedtls_x509_crt rb;
      mbedtls_x509_crt_init(&rb);
      int ret = mbedtls_x509_crt_parse(&rb, (const unsigned char *)s_attestation_cert, cert_len);
      if (ret == 0) {
        uint8_t hash[32];
        if (mbedtls_sha256(rb.raw.p, rb.raw.len, hash, 0) == 0) {
          if (memcmp(hash, s_user_cert_hash, sizeof(hash)) == 0) {
            ESP_LOGI(TAG, "User cert verify: match");
          } else {
            ESP_LOGW(TAG, "User cert verify: mismatch");
          }
        } else {
          ESP_LOGW(TAG, "User cert verify: SHA-256 failed");
        }
      } else {
        ESP_LOGW(TAG, "User cert verify: parse failed (-0x%04X)", (unsigned int)(-ret));
      }
      mbedtls_x509_crt_free(&rb);
    }
  } else {
    ESP_LOGW(TAG, "%s read failed or empty", label);
  }
}

static void maybe_write_user_cert(void)
{
#if ENABLE_USER_CERT_WRITE
#if ENABLE_USER_CERT_METADATA_CHECK
  if (!s_user_slot_metadata_ok) {
    ESP_LOGW(TAG, "User slot metadata check failed; skip write");
    return;
  }
#endif
  if (USER_CERT_PEM[0] == '\0') {
    ESP_LOGW(TAG, "USER_CERT_PEM is empty. Skipping write.");
    return;
  }

  mbedtls_x509_crt cert;
  mbedtls_x509_crt_init(&cert);
  int ret = mbedtls_x509_crt_parse(&cert, (const unsigned char *)USER_CERT_PEM,
                                   (uint16_t)strlen(USER_CERT_PEM) + 1);
  if (ret != 0) {
    ESP_LOGE(TAG, "Failed to parse USER_CERT_PEM: -0x%04X", (unsigned int)(-ret));
    mbedtls_x509_crt_free(&cert);
    return;
  }

  if (mbedtls_sha256(cert.raw.p, cert.raw.len, s_user_cert_hash, 0) == 0) {
    s_user_cert_hash_valid = true;
  } else {
    s_user_cert_hash_valid = false;
  }

  write_data_object(USER_CERT_SLOT, cert.raw.p, (uint16_t)cert.raw.len);
  ESP_LOGI(TAG, "Wrote user certificate to slot 0x%04X", USER_CERT_SLOT);
  mbedtls_x509_crt_free(&cert);
#else
  ESP_LOGI(TAG, "User cert write disabled (ENABLE_USER_CERT_WRITE=0)");
#endif
}

void optiga_demo_attestation(void) {
  ESP_LOGI(TAG, "[1] Device identity (Attestation)");
  ESP_LOGI(TAG, "Certificate slot: 0x%04X (CONFIG_OPTIGA_TRUST_M_CERT_SLOT)",
           CONFIG_OPTIGA_TRUST_M_CERT_SLOT);
  ESP_LOGI(TAG, "Reading device certificate from OPTIGA...");

  memset(s_attestation_cert, 0, sizeof(s_attestation_cert));
  // Read pre-provisioned certificate (slot from Kconfig)
  read_and_report_cert(CONFIG_OPTIGA_TRUST_M_CERT_SLOT, "Factory cert");

#if ENABLE_USER_CERT_METADATA_CHECK
  s_user_slot_metadata_ok = read_slot_metadata(USER_CERT_SLOT, "User slot metadata");
#endif

  // Optional: write and read a user-provisioned certificate
  maybe_write_user_cert();
#if ENABLE_USER_CERT_WRITE
  read_and_report_cert(USER_CERT_SLOT, "User cert");
#else
  ESP_LOGI(TAG, "User cert read skipped (ENABLE_USER_CERT_WRITE=0)");
#endif
}