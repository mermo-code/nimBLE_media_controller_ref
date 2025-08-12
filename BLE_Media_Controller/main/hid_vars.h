/*
 * hid_vars.h
 *
 *  Created on: 10 Aug 2025
 *      Author: gsabi
 */

#ifndef MAIN_HID_VARS_H_
#define MAIN_HID_VARS_H_

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/*****************HID DATA*****************/
extern uint8_t hid_input_report;
extern const uint8_t report_reference[];
extern const uint8_t hid_info[];
extern const uint8_t hid_descriptor_report[];

/*************HID DATA LEN************/
#define LEN_HID_REPORT_REFERENCE 2
#define LEN_HID_INFO 4
#define LEN_HID_DESCRIPTOR_REPORT 39

/***************UUID VARIABLES***************/
#define GATT_UUID_HID_SERVICE 0x1812

#define GATT_UUID_HID_INFORMATION 0x2A4A
#define GATT_UUID_HID_REPORT_MAP 0x2A4B
#define GATT_UUID_HID_CONTROL_POINT 0x2A4C
#define GATT_UUID_HID_INPUT_REPORT 0x2A4D

#define GATT_UUID_HID_RPT_REF_DESCR 0x2908

extern uint16_t HANDLE_HID_REPORT;

#endif /* MAIN_HID_VARS_H_ */
