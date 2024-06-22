#include <Arduino.h>

//#define B05

#ifdef B05
	#include "B05_static_buffer_001.h"
#endif


//#define B10
#ifdef B10
	#include "B10_esp32_microphone_001.h"
#endif


//#define B30
#ifdef B30
    #include "B30_ESP32-Camera_001.h"
#endif

#define B40
#ifdef B40
    #include "B40_ESP32_fusion_001.h"
#endif

void setup() {
	Serial.begin(115200);

	#ifdef B05
		B05_init();
	#endif

	#ifdef B10
		B10_init();
	#endif

	#ifdef B30
		B30_init();
	#endif

	#ifdef B40
		B40_init();
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

	#ifdef B30
		B30_run();
	#endif

	#ifdef B40
		B40_run();
	#endif

}
