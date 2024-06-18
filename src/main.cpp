#include <Arduino.h>




#define B10

#ifdef B10
	#include "B10_esp32_microphone_001.h"
#endif

void setup() {
	Serial.begin(115200);

	#ifdef B10
		B10_init();
	#endif

	Serial.println("11111");
}

void loop() {

	#ifdef B10
		B10_init();
	#endif

}
