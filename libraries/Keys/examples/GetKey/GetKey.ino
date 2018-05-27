#include <LiquidCrystal.h>

#include <Keys.h>
#include <string.h>

typedef char* pchar;

const uint64_t MIN_TIMEOUT = 250;

const uint16_t 
	rs = 8, 
	en = 9, 
	d4 = 4, 
	d5 = 5, 
	d6 = 6, 
	d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

String lines[2];
uint64_t display_tout = MIN_TIMEOUT;
uint64_t last_time = millis();

keys::Keyboard kbd(keys::P_KEYBOARD);

void setup() {
	lcd.begin(16, 2);
	lcd.clear();
	analogWrite(10, 50);
}

//-------------------------------------------------------------------------------
// display two strings on LCD 16x2
// provides automatic scrolling if displayed string is longer than dislpay width
//-------------------------------------------------------------------------------
void display(char *str, uint8_t line) {

	static pchar lines[2];
	static uint64_t last_disp_time;
	static byte pos[2];

	if (line < 0 || line > 1)
		return;
  
	if (strcmp(lines[line], str) == 0 && strlen(lines[line]) <= 16)
		return;

	if ( strcmp(lines[line], str) != 0 ) {
		if (lines[line] != NULL)
			free(lines[line]);
		lines[line] = malloc(strlen(str));
		strcpy(lines[line], str);
		pos[line] = 0;
	}

	lcd.setCursor(0, line);
	char buf[17];
	int len;
	if (strlen(lines[line]) > 16) {
		if (millis() - last_disp_time >= display_tout) {
			memset(buf, 0, 1);
			len = strlen(lines[line]) - pos[line];
			strncpy(buf, lines[line], len > 16 ? 16 : len);
			lcd.print(buf);
			if (++pos[line] >= strlen(lines[line]))    
				pos[line] = 0;
			last_disp_time = millis();
		}
	}
	else {
		lcd.print("                ");
		lcd.setCursor(0, line);
		lcd.print(lines[line]);
	}
}

void loop() {
 
	display("Press any key...", 0);
	char str[128];

	keys::Key key = kbd.get_key();

	strcpy(str, kbd.get_key_code_name(key.code));
	strcat(str, " : ");
	strcat(str, kbd.get_key_mode_name(key.mode));

	display(str, 1);
}