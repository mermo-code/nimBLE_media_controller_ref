/*
 * ble_gap.c
 *
 *  Created on: 9 Aug 2025
 *      Author: gsabi
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// === ESP-IDF Core ===
#include "esp_err.h" // esp_err_t error handling
#include "esp_log.h" // ESP_LOGI(), ESP_LOGE(), logging macros
#include "freertos/idf_additions.h"
#include "hid_vars.h"
#include "host/ble_gatt.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_mbuf.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"

// === BLE Host Layer ===
#include "host/ble_gap.h"  // GAP (advertising, connection handling)
#include "host/ble_hs.h"   // Main BLE host API (GAP, GATT, SM config)
#include "host/ble_uuid.h" // UUID handling (16/128-bit UUIDs)

// === BLE Services ===
#include "portmacro.h"
#include "services/gap/ble_svc_gap.h" // Generic Access Profile (GAP) service

// #include "ble_gatt.h"
#include "ble_gap.h"
#include "ble_init.h"

uint16_t conn_handle;
ble_addr_t connected_peer_addr;
bool HID_REPORT_NOTIFY_STATE;

int bleprph_gap_event(struct ble_gap_event *event, void *arg);

void print_conn_desc(struct ble_gap_conn_desc *desc) {
	ESP_LOGI(TAG, "connection_handle=%d", desc->conn_handle);
	ESP_LOGI(TAG, "our_ota_addr_type=%s, our_ota_addr=%s",
			 ble_addr_type_str(desc->our_ota_addr.type),
			 addr_str(desc->our_ota_addr.val));
	ESP_LOGI(TAG, "peer_ota_addr_type=%s, peer_ota_addr=%s",
			 ble_addr_type_str(desc->peer_ota_addr.type),
			 addr_str(desc->peer_ota_addr.val));
	ESP_LOGI(TAG, "our_id_addr_type=%s, our_id_addr=%s",
			 ble_addr_type_str(desc->our_id_addr.type),
			 addr_str(desc->our_id_addr.val));
	ESP_LOGI(TAG, "peer_id_addr_type=%s, peer_id_addr=%s",
			 ble_addr_type_str(desc->peer_id_addr.type),
			 addr_str(desc->peer_id_addr.val));
	ESP_LOGI(TAG,
			 "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
			 "encrypted=%d, authenticated=%d, bonded=%d\n",
			 desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
			 desc->sec_state.encrypted, desc->sec_state.authenticated,
			 desc->sec_state.bonded);
}

int user_parse(const struct ble_hs_adv_field *data, void *arg) {
	ESP_LOGI(TAG, "Parse data: field len %d, type %x, total %d bytes",
			 data->length, data->type,
			 data->length + 2); /* first byte type and second byte length */
	return 1;
}

/***************SENDING HID NOTIFY REPORT*****************/
void sendUpVolumeReport() {
	int rc;
	struct os_mbuf *om;

	if (HID_REPORT_NOTIFY_STATE) {
		hid_input_report = 0;
		hid_input_report = (1 << 0);
		om = ble_hs_mbuf_from_flat(&hid_input_report, sizeof(hid_input_report));
		rc = ble_gatts_notify_custom(conn_handle, HANDLE_HID_REPORT, om);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void sendDownVolumeReport() {
	int rc;
	struct os_mbuf *om;

	if (HID_REPORT_NOTIFY_STATE) {
		hid_input_report = 0;
		hid_input_report = (1 << 1);
		om = ble_hs_mbuf_from_flat(&hid_input_report, sizeof(hid_input_report));
		rc = ble_gatts_notify_custom(conn_handle, HANDLE_HID_REPORT, om);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void sendMediaReport() {
	int rc;
	struct os_mbuf *om;

	if (HID_REPORT_NOTIFY_STATE) {
		hid_input_report = 0;
		hid_input_report = (1 << 2);
		om = ble_hs_mbuf_from_flat(&hid_input_report, sizeof(hid_input_report));
		rc = ble_gatts_notify_custom(conn_handle, HANDLE_HID_REPORT, om);
		if (rc) {
			ESP_LOGW("Notify", "Error:%d", rc);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);

		hid_input_report = 0;
		om = ble_hs_mbuf_from_flat(&hid_input_report, sizeof(hid_input_report));
		rc = ble_gatts_notify_custom(conn_handle, HANDLE_HID_REPORT, om);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void releaseButtons() {
	int rc;
	struct os_mbuf *om;

	if (HID_REPORT_NOTIFY_STATE) {
		hid_input_report = 0;
		om = ble_hs_mbuf_from_flat(&hid_input_report, sizeof(hid_input_report));
		rc = ble_gatts_notify_custom(conn_handle, HANDLE_HID_REPORT, om);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/******************ADVERTISING********************/
void bleprph_advertise(void) {
	if (!ble_gap_adv_active()) {
		struct ble_gap_adv_params adv_params;
		struct ble_hs_adv_fields fields;
		const char *name;
		int rc;

		/**
		 *  Set the advertisement data included in our advertisements:
		 *     o Flags (indicates advertisement type and other general info).
		 *     o Advertising tx power.
		 *     o Device name.
		 *     o 16-bit service UUIDs (alert notifications).
		 */

		memset(&fields, 0, sizeof fields);

		/* Advertise two flags:
		 *     o Discoverability in forthcoming advertisement (general)
		 *     o BLE-only (BR/EDR unsupported).
		 */
		fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

		/* Indicate that the TX power level field should be included; have the
		 * stack fill this value automatically.  This is done by assigning the
		 * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
		 */
		fields.tx_pwr_lvl_is_present = 1;
		fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

		fields.adv_itvl_is_present = 1;
		fields.adv_itvl = 40;

		// name = ble_svc_gap_device_name();
		name = "Controller";
		fields.name = (uint8_t *)name;
		fields.name_len = strlen(name);
		fields.name_is_complete = 1;

		fields.appearance = 0x04C0;
		fields.appearance_is_present = 1;

		fields.uuids16 =
			(ble_uuid16_t[]){BLE_UUID16_INIT(GATT_UUID_HID_SERVICE)};
		fields.num_uuids16 = 1;
		fields.uuids16_is_complete = 1;

		uint8_t buf[50], buf_sz;
		rc = ble_hs_adv_set_fields(&fields, buf, &buf_sz, 50);
		if (rc != 0) {
			ESP_LOGE(TAG, "Error setting advertisement data to buf, Reason=%d",
					 rc);
			return;
		}
		/*if (buf_sz > BLE_HS_ADV_MAX_SZ) {
			ESP_LOGE(TAG,
					 "Too long advertising data: name %s, appearance %x, uuid16
		"
					 "%x, advsize = %d",
					 name, fields.appearance, GATT_UUID_HID_SERVICE, buf_sz);
			ble_hs_adv_parse(buf, buf_sz, user_parse, NULL);
			return;
		}*/

		rc = ble_gap_adv_set_fields(&fields);
		if (rc != 0) {
			ESP_LOGE(TAG, "Error setting advertisement data; rc=%d", rc);
			return;
		}

		/* Begin advertising. */
		memset(&adv_params, 0, sizeof adv_params);
		adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
		adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
		rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
							   bleprph_gap_event, NULL);
		if (rc != 0) {
			ESP_LOGE(TAG, "Error enabling advertisement, Reason=%d", rc);
			return;
		}
		ESP_LOGI(TAG, "Advertisement started");
	}
}

/***************GAP EVENTS HANDLER***************/
int bleprph_gap_event(struct ble_gap_event *event, void *arg) {
	struct ble_gap_conn_desc desc;
	int rc;

	switch (event->type) {
	/********************************************************************/
	case BLE_GAP_EVENT_CONNECT: // CONNECTION
		ESP_LOGI(TAG, "Connection (%d) %s", event->connect.conn_handle,
				 event->connect.status == 0 ? "established" : "failed");
		if (event->connect.status == 0) {
			rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
			assert(rc == 0);
			print_conn_desc(&desc);
			connected_peer_addr = desc.peer_id_addr;
		} else {
			bleprph_advertise();
		}
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_DISCONNECT: // DISCONNECTION
		ESP_LOGI(TAG, "Disconnected (%d), Reason=%d",
				 event->disconnect.conn.conn_handle, event->disconnect.reason);
		print_conn_desc(&event->disconnect.conn);
		bleprph_advertise();
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_CONN_UPDATE_REQ: // CONNECTION UPDATE REQUEST
		/* The central requested the connection parameters update. */
		ESP_LOGI(TAG, "Connection (%d) update request",
				 event->conn_update_req.conn_handle);
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_CONN_UPDATE: // CONNECTION UPDATE
		/* The central has updated the connection parameters. */
		ESP_LOGI(TAG, "Connection (%d) update %s",
				 event->conn_update.conn_handle,
				 event->conn_update.status == 0 ? "succeeded" : "failed");

		rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
		if (rc == 0) {
			print_conn_desc(&desc);
		} else {
			ESP_LOGW(TAG, "Connection (%d) update: conn handle not found",
					 event->conn_update.conn_handle);
		}
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_ADV_COMPLETE: // ADVERTISEMENT COMPLETE
		ESP_LOGI(TAG, "Advertisement complete, Reason=%d",
				 event->adv_complete.reason);

		// Optional: Only restart advertising if not active
		// bleprph_advertise();
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_ENC_CHANGE: // ENCRYPTION EVENT
		/* Encryption has been enabled or disabled for this connection. */
		ESP_LOGI(TAG, "Connection (%d) encryption change event, Status=%d ",
				 event->enc_change.conn_handle, event->enc_change.status);
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_MTU: // MTU EXCHANGE
		ESP_LOGI(TAG, "MTU updated: conn_handle=%d, ch_id=%d, mtu_val=%d",
				 event->mtu.conn_handle, event->mtu.channel_id,
				 event->mtu.value);
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_REPEAT_PAIRING: // REPEAT PAIRING
		/* We already have a bond with the peer, but it is attempting to
		 * establish a new secure link.  This app sacrifices security for
		 * convenience: just throw away the old bond and accept the new link.
		 */

		/* Delete the old bond. */
		rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
		assert(rc == 0);
		ble_store_util_delete_peer(&desc.peer_id_addr);

		/* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
		 * continue with the pairing operation.
		 */
		return BLE_GAP_REPEAT_PAIRING_RETRY;
	/********************************************************************/
	case BLE_GAP_EVENT_SUBSCRIBE: // SUBSCRIPTION EVENT
		ESP_LOGI(TAG, "Subscription event on attr_handle=%d",
				 event->subscribe.attr_handle);

		if (event->subscribe.attr_handle == HANDLE_HID_REPORT) {
			HID_REPORT_NOTIFY_STATE = event->subscribe.cur_notify;

			ESP_LOGI(TAG, "HID Report Notify State: %s",
					 event->subscribe.cur_notify ? "ENABLED" : "DISABLED");

			ESP_LOGI(TAG,
					 "conn_handle=%d attr_handle=%04X "
					 "reason=%d prev_notify=%d cur_notify=%d prev_indicate=%d "
					 "cur_indicate=%d",
					 event->subscribe.conn_handle, event->subscribe.attr_handle,
					 event->subscribe.reason, event->subscribe.prev_notify,
					 event->subscribe.cur_notify,
					 event->subscribe.prev_indicate,
					 event->subscribe.cur_indicate);
		}
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_NOTIFY_TX: // DUNNO
		ESP_LOGI(
			TAG,
			"notify event; status=%d conn_handle=%d attr_handle=%04X type=%s",
			event->notify_tx.status, event->notify_tx.conn_handle,
			event->notify_tx.attr_handle,
			event->notify_tx.indication ? "indicate" : "notify");
		return 0;
	/********************************************************************/
	case BLE_GAP_EVENT_DATA_LEN_CHG: // CHANGING MAX DATA LEN
		/*if (event->data_len_chg.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
			rc = ble_gap_set_data_len(event->data_len_chg.conn_handle,
									  event->data_len_chg.max_tx_octets,
									  event->data_len_chg.max_tx_time);*/
		rc = 0;
		if (rc == 0) {
			ESP_LOGI(TAG,
					 "Changed Data Length of Connection (%d): max_tx_len=%d , "
					 "Max_tx_time=%d",
					 event->data_len_chg.conn_handle,
					 event->data_len_chg.max_tx_octets,
					 event->data_len_chg.max_tx_time);
		} else {
			ESP_LOGW(TAG,
					 "Could not change Data Length of Connection (%d): "
					 "max_tx_len=%d , "
					 "Max_tx_time=%d",
					 event->data_len_chg.conn_handle,
					 event->data_len_chg.max_tx_octets,
					 event->data_len_chg.max_tx_time);
		}

		return 0;
	/********************************************************************/
	/*case BLE_GAP_EVENT_PASSKEY_ACTION:
		ESP_LOGI(TAG, "Passkey event");
		struct ble_sm_io pkey = {0};
		if (event->passkey.params.action == BLE_SM_IOACT_NONE) {
			pkey.action = event->passkey.params.action;
			pkey.passkey = ble_passkey;
			ESP_LOGI(TAG, "Enter passkey %d on the peer side",
					 (int)pkey.passkey);
			rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
			ESP_LOGI(TAG, "ble_sm_inject_io result: %d\n", rc);
		}
		return 0;
*/ default:
		ESP_LOGW(TAG, "Unhandled GAP event: %d", event->type);
		return 0;
	}
}
