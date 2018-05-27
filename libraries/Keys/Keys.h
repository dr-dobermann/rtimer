/*
* Library for control a resistive keyboard attached to LCD display shield
*
* (c) 2018, dr-dobermann
^ 
* Project site: https://github.com/dr-dobermann/rtimer.git
*/

#ifndef __KEYS_H__
#define __KEYS_H__

#if defined(__AVR__)
	#include "Arduino.h"
#elif defined(__arm__)
	#include "Arduino.h"
#endif


namespace keys {
	
	const uint16_t 
		// Default keyboard port on the shield
		P_KEYBOARD = 0,
		
		MAX_KEYS = 6;
	
	const uint64_t 
	  DEBOUNCE_TOUT = 20,
	  DBL_CLICK_TOUT = 300,
	  LONG_PRESS_TOUT = 500;

	typedef 
		enum {
			kcNone = 0,
			kcSelect,
			kcLeft,
			kcUp,
			kcDown,
			kcRight
		} KeyCode;
				 
	  					   
	typedef
		enum {
			kmSingle, 
			kmLong, 
			kmDouble
		} KeyMode;
						  
	typedef 
		struct {
			KeyCode code;
			KeyMode mode;
		} Key;

	//-------------------------------------------------------------------------------------------
	class Keyboard {
		public:
			Keyboard(uint16_t kport) :
				kbd_port(kport) {};
			
			Key get_key();
			
			char* get_key_code_name(KeyCode code) {
				static char unknwn[] = "UNKNOWN";
				
				if ( code >= kcNone && code <= kcRight )
					return K_NAMES[code];
				
				return unknwn;
			};
			
			char* get_key_mode_name(KeyMode mode) {
				return K_MODES[mode];
			}
			
		private:
			uint16_t kbd_port;

			Key last_key;
			uint8_t last_effective_key;
			uint64_t last_getkey_time;
			uint64_t last_key_time;
			uint64_t last_ekey_time;
			
			char K_NAMES[6][10] = { "NO_KEY",
							        "SELECT",
							        "LEFT",
							        "UP",
							        "DOWN",
							        "RIGHT" };
					   
			char K_MODES[3][5] = { "SNGL", 
							       "LONG", 
							       "DBL"};
								   
			typedef 
				struct {
					KeyCode code;
					uint16_t resistance;
				} KeyResistance;
								   
			KeyResistance kresistor[MAX_KEYS] = {
				{kcRight,  50},
				{kcUp,     150},
				{kcDown,   350},
				{kcLeft,   500},
				{kcSelect, 850},
				{kcNone,   1023}
			};
	};
};
// end of namespace keys

#endif // __KEYS_H__