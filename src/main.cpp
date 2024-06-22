#include <Arduino.h>


//#define B05
#ifdef B05
	#include "B05_static_buffer_001.h"
#endif


//#define B10
#ifdef B10
	#include "B10_esp32_microphone_001.h"
#endif

//#define B15
#ifdef B15
	#include "B15_esp32_microphone_continuous_001.h"
#endif


//#define B30
#ifdef B30
    #include "B30_ESP32-Camera_001.h"
#endif

//#define B40
#ifdef B40
    #include "B40_ESP32_fusion_001.h"
#endif

#define C10
#ifdef C10
    #include "C10_esp32-platformio-edge-impulse-standalone-example_001.h"
#endif


void setup() {
	Serial.begin(115200);

	#ifdef B05
		B05_init();
	#endif

	#ifdef B10
		B10_init();
	#endif

	#ifdef B15
		B15_init();
	#endif

	#ifdef B30
		B30_init();
	#endif

	#ifdef B40
		B40_init();
	#endif

	#ifdef C10
		C10_init();
	#endif

	Serial.println("11111");
}

void loop() {

	#ifdef B05
		B05_run();
	#endif


	#ifdef B10
		B10_run();
	#endif

	#ifdef B15
		B15_run();
	#endif

	#ifdef B30
		B30_run();
	#endif


	#ifdef B40
		B40_run();
	#endif

	#ifdef C10
		C10_run();
	#endif
}
