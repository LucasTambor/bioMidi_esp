#ifndef _BLE_MIDI_SERVICE_
#define _BLE_MIDI_SERVICE_

#include "stdint.h"
#include "esp_gatt_defs.h"
#include <string.h>
/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_MIDI_CHAR_VAL_LEN_MAX.
*/
#define GATTS_MIDI_CHAR_VAL_LEN_MAX 100
#define BLEMIDI_NUM_PORTS 1

/* Enumeration of the services and characteristics */
enum
{
    BIOMIDI_IDX_SERVICE,
    BIOMIDI_IDX_CHAR,
    BIOMIDI_IDX_VAL,
    BIOMIDI_IDX_CONFIG,

    BIOMIDI_IDX_MAX,
};
uint16_t midi_handle_table[BIOMIDI_IDX_MAX];

extern const uint8_t midi_service_uuid[16];
extern const esp_gatts_attr_db_t midi_serv_gatt_db[BIOMIDI_IDX_MAX];
extern size_t   blemidi_continued_sysex_pos[BLEMIDI_NUM_PORTS];

uint16_t getAttributeIndexByMIDIHandle(uint16_t attributeHandle);

// void handleMIDIReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp);
void handleMIDIWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);
void handleMIDIMtuEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);

void (*blemidi_callback_midi_message_received)(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos);

#endif //_BLE_MIDI_SERVICE_