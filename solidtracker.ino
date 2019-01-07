#include <MPU9250.h>
#include "_Joystick.h"
#include "MadgwickAHRS.h"

// Create the Joystick
static Joystick_ _joystick(
  JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK
);

static MPU9250 _imu(Wire,0x68);

static volatile bool _dataAvailable = false;

static void readIMU()
{ 
  _dataAvailable = true;
}

Madgwick _filter;
const int BUTTON_PIN = 10;

void setup() {

  // serial to display data
  Serial.begin(115200);
  Serial.println("Setup...");

  Wire.setClock(400000);
  
  // Initialize Button Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize Joystick Library
  _joystick.begin(false);
  _joystick.setRxAxisRange(-32767, 32767);
  _joystick.setRyAxisRange(-32767, 32767);
  _joystick.setRzAxisRange(-32767, 32767);
  
  // start communication with IMU 
  int status = _imu.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while(1) {};
  }

  // Low Pass Filter - 20 Hz
  _imu.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);

  // 200 Hz update rate (srd = 4)
  _imu.setSrd(4);
  
  // enabling the data ready interrupt
  _imu.enableDataReadyInterrupt();
  
  // attaching the interrupt to microcontroller pin 1
  pinMode(7,INPUT);
  attachInterrupt(digitalPinToInterrupt(7), readIMU, RISING);

  _filter.begin(200);
}

unsigned long _time = 0;
float ax, ay, az, gx, gy, gz, mx, my, mz;

void loop() {

  if (_dataAvailable) {

    _imu.readSensor();

    ax = _imu.getAccelX_mss();
    ay = _imu.getAccelY_mss();
    az = _imu.getAccelZ_mss();

    gx = _imu.getGyroX_rads();
    gy = _imu.getGyroY_rads();
    gz = _imu.getGyroZ_rads();

    mx = _imu.getMagX_uT();
    my = _imu.getMagY_uT();
    mz = _imu.getMagZ_uT();

    _filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);
     
    _dataAvailable = false;
  }

  unsigned long now = millis();
  if (now - _time > 10) {
    
    /*Serial.print(_filter.getRoll(), 6);
    Serial.print(" ");
    Serial.print(_filter.getPitch(), 6);
    Serial.print(" ");
    Serial.println(_filter.getYaw(), 6);*/

    _joystick.setRxAxis(((_filter.getRoll() - 180) / 180) * 32767);    
    _joystick.setRyAxis((_filter.getPitch() / 90) * 32767);
    _joystick.setRzAxis(((_filter.getYaw() / 360) * 65536) - 32767);
    _joystick.sendState();

    _time = now;
  }
}
