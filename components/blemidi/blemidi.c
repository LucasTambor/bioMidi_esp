/*
 * BLE MIDI Driver
 *
 * See README.md for usage hints
 *
 * =============================================================================
 *
 * MIT License
 *
 * Copyright (c) 2019 Thorsten Klose (tk@midibox.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * =============================================================================
 */

#include "blemidi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

// Services
#include "ble_common.h"
#include "ble_midi_service.h"


#define BIOMIDI_PROFILE_NUM                 1       // Number of profiles in application
#define BIOMIDI_APP_PROFILE_IDX             0       // Index of BioMidi app
#define BIOMIDI_APP_ID                      0x55    // The Application Profile ID, which is an user-assigned number to identify each profile
#define SVC_INST_ID                         0

#define PREPARE_BUF_MAX_SIZE        2048
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;

// the MTU can be changed by the client during runtime
static size_t blemidi_mtu = GATTS_MIDI_CHAR_VAL_LEN_MAX - 3;





typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
    uint16_t                handle;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;


/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = false, // exclude name to ensure that we don't exceed 31 bytes...
    .include_txpower     = true,
    .min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(midi_service_uuid),
    .p_service_uuid      = midi_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0001,  // The minimum and maximum slave preferred connection intervals are set in units of 1.25 ms
    .max_interval        = 0x000A,  //The BLE MIDI device must request a connection interval of 15 ms or less
    .appearance          = 0x00,
    .manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(midi_service_uuid),
    .p_service_uuid      = midi_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst midi_profile_tab[BIOMIDI_PROFILE_NUM] = {
    [BIOMIDI_APP_PROFILE_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};



void (*blemidi_callback_midi_message_received)(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos);
void (*blemidi_callback_on_connect)();
void (*blemidi_callback_on_disconnect)();

// This timestamp should be increased each mS from the application via blemidi_tick_ms() call:
static uint16_t blemidi_timestamp = 0;

// we buffer outgoing MIDI messages for 10 mS - this should avoid that multiple BLE packets have to be queued for small messages
static uint8_t  blemidi_outbuffer[BLEMIDI_NUM_PORTS][GATTS_MIDI_CHAR_VAL_LEN_MAX];
static uint16_t blemidi_outbuffer_len[BLEMIDI_NUM_PORTS];
static uint16_t blemidi_outbuffer_timestamp_last_flush = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////
// Timestamp handling
////////////////////////////////////////////////////////////////////////////////////////////////////
void blemidi_tick(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  blemidi_timestamp = (tv.tv_sec * 1000 + (tv.tv_usec / 1000)); // 1 mS per increment

  if( (blemidi_outbuffer_timestamp_last_flush > blemidi_timestamp) ||
      (blemidi_timestamp > (blemidi_outbuffer_timestamp_last_flush + BLEMIDI_OUTBUFFER_FLUSH_MS)) ) {
    uint8_t blemidi_port;
    for(blemidi_port=0; blemidi_port < BLEMIDI_NUM_PORTS; ++blemidi_port) {
      blemidi_outbuffer_flush(blemidi_port);
    }

    blemidi_outbuffer_timestamp_last_flush = blemidi_timestamp;
  }
}

uint8_t blemidi_timestamp_high(void)
{
  return (0x80 | ((blemidi_timestamp >> 7) & 0x3f));
}

uint8_t blemidi_timestamp_low(void)
{
  return (0x80 | (blemidi_timestamp & 0x7f));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Flush Output Buffer (normally done by blemidi_tick_ms each 15 mS)
////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t blemidi_outbuffer_flush(uint8_t blemidi_port)
{
  if( blemidi_port >= BLEMIDI_NUM_PORTS )
    return -1; // invalid port

  if( blemidi_outbuffer_len[blemidi_port] > 0 ) {
    esp_ble_gatts_send_indicate(midi_profile_tab[BIOMIDI_APP_PROFILE_IDX].gatts_if, midi_profile_tab[BIOMIDI_APP_PROFILE_IDX].conn_id, midi_handle_table[BIOMIDI_IDX_VAL], blemidi_outbuffer_len[blemidi_port], blemidi_outbuffer[blemidi_port], false);
    blemidi_outbuffer_len[blemidi_port] = 0;
  }
  return 0; // no error
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Push a new MIDI message to the output buffer
////////////////////////////////////////////////////////////////////////////////////////////////////
static int32_t blemidi_outbuffer_push(uint8_t blemidi_port, uint8_t *stream, size_t len)
{
  const size_t max_header_size = 2;

  if( blemidi_port >= BLEMIDI_NUM_PORTS )
    return -1; // invalid port

  // if len >= MTU, it makes sense to send out immediately
  if( len >= (blemidi_mtu-max_header_size) ) {
    // this is very unlikely, since applemidi_send_message() maintains the size
    // but just in case of future extensions, we prepare dynamic memory allocation for "big packets"
    blemidi_outbuffer_flush(blemidi_port);
    {
      size_t packet_len = max_header_size + len;
      uint8_t *packet = malloc(packet_len);
      if( packet == NULL ) {
        return -1; // couldn't create temporary packet
      } else {
        // new packet: with timestampHigh and timestampLow, or in case of continued SysEx packet: only timestampHigh
        packet[0] = blemidi_timestamp_high();
        if( stream[0] >= 0x80 ) {
          packet[1] = blemidi_timestamp_low();
          memcpy((uint8_t *)packet + 2, stream, len);
        } else {
          packet_len -= 1;
          memcpy((uint8_t *)packet + 1, stream, len);
        }

        esp_ble_gatts_send_indicate(midi_profile_tab[BIOMIDI_APP_PROFILE_IDX].gatts_if, midi_profile_tab[BIOMIDI_APP_PROFILE_IDX].conn_id, midi_handle_table[BIOMIDI_IDX_VAL], packet_len, packet, false);
        free(packet);
      }
    }
  } else {
    // flush buffer before adding new message
    if( (blemidi_outbuffer_len[blemidi_port] + len) >= blemidi_mtu )
      blemidi_outbuffer_flush(blemidi_port);

    // adding new message
    if( blemidi_outbuffer_len[blemidi_port] == 0 ) {
      // new packet: with timestampHigh and timestampLow, or in case of continued SysEx packet: only timestampHigh
      blemidi_outbuffer[blemidi_port][blemidi_outbuffer_len[blemidi_port]++] = blemidi_timestamp_high();
      if( stream[0] >= 0x80 ) {
        blemidi_outbuffer[blemidi_port][blemidi_outbuffer_len[blemidi_port]++] = blemidi_timestamp_low();
      }
    } else {
      blemidi_outbuffer[blemidi_port][blemidi_outbuffer_len[blemidi_port]++] = blemidi_timestamp_low();
    }

    memcpy(&blemidi_outbuffer[blemidi_port][blemidi_outbuffer_len[blemidi_port]], stream, len);
    blemidi_outbuffer_len[blemidi_port] += len;
  }

  return 0; // no error
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Sends a BLE MIDI message
////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t blemidi_send_message(uint8_t blemidi_port, uint8_t *stream, size_t len)
{
  const size_t max_header_size = 2;

  if( blemidi_port >= BLEMIDI_NUM_PORTS )
    return -1; // invalid port

  // we've to consider blemidi_mtu
  // if more bytes need to be sent, split over multiple packets
  // this will cost some extra stack space :-/ therefore handled separatly?

  if( len < (blemidi_mtu-max_header_size) ) {
    // just add to output buffer
    blemidi_outbuffer_push(blemidi_port, stream, len);
  } else {
    // sending packets
    size_t max_size = blemidi_mtu - max_header_size; // -3 since blemidi_outbuffer_push() will add the timestamps
    int pos;
    for(pos=0; pos<len; pos += max_size) {
      size_t packet_len = len-pos;
      if( packet_len >= max_size ) {
        packet_len = max_size;
      }
      blemidi_outbuffer_push(blemidi_port, &stream[pos], packet_len);
    }
  }


  return 0; // no error
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// From GATT Server Demo (customized for BLE MIDI service)
////////////////////////////////////////////////////////////////////////////////////////////////////


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLEMIDI_TAG, "advertising start failed");
            }else{
                ESP_LOGI(BLEMIDI_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLEMIDI_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(BLEMIDI_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(BLEMIDI_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void blemidi_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(BLEMIDI_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(BLEMIDI_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(BLEMIDI_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(BLEMIDI_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

static void blemidi_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(BLEMIDI_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(BLEMIDI_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BLEMIDI_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(BLEMIDI_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(BLEMIDI_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(BLEMIDI_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;

            // Register Attributes Tables
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(midi_serv_gatt_db, gatts_if, BIOMIDI_IDX_MAX, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(BLEMIDI_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
            break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_READ_EVT");
            break;
        case ESP_GATTS_WRITE_EVT:
        {
            esp_gatt_status_t status = ESP_GATT_WRITE_NOT_PERMIT;
            int attrIndex;

            if (!param->write.is_prep){

                if( (attrIndex = getAttributeIndexByMIDIHandle(param->write.handle)) < BIOMIDI_IDX_MAX)
                {
                    ESP_LOGI(BLEMIDI_TAG,"BIO MIDI WRITE");
                    handleMIDIWriteEvent(attrIndex, param->write.value, param->write.len);
                    status = ESP_GATT_OK;
                }


            } else {
                /* handle prepare write */
                prepare_write_env.handle = param->write.handle;  // keep track of the handle for ESP_GATTS_EXEC_WRITE_EVT since it doesn't provide it.
                blemidi_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
            break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than blemidi_mtu.
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            blemidi_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);

            // change MTU for BLE MIDI transactions
            if( param->mtu.mtu <= 3 ) {
              blemidi_mtu = 3; // very unlikely...
            } else {
              // we decrease -10 to prevent following driver warning:
              //  (30774) BT_GATT: attribute value too long, to be truncated to 97
              blemidi_mtu = param->mtu.mtu - 3;
              // failsave
              if( blemidi_mtu > (GATTS_MIDI_CHAR_VAL_LEN_MAX-3) )
                blemidi_mtu = (GATTS_MIDI_CHAR_VAL_LEN_MAX-3);
            }
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(BLEMIDI_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(BLEMIDI_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x10;    // max_int = 0x10*1.25ms = 20ms
            conn_params.min_int = 0x0b;    // min_int = 0x0b*1.25ms = 15ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);

            // Call on_connect callback
            blemidi_callback_on_connect();
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(BLEMIDI_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            // Call on_disconnect callback
            blemidi_callback_on_disconnect();
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(BLEMIDI_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }

            //Add calls to start external services
            // else if(param->add_attr_tab.svc_uuid.uuid.uuid32 == midi_service_uuid)
            // {
            // }
            else{
				if(param->add_attr_tab.num_handle != BIOMIDI_IDX_MAX)
				{
					ESP_LOGE(BLEMIDI_TAG,"create attribute table abnormally, num_handle (%d) isn't equal to BIOMIDI_IDX_MAX(%d)", param->add_attr_tab.num_handle, BIOMIDI_IDX_MAX);
				}
				else
				{
					ESP_LOGI(BLEMIDI_TAG,"create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
					memcpy(midi_handle_table, param->add_attr_tab.handles, sizeof(midi_handle_table));
					esp_ble_gatts_start_service(midi_handle_table[BIOMIDI_IDX_SERVICE]);
				}
            }

            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            midi_profile_tab[BIOMIDI_APP_PROFILE_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(BLEMIDI_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < BIOMIDI_PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == midi_profile_tab[idx].gatts_if) {
                if (midi_profile_tab[idx].gatts_cb) {
                    midi_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the BLE MIDI Server
////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t blemidi_init(void *_callback_midi_message_received, void *_callback_midi_on_connect, void *_callback_midi_on_disconnect)
{
  esp_err_t ret;

  // callback will be installed if driver was booted successfully
  blemidi_callback_midi_message_received = NULL;
  blemidi_callback_on_connect = NULL;
  blemidi_callback_on_disconnect = NULL;

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  /* BT Controller and Stack Initialization */
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(BLEMIDI_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    return -1;
  }
  /* Controller is enabled in BLE Mode */
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(BLEMIDI_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    return -2;
  }

  /* Initialize Bluedroid Stack */
  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(BLEMIDI_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
    return -3;
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(BLEMIDI_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
    return -4;
  }

  /**
   * The Bluetooth stack is up and running at this point in the program flow, however the functionality of the
   * application has not been defined yet. The functionality is defined by reacting to events such as what
   * happens when another device tries to read or write parameters and establish a connection. The two main
   * managers of events are the GAP and GATT event handlers. The application needs to register a callback
   * function for each event handler in order to let the application know which functions are going to handle
   * the GAP and GATT events:
  */

  ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret){
    ESP_LOGE(BLEMIDI_TAG, "gatts register error, error code = %x", ret);
    return -5;
  }

  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret){
    ESP_LOGE(BLEMIDI_TAG, "gap register error, error code = %x", ret);
    return -6;
  }

  ret = esp_ble_gatts_app_register(BIOMIDI_APP_ID);
  if (ret){
    ESP_LOGE(BLEMIDI_TAG, "gatts app register error, error code = %x", ret);
    return -7;
  }

  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(GATTS_MIDI_CHAR_VAL_LEN_MAX);
  if (local_mtu_ret){
    ESP_LOGE(BLEMIDI_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    return -8;
  }

  // Output Buffer
  {
    uint32_t blemidi_port;
    for(blemidi_port=0; blemidi_port<BLEMIDI_NUM_PORTS; ++blemidi_port) {
      blemidi_outbuffer_len[blemidi_port] = 0;
      blemidi_continued_sysex_pos[blemidi_port] = 0;
    }
  }

  // Finally install callback
  blemidi_callback_midi_message_received = _callback_midi_message_received;
  blemidi_callback_on_connect = _callback_midi_on_connect;
  blemidi_callback_on_disconnect = _callback_midi_on_disconnect;

  // esp_log_level_set(BLEMIDI_TAG, ESP_LOG_WARN); // can be changed with the "blemidi_debug on" console command

  return 0; // no error
}


