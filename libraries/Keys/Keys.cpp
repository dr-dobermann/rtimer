#include "Keys.h"

using namespace keys;

Key Keyboard::get_key() {
  
	KeyCode key = kcNone;
            
	uint16_t k = analogRead(kbd_port);
	for (uint16_t i=0; i<MAX_KEYS; i++)
		if (k < kresistor[i].resistance) {
			key = kresistor[i].code;
			break;
		}
  
	// debounce key
	if ( last_key.code != key && millis() - last_getkey_time < DEBOUNCE_TOUT )
		return last_key;

	last_getkey_time = millis();

	if (key == kcNone) {
		if (last_key.code != kcNone) {
			last_effective_key = last_key.code;
			last_ekey_time = millis();
		}
		last_key_time = 0;
		last_key = {kcNone, kmSingle};

		return last_key;
	}

	if (last_key.code == kcNone) {
		last_key_time = millis();
		// check for double press
		if (millis() - last_ekey_time < DBL_CLICK_TOUT && last_effective_key == key) {
			last_key = {key, kmDouble};
		  
			return last_key;
		}
	}

	if (key == last_key.code) {
		// check for long press
		if (last_key_time != 0 && millis() - last_key_time >= LONG_PRESS_TOUT) {
			last_key.mode = kmLong;

			return last_key;   
		}
	}

	last_key = {key, kmSingle};

	return last_key;
}