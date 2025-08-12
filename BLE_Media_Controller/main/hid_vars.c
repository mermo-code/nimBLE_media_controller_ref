/*
 * hid_vars.c
 *
 *  Created on: 10 Aug 2025
 *      Author: gsabi
 */

#include "hid_vars.h"

uint16_t HANDLE_HID_REPORT;

/****************HID VARIABLES*****************/
uint8_t hid_input_report;

const uint8_t report_reference[] = {0x00,
									0x01}; // Report ID 0, Type = Input (0x01)

const uint8_t hid_info[] = {
	0x01, 0x00, // Version v1.0
	0x00,		// Country code (not localised)
	0x03		// Flags (1-wakeup,2-normally connectable,3-both)
};

const uint8_t hid_descriptor_report[] = {
	0x05, 0x0C, // Usage Page (Consumer Page)
	0x09, 0x01, // Usage (Consumer Control)
	0xA1, 0x01, // Collection (Application)
	
	0x09, 0xE9, // Usage (Volume Increment)
	0x09, 0xEA, // Usage (Volume Decrement)
	0x15, 0x00, // Logical Minimum (0)
	0x25, 0x01, // Logical Maximum (1)
	0x75, 0x01, // Report Size (1)
	0x95, 0x02, // Report Count (2)
	0x81, 0x02, // Input (Data, Variable, Absolute)
	
	0x09, 0xCD,       //   Usage (Play/Pause)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x04,       //   Input (Data, Variable, Absolute)
	
	0x75, 0x05, // Report Size () - Padding
	0x95, 0x01, // Report Count (1) - Padding
	0x81, 0x01, // Input (Constant, Array, Absolute)
	0xC0		// Collection End
};

/*const uint8_t hid_descriptor_report[] = {
    0x05, 0x0C,       // Usage Page (Consumer Page)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)

    0x09, 0xCD,       //   Usage (Play/Pause)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x04,       //   Input (Data, Variable, Absolute)

    0x75, 0x07,       //   Report Size (7) — padding
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x01,       //   Input (Constant, Array, Absolute)

    0xC0              // End Collection
};*/

// uint16_t LEN_HID_DESCRIPTOR_REPORT = sizeof(hid_descriptor_report);