#include "esphome.h"

class JuraCoffee : public PollingComponent, public UARTDevice {
 BinarySensor *xsensor1 {nullptr};
 BinarySensor *xsensor2 {nullptr};
 BinarySensor *xsensor3 {nullptr};
 BinarySensor *xsensor4 {nullptr};
 TextSensor *xsensor {nullptr};

 public:
  JuraCoffee(UARTComponent *parent, BinarySensor *sensor1, BinarySensor *sensor2, BinarySensor *sensor3, BinarySensor *sensor4, TextSensor *sensor) : UARTDevice(parent),  xsensor1(sensor1), xsensor2(sensor2), xsensor3(sensor3), xsensor4(sensor4), xsensor(sensor) {}

  boolean power_state, tray_state, tank_state, bean_state;
  std::string status;

  // Jura communication function taken in entirety from cmd2jura.ino, found at https://github.com/hn/jura-coffee-machine
  String cmd2jura(String outbytes) {
    String inbytes;
    int w = 0;

    while (available()) {
      read();
    }

    outbytes += "\r\n";
    for (int i = 0; i < outbytes.length(); i++) {
      for (int s = 0; s < 8; s += 2) {
        char rawbyte = 255;
        bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
        bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
        write(rawbyte);
      }
      delay(8);
    }

    int s = 0;
    char inbyte;
    while (!inbytes.endsWith("\r\n")) {
      if (available()) {
        byte rawbyte = read();
        bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
        bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
        if ((s += 2) >= 8) {
          s = 0;
          inbytes += inbyte;
        }
      } else {
        delay(10);
      }
      if (w++ > 500) {
        return "";
      }
    }

    return inbytes.substring(0, inbytes.length() - 2);
  }

  void setup() override {
    this->set_update_interval(10000); // 600000 = 10 minutes // Now 10 seconds
  }

  void loop() override {
  }

  void update() override {
    String result, state, power;
    //byte hex_to_byte;
    boolean tray=true, tank=true, beans=true, pwr=true;
    

    // Tray & water tank status
    // Much gratitude to https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/ for figuring out how these bits are stored
    // and https://knx-user-forum.de/forum/projektforen/edomi/1010532-kaffeeautomaten-jura-bremer-aehnliche
    
    result = cmd2jura("RR:03");
    
    power = result.substring(4,5);
    state = result.substring(6,7)+result.substring(25,28);
    
    if (power == "0") { 
      status = "OFF";
      pwr = false;
      } 
      else if (state=="C404") {
        status = "Fill water";
        tank = false;
        }
      else if (state=="C045") {
        status = "Tray missing";
        tray = false;
        }
      else if (state=="4805") {
        status = "Fill beans";
        beans = false;
        }
      else if (state=="4205") {
        status = "Empty grounds";
        }
      else if (state=="C105") {
        status = "Empty tray";
        }
      else if (state == "C444") {
        tray = false;
        tank = false;
        }
      else if (state=="4005") {
        status = "Ready";
        pwr = true;
        tray = true;
        tank = true;
        beans = true;
      }
      else {
        ESP_LOGD("main", "Raw IC result: %s", result.c_str());
        ESP_LOGD("main", "Power: %s", power.c_str());
        ESP_LOGD("main", "State: %s", state.c_str());
        };
      
    if (xsensor != nullptr)   xsensor->publish_state(status);
    if (xsensor1 != nullptr)  xsensor1->publish_state(tray);
    if (xsensor2 != nullptr)  xsensor2->publish_state(tank);
    if (xsensor3 != nullptr)  xsensor3->publish_state(beans);
    if (xsensor4 != nullptr)  xsensor4->publish_state(pwr);

  }
};