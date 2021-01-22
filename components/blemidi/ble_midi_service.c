#include "ble_midi_service.h"

#include "ble_common.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

//********************************************************************************************


//********************************************************************************************

static const char * TAG = "BLE_MIDI_SERVICE";

static const uint8_t char_value[3]                 = {0x80, 0x80, 0xfe};
static const uint8_t blemidi_ccc[2]                = {0x00, 0x00}; // Client Characteristic Configuration (CCC) descriptor which is an additional attribute that describes if the characteristic has notifications enabled.



const uint8_t midi_service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7, 0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03
};

static const uint8_t midi_characteristics_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77
};

/* Full Database Description - Used to add attributes into the database */
const esp_gatts_attr_db_t midi_serv_gatt_db[BIOMIDI_IDX_MAX] =
{
    // Service Declaration
    [BIOMIDI_IDX_SERVICE]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      16, sizeof(midi_service_uuid), (uint8_t *)&midi_service_uuid}},

    /* Characteristic Declaration */
    [BIOMIDI_IDX_CHAR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_writenr_notify}},

    /* Characteristic Value */
    [BIOMIDI_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&midi_characteristics_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_MIDI_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    /* Client Characteristic Configuration Descriptor (this is a BLE2902 descriptor) */
    [BIOMIDI_IDX_CONFIG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(blemidi_ccc), (uint8_t *)blemidi_ccc}},

};

//**************************************************************************************************************

////////////////////////////////////////////////////////////////////////////////////////////////////
// For internal usage only: receives a BLE MIDI packet and calls the specified callback function.
// The user will specify this callback while calling blemidi_init()
////////////////////////////////////////////////////////////////////////////////////////////////////
// to handled continued SysEx
size_t   blemidi_continued_sysex_pos[BLEMIDI_NUM_PORTS];

static int32_t blemidi_receive_packet(uint8_t blemidi_port, uint8_t *stream, size_t len, void *_callback_midi_message_received)
{
  void (*callback_midi_message_received)(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos) = _callback_midi_message_received;

  if( blemidi_port >= BLEMIDI_NUM_PORTS )
    return -1; // invalid port

  ESP_LOGI(TAG, "receive_packet blemidi_port=%d, len=%d, stream:", blemidi_port, len);
  esp_log_buffer_hex(TAG, stream, len);

  // detect continued SysEx
  uint8_t continued_sysex = 0;
  if( len > 2 && (stream[0] & 0x80) && !(stream[1] & 0x80)) {
    continued_sysex = 1;
  } else {
    blemidi_continued_sysex_pos[blemidi_port] = 0;
  }


  if( len < 3 ) {
    ESP_LOGE(TAG, "stream length should be >=3");
    return -1;
  } else if( !(stream[0] & 0x80) ) {
    ESP_LOGE(TAG, "missing timestampHigh");
    return -2;
  } else {
    size_t pos = 0;

    // getting timestamp
    uint16_t timestamp = (stream[pos++] & 0x3f) << 7;

    // parsing stream
    {
      //! Number if expected bytes for a common MIDI event - 1
      const uint8_t midi_expected_bytes_common[8] = {
        2, // Note On
        2, // Note Off
        2, // Poly Preasure
        2, // Controller
        1, // Program Change
        1, // Channel Preasure
        2, // Pitch Bender
        0, // System Message - must be zero, so that mios32_midi_expected_bytes_system[] will be used
      };

      //! Number if expected bytes for a system MIDI event - 1
      const uint8_t midi_expected_bytes_system[16] = {
        1, // SysEx Begin (endless until SysEx End F7)
        1, // MTC Data frame
        2, // Song Position
        1, // Song Select
        0, // Reserved
        0, // Reserved
        0, // Request Tuning Calibration
        0, // SysEx End

        // Note: just only for documentation, Realtime Messages don't change the running status
        0, // MIDI Clock
        0, // MIDI Tick
        0, // MIDI Start
        0, // MIDI Continue
        0, // MIDI Stop
        0, // Reserved
        0, // Active Sense
        0, // Reset
      };

      uint8_t midi_status = continued_sysex ? 0xf0 : 0x00;

      while( pos < len ) {
        if( !(stream[pos] & 0x80) ) {
          if( !continued_sysex ) {
            ESP_LOGE(TAG, "missing timestampLow in parsed message");
            return -3;
          }
        } else {
          timestamp &= ~0x7f;
          timestamp |= stream[pos++] & 0x7f;
          continued_sysex = 0;
          blemidi_continued_sysex_pos[blemidi_port] = 0;
        }

        if( stream[pos] & 0x80 ) {
          midi_status = stream[pos++];
        }

        if( midi_status == 0xf0 ) {
          size_t num_bytes;
          for(num_bytes=0; stream[pos+num_bytes] < 0x80; ++num_bytes) {
            if( (pos+num_bytes) >= len ) {
              break;
            }
          }
          if( _callback_midi_message_received ) {
            callback_midi_message_received(blemidi_port, timestamp, midi_status, &stream[pos], num_bytes, blemidi_continued_sysex_pos[blemidi_port]);
          }
          pos += num_bytes;
          blemidi_continued_sysex_pos[blemidi_port] += num_bytes; // we expect another packet with the remaining SysEx stream
        } else {
          uint8_t num_bytes = midi_expected_bytes_common[(midi_status >> 4) & 0x7];
          if( num_bytes == 0 ) { // System Message
            num_bytes = midi_expected_bytes_system[midi_status & 0xf];
          }

          if( (pos+num_bytes) > len ) {
            ESP_LOGE(TAG, "missing %d bytes in parsed message", num_bytes);
            return -5;
          } else {
            if( _callback_midi_message_received ) {
              callback_midi_message_received(blemidi_port, timestamp, midi_status, &stream[pos], num_bytes, 0);
            }
            pos += num_bytes;
          }
        }
      }
    }
  }

  return 0; // no error
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Dummy callback for demo and debugging purposes
////////////////////////////////////////////////////////////////////////////////////////////////////
void blemidi_receive_packet_callback_for_debugging(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos)
{
  ESP_LOGI(TAG, "receive_packet CALLBACK blemidi_port=%d, timestamp=%d, midi_status=0x%02x, len=%d, continued_sysex_pos=%d, remaining_message:", blemidi_port, timestamp, midi_status, len, continued_sysex_pos);
  esp_log_buffer_hex(TAG, remaining_message, len);
}




//**************************************************************************************************************

uint16_t getAttributeIndexByMIDIHandle(uint16_t attributeHandle) {
	// Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = BIOMIDI_IDX_MAX;
    uint16_t index;

    for(index = 0; index < BIOMIDI_IDX_MAX; ++index)
    {
        if( midi_handle_table[index] == attributeHandle )
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleMIDIWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len) {
    switch(attrIndex) {
        case BIOMIDI_IDX_VAL:
            // ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", *char_val_p, char_val_len);
            // esp_log_buffer_hex(TAG, *char_val_p, char_val_len);
            blemidi_receive_packet(0, char_val_p, char_val_len, blemidi_callback_midi_message_received);
        break;
    }

}
