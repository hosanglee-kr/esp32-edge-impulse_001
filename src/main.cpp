#include <Arduino.h>

// The build version comes from an environment variable. Use the VERSION
// define wherever the version is needed.
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg) STRINGIZER(arg)
#define VERSION STR_VALUE(BUILD_VERSION)


#include "S10_streams-i2s-webserver_wav_001.h"

void setup(){
  //delay(5000);

  Serial.begin(115200);
  Serial.print("Version: ");
  Serial.println(VERSION);

  S10_setup();

}

void loop(){

  S10_loop();
  
}
