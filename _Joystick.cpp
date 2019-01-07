/*
  Joystick.cpp

  Copyright (c) 2015-2017, Matthew Heironimus

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "_Joystick.h"

#if defined(_USING_DYNAMIC_HID)

#define JOYSTICK_REPORT_ID_INDEX 7
#define JOYSTICK_AXIS_MINIMUM -32767
#define JOYSTICK_AXIS_MAXIMUM 32767
#define JOYSTICK_SIMULATOR_MINIMUM -32767
#define JOYSTICK_SIMULATOR_MAXIMUM 32767

#define JOYSTICK_INCLUDE_X_AXIS  B00000001
#define JOYSTICK_INCLUDE_Y_AXIS  B00000010
#define JOYSTICK_INCLUDE_Z_AXIS  B00000100
#define JOYSTICK_INCLUDE_RX_AXIS B00001000
#define JOYSTICK_INCLUDE_RY_AXIS B00010000
#define JOYSTICK_INCLUDE_RZ_AXIS B00100000

#define JOYSTICK_INCLUDE_RUDDER      B00000001
#define JOYSTICK_INCLUDE_THROTTLE    B00000010
#define JOYSTICK_INCLUDE_ACCELERATOR B00000100
#define JOYSTICK_INCLUDE_BRAKE       B00001000
#define JOYSTICK_INCLUDE_STEERING    B00010000

Joystick_::Joystick_(
	uint8_t hidReportId,
	uint8_t joystickType)
{
   // Set the USB HID Report ID
  _hidReportId = hidReportId;
	
  // Build Joystick HID Report Description
	
  // Button Calculations
  uint8_t buttonPaddingBits = 0;
    	
	// Axis Calculations
	uint8_t axisCount = 3;
    		
  uint8_t tempHidReportDescriptor[150];
  int hidReportDescriptorSize = 0;

  // USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (Joystick - 0x04; Gamepad - 0x05; Multi-axis Controller - 0x08)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = joystickType;

  // COLLECTION (Application)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_ID (Default: 3)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
  tempHidReportDescriptor[hidReportDescriptorSize++] = _hidReportId;
	
	// USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;		
	
	// USAGE (Pointer)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

	// LOGICAL_MINIMUM (-32767)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x16;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x80;

	// LOGICAL_MAXIMUM (+32767)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7F;

	// REPORT_SIZE (16)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

	// REPORT_COUNT (axisCount)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
	tempHidReportDescriptor[hidReportDescriptorSize++] = axisCount;
					
	// COLLECTION (Physical)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
	
  // USAGE (Rx)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x33;

  // USAGE (Ry)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x34;
	
	// USAGE (Rz)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
	
	// INPUT (Data,Var,Abs)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
	
	// END_COLLECTION (Physical)
	tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
	
  // END_COLLECTION
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

	// Create a copy of the HID Report Descriptor template that is just the right size
	uint8_t *customHidReportDescriptor = new uint8_t[hidReportDescriptorSize];
	memcpy(customHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);
	
	// Register HID Report Description
	DynamicHIDSubDescriptor *node = new DynamicHIDSubDescriptor(customHidReportDescriptor, hidReportDescriptorSize, false);
	DynamicHID().AppendDescriptor(node);
	
	// Calculate HID Report Size
	_hidReportSize = 0;
	_hidReportSize += (axisCount * 2);
	
	// Initalize Joystick State
	_xAxisRotation = 0;
	_yAxisRotation = 0;
	_zAxisRotation = 0;
}

void Joystick_::begin(bool initAutoSendState)
{
	_autoSendState = initAutoSendState;
	sendState();
}

void Joystick_::end()
{
}

void Joystick_::setRxAxis(int16_t value)
{
	  _xAxisRotation = value;
	  if (_autoSendState) sendState();
}

void Joystick_::setRyAxis(int16_t value)
{
	_yAxisRotation = value;
	if (_autoSendState) sendState();
}

void Joystick_::setRzAxis(int16_t value)
{
	_zAxisRotation = value;
	if (_autoSendState) sendState();
}

int Joystick_::buildAndSet16BitValue(bool includeValue, int16_t value, int16_t valueMinimum, int16_t valueMaximum, int16_t actualMinimum, int16_t actualMaximum, uint8_t dataLocation[]) 
{
	int16_t convertedValue;
	uint8_t highByte;
	uint8_t lowByte;
	int16_t realMinimum = min(valueMinimum, valueMaximum);
	int16_t realMaximum = max(valueMinimum, valueMaximum);

	if (includeValue == false) return 0;

	if (value < realMinimum) {
		value = realMinimum;
	}
	if (value > realMaximum) {
		value = realMaximum;
	}

	if (valueMinimum > valueMaximum) {
		// Values go from a larger number to a smaller number (e.g. 1024 to 0)
		value = realMaximum - value + realMinimum;
	}

	convertedValue = map(value, realMinimum, realMaximum, actualMinimum, actualMaximum);

	highByte = (uint8_t)(convertedValue >> 8);
	lowByte = (uint8_t)(convertedValue & 0x00FF);
	
	dataLocation[0] = lowByte;
	dataLocation[1] = highByte;
	
	return 2;
}

int Joystick_::buildAndSetAxisValue(bool includeAxis, int16_t axisValue, int16_t axisMinimum, int16_t axisMaximum, uint8_t dataLocation[]) 
{
	return buildAndSet16BitValue(includeAxis, axisValue, axisMinimum, axisMaximum, JOYSTICK_AXIS_MINIMUM, JOYSTICK_AXIS_MAXIMUM, dataLocation);
}

void Joystick_::sendState()
{
	uint8_t data[_hidReportSize];
	int index = 0;

	// Set Axis Values
	index += buildAndSetAxisValue(true, _xAxisRotation, _rxAxisMinimum, _rxAxisMaximum, &(data[index]));
	index += buildAndSetAxisValue(true, _yAxisRotation, _ryAxisMinimum, _ryAxisMaximum, &(data[index]));
	index += buildAndSetAxisValue(true, _zAxisRotation, _rzAxisMinimum, _rzAxisMaximum, &(data[index]));
	
	DynamicHID().SendReport(_hidReportId, data, _hidReportSize);
}

#endif
