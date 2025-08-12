/*
 * gpio_fun.c
 *
 *  Created on: 11 Aug 2025
 *      Author: gsabi
 */

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "esp_err.h" // esp_err_t error handling
#include "esp_log.h" // ESP_LOGI(), ESP_LOGE(), logging macros
#include "freertos/idf_additions.h"

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_store.h"
#include "nimble/ble.h"
#include "soc/gpio_num.h"

#include "ble_gap.h"
#include "gpio_fun.h"

TaskHandle_t bttn_detection_task_handle = NULL;

/*******************BUTTON PINS******************/
int pair_bttn = GPIO_NUM_4;		   // Pair Button
int vol_plus_bttn = GPIO_NUM_18;   // Plus Volume Button
int vol_minus_bttn = GPIO_NUM_19;  // Minus Volume Button
int play_pause_bttn = GPIO_NUM_21; // Play Pause Button

/*************BUTTON STATES**************/
bool prev_pair_bttn_state = false;
bool prev_vol_plus_bttn_state = false;
bool prev_vol_minus_bttn_state = false;
bool prev_media_bttn_state = false;

/*************MAIN DETECTION FUNCTION**************/
void detectButtonPress(void *parameters) {
	while (1) {
		bool curr_bttn = gpio_get_level(pair_bttn);
		if (curr_bttn && !prev_pair_bttn_state) {
			ESP_LOGI("Button Detection", "Pair button pressed");
			// ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_PEER_SEC, NULL);
			// bleprph_advertise();
			/*if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
				// We are connected, terminate connection first
				int rc;
				rc = ble_gap_unpair(&connected_peer_addr);
				ESP_LOGI("Button Detection", "Unpair Event: state=%d", rc);
				rc =
					ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
				if (rc != 0) {
					ESP_LOGE("Button Detection", "Failed to terminate: %d", rc);
				}


			} else {
				// Already disconnected — start advertising immediately
				bleprph_advertise();
			}*/
		};
		prev_pair_bttn_state = curr_bttn;

		/*********VOLUME UP**********/
		curr_bttn = gpio_get_level(vol_plus_bttn);
		if (curr_bttn && !prev_vol_plus_bttn_state) {
			ESP_LOGI("Button Detection", "Plus Volume button pressed");
			sendUpVolumeReport();
		};
		if (!curr_bttn && prev_vol_plus_bttn_state) {
			ESP_LOGI("Button Detection", "Plus Volume button released");
			releaseButtons();
		}
		prev_vol_plus_bttn_state = curr_bttn;

		/*********VOLUME DOWN**********/
		curr_bttn = gpio_get_level(vol_minus_bttn);
		if (curr_bttn && !prev_vol_minus_bttn_state) {
			ESP_LOGI("Button Detection", "Minus Volume button pressed");
			sendDownVolumeReport();
		};
		if (!curr_bttn && prev_vol_minus_bttn_state) {
			ESP_LOGI("Button Detection", "Minus Volume button released");
			releaseButtons();
		}
		prev_vol_minus_bttn_state = curr_bttn;

		/*********PLAY/PAUSE*********/
		curr_bttn = gpio_get_level(play_pause_bttn);
		if (curr_bttn && !prev_media_bttn_state) {
			ESP_LOGI("Button Detection", "Play/Pause button pressed");
			sendMediaReport();
		};
		/*if (!curr_bttn && prev_media_bttn_state) {
			ESP_LOGI("Button Detection", "Play/Pause button released");
			releaseButtons();
		}*/
		prev_media_bttn_state = curr_bttn;

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/****************BUTTON INITIATION****************/
void buttonInit() {
	gpio_config_t button_io_config = {
		.pin_bit_mask = (1ULL << pair_bttn) | (1ULL << vol_plus_bttn) |
						(1ULL << vol_minus_bttn) | (1ULL << play_pause_bttn),
		.mode = GPIO_MODE_INPUT,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.intr_type = GPIO_INTR_DISABLE};
	gpio_config(
		&button_io_config); // Configure all buttons as pulldown input pins

	xTaskCreate(detectButtonPress, "Button Detection", 4096, NULL, 1,
				&bttn_detection_task_handle);
}