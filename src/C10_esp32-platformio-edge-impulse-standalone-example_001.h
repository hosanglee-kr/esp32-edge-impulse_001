

// https://github.com/edgeimpulse/esp32-platformio-edge-impulse-standalone-example


/* Includes ---------------------------------------------------------------- */

// Modify the following line according to your project name
// Do not forget to update the platformio.ini file to point to the exported
// Edge Impulse Arduino library zip file

#include <DSP_test_inferencing.h>
//////#include <C20_Person_Detection_Classification_inferencing.h>

static const float g_C10_features[] = {
    // copy raw features here (for example from the 'Live classification' page)
    // see https://docs.edgeimpulse.com/docs/running-your-impulse-arduino
};

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int C10_raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, g_C10_features + offset, length * sizeof(float));
    return 0;
}


/**
 * @brief      Arduino setup function
 */
void C10_init()
{


    Serial.println("Edge Impulse Inferencing Demo");
}

/**
 * @brief      Arduino main function
 */
void C10_run()
{
    ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

    if (sizeof(g_C10_features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'g_C10_features' array is not correct. Expected %lu items, but had %lu\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(g_C10_features) / sizeof(float));
        delay(1000);
        return;
    }

    ei_impulse_result_t result = { 0 };

    // the g_C10_features are stored into flash, and we don't want to load everything into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(g_C10_features) / sizeof(g_C10_features[0]);
    features_signal.get_data = &C10_raw_feature_get_data;

    // invoke the impulse
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    ei_printf("run_classifier returned: %d\n", res);

    if (res != 0) return;

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    ei_printf("[");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("%.5f", result.classification[ix].value);
		#if EI_CLASSIFIER_HAS_ANOMALY == 1
			ei_printf(", ");
		#else
			if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
				ei_printf(", ");
			}
		#endif
    }
	#if EI_CLASSIFIER_HAS_ANOMALY == 1
		ei_printf("%.3f", result.anomaly);
	#endif
    ei_printf("]\n");

    // human-readable predictions
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }

	#if EI_CLASSIFIER_HAS_ANOMALY == 1
		ei_printf("    anomaly score: %.3f\n", result.anomaly);
	#endif

    delay(1000);
}

// /**
//  * @brief      Printf function uses vsnprintf and output using Arduino Serial
//  *
//  * @param[in]  format     Variable argument list
//  */
// void ei_printf(const char *format, ...) {
//     static char print_buf[1024] = { 0 };

//     va_list args;
//     va_start(args, format);
//     int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
//     va_end(args);

//     if (r > 0) {
//         Serial.write(print_buf);
//     }
// }
