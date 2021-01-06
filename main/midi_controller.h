#ifndef _MIDI_CONTROLLER_H_
#define _MIDI_CONTROLLER_H_

typedef enum {
    STATE_IDLE = 0,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_RUN,
    STATE_CALIBRATION,
    STATE_SLEEP,
    STATE_MAX,
} midi_controller_state_e;


void vMidiController( void *pvParameters );


#endif // _MIDI_CONTROLLER_H_