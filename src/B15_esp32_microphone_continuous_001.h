
// These sketches are tested with 2.0.4 ESP32 Arduino Core
// https://github.com/espressif/arduino-esp32/releases/tag/2.0.4

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK 0

/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */
#include <DSP_test_inferencing.h>

#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** Audio buffers, pointers and selectors */
typedef struct {
	signed short *buffers[2];
	unsigned char buf_select;
	unsigned char buf_ready;
	unsigned int  buf_count;
	unsigned int  n_samples;
} T_B15_inference_t;

static T_B15_inference_t	g_B15_inference;
static const uint32_t 		g_B15_sample_buffer_size = 2048;
static signed short	  		g_B15_sampleBuffer[g_B15_sample_buffer_size];
static bool			  		g_B15_debug_nn		= false;  // Set this to true to see e.g. features generated from the raw signal
static int			  		g_B15_print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static bool			  		g_B15_record_status = true;



///////////////////////////
static void B15_microphone_inference_end(void);
static int B15_i2s_deinit(void);
static int B15_i2s_init(uint32_t sampling_rate);
static int B15_microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static bool B15_microphone_inference_record(void);
static bool B15_microphone_inference_start(uint32_t n_samples);
static void B15_capture_samples(void *arg);
static void B15_audio_inference_callback(uint32_t n_bytes) ;

////////////////////////////
/**
 * @brief      Arduino setup function
 */
void B15_init() {

	 Serial.println("Edge Impulse Inferencing Demo");

	 // summary of inferencing settings (from model_metadata.h)
	 ei_printf("Inferencing settings:\n");
	 ei_printf("\tInterval: ");
	 ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
	 ei_printf(" ms.\n");
	 ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
	 ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
	 ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

	 run_classifier_init();
	 ei_printf("\nStarting continious inference in 2 seconds...\n");
	 ei_sleep(2000);

	 if (B15_microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
		 ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
		 return;
	 }

	 ei_printf("Recording...\n");
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void B15_run() {
	bool m = B15_microphone_inference_record();
	if (!m) {
		ei_printf("ERR: Failed to record audio...\n");
		return;
	}

	signal_t signal;
	signal.total_length		   = EI_CLASSIFIER_SLICE_SIZE;
	signal.get_data			   = &B15_microphone_audio_signal_get_data;
	ei_impulse_result_t result = {0};

	EI_IMPULSE_ERROR	r	   = run_classifier_continuous(&signal, &result, g_B15_debug_nn);
	if (r != EI_IMPULSE_OK) {
		ei_printf("ERR: Failed to run classifier (%d)\n", r);
		return;
	}

	if (++g_B15_print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
		// print the predictions
		ei_printf("Predictions ");
		ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
				  result.timing.dsp, result.timing.classification, result.timing.anomaly);
		ei_printf(": \n");
		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
			ei_printf("    %s: ", result.classification[ix].label);
			ei_printf_float(result.classification[ix].value);
			ei_printf("\n");
		}
		#if EI_CLASSIFIER_HAS_ANOMALY == 1
				ei_printf("    anomaly score: ");
				ei_printf_float(result.anomaly);
				ei_printf("\n");
		#endif

		g_B15_print_results = 0;
	}
}



static void B15_audio_inference_callback(uint32_t n_bytes) {
	for (int i = 0; i < n_bytes >> 1; i++) {
		g_B15_inference.buffers[g_B15_inference.buf_select][g_B15_inference.buf_count++] = g_B15_sampleBuffer[i];

		if (g_B15_inference.buf_count >= g_B15_inference.n_samples) {
			g_B15_inference.buf_select ^= 1;
			g_B15_inference.buf_count = 0;
			g_B15_inference.buf_ready = 1;
		}
	}
}


static void B15_capture_samples(void *arg) {
	const int32_t i2s_bytes_to_read = (uint32_t)arg;
	size_t		  bytes_read		= i2s_bytes_to_read;

	while (g_B15_record_status) {
		/* read data at once from i2s */
		i2s_read((i2s_port_t)1, (void *)g_B15_sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

		if (bytes_read <= 0) {
			ei_printf("Error in I2S read : %d", bytes_read);
		} else {
			if (bytes_read < i2s_bytes_to_read) {
				ei_printf("Partial I2S read");
			}

			// scale the data (otherwise the sound is too quiet)
			for (int x = 0; x < i2s_bytes_to_read / 2; x++) {
				g_B15_sampleBuffer[x] = (int16_t)(g_B15_sampleBuffer[x]) * 8;
			}

			if (g_B15_record_status) {
				B15_audio_inference_callback(i2s_bytes_to_read);
			} else {
				break;
			}
		}
	}
	vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool B15_microphone_inference_start(uint32_t n_samples) {
	g_B15_inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

	if (g_B15_inference.buffers[0] == NULL) {
		return false;
	}

	g_B15_inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

	if (g_B15_inference.buffers[1] == NULL) {
		ei_free(g_B15_inference.buffers[0]);
		return false;
	}

	g_B15_inference.buf_select = 0;
	g_B15_inference.buf_count	 = 0;
	g_B15_inference.n_samples	 = n_samples;
	g_B15_inference.buf_ready	 = 0;

	if (B15_i2s_init(EI_CLASSIFIER_FREQUENCY)) {
		ei_printf("Failed to start I2S!");
	}

	ei_sleep(100);

	g_B15_record_status = true;

	xTaskCreate(B15_capture_samples, "CaptureSamples", 1024 * 32, (void *)g_B15_sample_buffer_size, 10, NULL);

	return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool B15_microphone_inference_record(void) {
	bool ret = true;

	if (g_B15_inference.buf_ready == 1) {
		ei_printf(
			"Error sample buffer overrun. Decrease the number of slices per model window "
			"(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
		ret = false;
	}

	while (g_B15_inference.buf_ready == 0) {
		delay(1);
	}

	g_B15_inference.buf_ready = 0;
	return true;
}



/**
 * Get raw audio signal data
 */
static int B15_microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
	numpy::int16_to_float(&g_B15_inference.buffers[g_B15_inference.buf_select ^ 1][offset], out_ptr, length);

	return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void B15_microphone_inference_end(void) {
	B15_i2s_deinit();
	ei_free(g_B15_inference.buffers[0]);
	ei_free(g_B15_inference.buffers[1]);
}



static int B15_i2s_init(uint32_t sampling_rate) {
	// Start listening for audio: MONO @ 8/16KHz
	i2s_config_t i2s_config = {
		.mode				  = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
		.sample_rate		  = sampling_rate,
		.bits_per_sample	  = (i2s_bits_per_sample_t)16,
		.channel_format		  = I2S_CHANNEL_FMT_ONLY_RIGHT,
		.communication_format = I2S_COMM_FORMAT_STAND_I2S, //I2S_COMM_FORMAT_I2S,
		.intr_alloc_flags	  = 0,
		.dma_buf_count		  = 8,
		.dma_buf_len		  = 512,
		.use_apll			  = false,
		.tx_desc_auto_clear	  = false,
		.fixed_mclk			  = -1,
	};
	i2s_pin_config_t pin_config = {
		.bck_io_num	  = 26,	 // IIS_SCLK
		.ws_io_num	  = 32,	 // IIS_LCLK
		.data_out_num = -1,	 // IIS_DSIN
		.data_in_num  = 33,	 // IIS_DOUT
	};
	esp_err_t ret = 0;

	ret			  = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
	if (ret != ESP_OK) {
		ei_printf("Error in i2s_driver_install");
	}

	ret = i2s_set_pin((i2s_port_t)1, &pin_config);
	if (ret != ESP_OK) {
		ei_printf("Error in i2s_set_pin");
	}

	ret = i2s_zero_dma_buffer((i2s_port_t)1);
	if (ret != ESP_OK) {
		ei_printf("Error in initializing dma buffer with 0");
	}

	return int(ret);
}

static int B15_i2s_deinit(void) {
	i2s_driver_uninstall((i2s_port_t)1);  // stop & destroy i2s driver
	return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
	#error "Invalid model for current sensor."
#endif
