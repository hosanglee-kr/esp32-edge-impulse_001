#include <Arduino.h>

#define B05

#ifdef B05
	#include "B05_static_buffer_001.h"
#endif

//#define B10

#ifdef B10
	#include "B10_esp32_microphone_001.h"
#endif

void setup() {
	Serial.begin(115200);

	#ifdef B05
		B05_init();
	#endif

	#ifdef B10
		B10_init();
	#endif



	Serial.println("11111");
}

void loop() {
	#ifdef B05
		B05_init();
	#endif

	#ifdef B10
		B10_init();
	#endif
}
