#include <EEPROM.h>
#include <ESPUI.h>
#include <PowerFeather.h>

#include <driver/rtc_io.h>

#include <WiFi.h>
#include <DNSServer.h>

// Settings
#define BATTERY_CAPACITY 3000
#define HOSTNAME "powerfeather"
#define SSID "PowerFeather_Demo"
#define TITLE "ESP32-S3 PowerFeather Demo"

using namespace PowerFeather;

const byte DNS_PORT = 53;
DNSServer dnsServer;

IPAddress apIP(192, 168, 1, 1);
IPAddress netMsk(255, 255, 255, 0);

const String disabledStyle = "background-color: #bbb; border-bottom: #999 3px solid;";

char supplyVoltageString[32];
char supplyCurrentString[32];

char batteryVoltageString[32];
char batteryCurrentString[32];
char batteryChargeString[32];
char batteryHealthString[32];
char batteryCyclesString[32];
char batteryTimeLeftString[32];
char batteryTemperatureString[32];

uint16_t supplyVoltageLabel;
uint16_t supplyCurrentLabel;

uint16_t batteryVoltageLabel;
uint16_t batteryCurrentLabel;
uint16_t batteryChargeLabel;
uint16_t batteryHealthLabel;
uint16_t batteryCyclesLabel;
uint16_t batteryTimeLeftLabel;

uint16_t batteryChargingEnableSwitcher;
uint16_t batteryChargingMaxCurrentSlider;

uint16_t batteryTemperatureEnableSwitcher;
uint16_t batteryTemperatureLabel;

uint16_t output3V3HeaderSwitcher;
uint16_t output3V3SQTSwitcher;

uint16_t mppVoltageSlider;

// Most elements in this test UI are assigned this generic callback which prints some
// basic information. Event types are defined in ESPUI.h
void generalCallback(Control *sender, int type)
{
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
}

void setMaxChargingCurrentSlider(bool enable)
{
  ESPUI.setEnabled(batteryChargingMaxCurrentSlider, enable);
  ESPUI.setPanelStyle(batteryChargingMaxCurrentSlider, enable ? ";" : disabledStyle);
}

void setBatteryTempLabel(bool enable)
{
  ESPUI.setEnabled(batteryTemperatureLabel, enable);
  ESPUI.setPanelStyle(batteryTemperatureLabel, enable ? ";" : disabledStyle);
}

void setMppVoltageSlider(bool enable)
{
  ESPUI.setEnabled(mppVoltageSlider, enable);
  ESPUI.setPanelStyle(mppVoltageSlider, enable ? ";" : disabledStyle);
}

void chargingEnableCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.enableBatteryCharging(atoi(sender->value.c_str()));
  setMaxChargingCurrentSlider(atoi(sender->value.c_str()));
}

void tempSenseEnableCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  setBatteryTempLabel(atoi(sender->value.c_str()));
  Board.enableBatteryTempSense(atoi(sender->value.c_str()));
}

void chargingMaxCurrentCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.setBatteryChargingMaxCurrent(atoi(sender->value.c_str()));
}

void mppVoltageCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.setSupplyMaintainVoltage(atoi(sender->value.c_str()));
}

void enterShipModeCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.enterShipMode();
}

void enterDeepSleepCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  esp_sleep_enable_timer_wakeup(5 * 1000000);
  esp_deep_sleep_start();
}

void output3V3HeaderCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.enable3V3(atoi(sender->value.c_str()));
}

void output3V3SQTCallback(Control *sender, int type)
{
  generalCallback(sender, type);
  Board.enableVSQT(atoi(sender->value.c_str()));
}

// This is the main function which builds our GUI
void setUpUI()
{
  // Turn off verbose debugging
  ESPUI.setVerbosity(Verbosity::Quiet);

  // Make sliders continually report their position as they are being dragged.
  ESPUI.sliderContinuous = false;

  String clearLabelStyle = "background-color: unset; width: 100%;";
  String largeLabelStyle = "width: 48%; .3rem; margin-right: .3rem; font-size: 35px";
  String smallLabelStyle = "background-color: unset; width: 48%;";

  auto batteryInfoGroup = batteryVoltageLabel = ESPUI.addControl(Label, "Battery Information", batteryVoltageString, Dark);
  batteryCurrentLabel = ESPUI.addControl(Label, "", batteryCurrentString, Dark, batteryInfoGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Voltage", Dark, batteryInfoGroup), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Current", Dark, batteryInfoGroup), smallLabelStyle);

  batteryChargeLabel = ESPUI.addControl(Label, "", batteryChargeString, Dark, batteryInfoGroup);
  batteryTimeLeftLabel = ESPUI.addControl(Label, "", batteryTimeLeftString, Dark, batteryInfoGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Charge", Dark, batteryInfoGroup), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Time Left", Dark, batteryInfoGroup), smallLabelStyle);

  batteryHealthLabel = ESPUI.addControl(Label, "", batteryHealthString, Dark, batteryInfoGroup);
  batteryCyclesLabel = ESPUI.addControl(Label, "", batteryCyclesString, Dark, batteryInfoGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Health", Dark, batteryInfoGroup), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Cycles", Dark, batteryInfoGroup), smallLabelStyle);

  ESPUI.setElementStyle(batteryVoltageLabel, largeLabelStyle);
  ESPUI.setElementStyle(batteryCurrentLabel, largeLabelStyle);
  ESPUI.setElementStyle(batteryChargeLabel, largeLabelStyle);
  ESPUI.setElementStyle(batteryTimeLeftLabel, largeLabelStyle);
  ESPUI.setElementStyle(batteryHealthLabel, largeLabelStyle);
  ESPUI.setElementStyle(batteryCyclesLabel, largeLabelStyle);

  auto supplyInfoGroup = supplyVoltageLabel = ESPUI.addControl(Label, "Supply Information", supplyVoltageString, Dark);
  supplyCurrentLabel = ESPUI.addControl(Label, "", supplyCurrentString, Dark, supplyInfoGroup);

  ESPUI.setElementStyle(supplyVoltageLabel, largeLabelStyle);
  ESPUI.setElementStyle(supplyCurrentLabel, largeLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Voltage", Dark, supplyInfoGroup), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Current", Dark, supplyInfoGroup), smallLabelStyle);

  auto chargingControlGroup = batteryChargingEnableSwitcher = ESPUI.addControl(Switcher, "Charging", "0", Dark, Control::noParent, chargingEnableCallback);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Enable", None, chargingControlGroup), clearLabelStyle);
  batteryChargingMaxCurrentSlider = ESPUI.addControl(Slider, "", "50", Dark, chargingControlGroup, chargingMaxCurrentCallback);
  ESPUI.addControl(Min, "", "50", None, batteryChargingMaxCurrentSlider);
  ESPUI.addControl(Max, "", "2000", None, batteryChargingMaxCurrentSlider);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Max Charging Current (mA)", None, chargingControlGroup), clearLabelStyle);

  auto batteryTempControlGroup = batteryTemperatureEnableSwitcher = ESPUI.addControl(Switcher, "Battery Temperature Sense", "", Dark, Control::noParent, tempSenseEnableCallback);
  batteryTemperatureLabel = ESPUI.addControl(Label, "", batteryTemperatureString, Dark, batteryTempControlGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Enable", None, batteryTempControlGroup), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Temperature", None, batteryTempControlGroup), smallLabelStyle);
  ESPUI.setElementStyle(batteryTemperatureEnableSwitcher, "margin-left: 14%; margin-right: 0%");
  ESPUI.setElementStyle(batteryTemperatureLabel, "width: 40%; margin-left: 21%; font-size: 30px");

  auto output3V3Group = output3V3HeaderSwitcher = ESPUI.addControl(Switcher, "3.3 V Outputs", "", Dark, Control::noParent, output3V3HeaderCallback);
  output3V3SQTSwitcher = ESPUI.addControl(Switcher, "", "", Dark, output3V3Group, output3V3SQTCallback);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", Dark, output3V3Group), clearLabelStyle);

  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "3V3 Enable", None, output3V3Group), smallLabelStyle);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "STEMMA QT Enable", None, output3V3Group), smallLabelStyle);
  ESPUI.setElementStyle(output3V3HeaderSwitcher, "margin-right: 20%");
  ESPUI.setElementStyle(output3V3SQTSwitcher, "margin-left: 17%");

  mppVoltageSlider = ESPUI.addControl(Slider, "Max Power Point", "4600", Dark, Control::noParent, mppVoltageCallback);
  ESPUI.addControl(Min, "", "4600", None, mppVoltageSlider);
  ESPUI.addControl(Max, "", "16800", None, mppVoltageSlider);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Voltage (mV)", None, mppVoltageSlider), clearLabelStyle);

  auto powerStatesGroup = ESPUI.addControl(Button, "Power States", "Ship Mode", Dark, Control::noParent, enterShipModeCallback);
  ESPUI.addControl(Button, "", "Deep Sleep", Dark, powerStatesGroup, enterDeepSleepCallback);

  bool header3V3Enabled = rtc_gpio_get_level(static_cast<gpio_num_t>(4));
  ESPUI.updateSwitcher(output3V3HeaderSwitcher, header3V3Enabled);

  bool sqt3V3Enabled = rtc_gpio_get_level(static_cast<gpio_num_t>(14));
  ESPUI.updateSwitcher(output3V3SQTSwitcher, sqt3V3Enabled);

  setMaxChargingCurrentSlider(false);
  setBatteryTempLabel(false);

  // Finally, start up the UI.
  // This should only be called once we are connected to WiFi.
  ESPUI.begin(TITLE);
}

void startWifi()
{
  Serial.print("\nCreating access point...");
  WiFi.mode(WIFI_AP);
  WiFi.setHostname(HOSTNAME);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(SSID);
  Serial.println("done!");
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Board.init(BATTERY_CAPACITY);
  Board.enableBatteryCharging(false);
  Board.setBatteryChargingMaxCurrent(50);
  Board.setSupplyMaintainVoltage(4600);
  Board.enableBatteryTempSense(false);

  startWifi();
  WiFi.setSleep(false);

  delay(1000);

  setUpUI();

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
}

void loop()
{
  static long unsigned lastTime = 0;

  // Send periodic updates if switcher is turned on
  if (millis() > (lastTime + 100))
  {
    const char *voltageFormat = "%.2f V";
    const char *currentFormat = "%.2f A";

    uint16_t value1 = 0;
    int16_t value2 = 0;
    uint8_t value3 = 0;
    int32_t value4 = 0;
    float value5 = 0.0f;

    value1 = 0;
    Board.getSupplyVoltage(value1);
    memset(supplyVoltageString, 0, sizeof(supplyVoltageString));
    sprintf(supplyVoltageString, voltageFormat, value1 / 1000.0f);

    value2 = 0;
    Board.getSupplyCurrent(value2);
    memset(supplyCurrentString, 0, sizeof(supplyVoltageString));
    sprintf(supplyCurrentString, currentFormat, value2 / 1000.0f);

    value1 = 0;
    Board.getBatteryVoltage(value1);
    memset(batteryVoltageString, 0, sizeof(supplyVoltageString));
    sprintf(batteryVoltageString, voltageFormat, value1 / 1000.0f);

    value2 = 0;
    Board.getBatteryCurrent(value2);
    memset(batteryCurrentString, 0, sizeof(supplyVoltageString));
    sprintf(batteryCurrentString, currentFormat, value2 / 1000.0f);

    value3 = 0;
    Board.getBatteryCharge(value3);
    memset(batteryChargeString, 0, sizeof(batteryChargeString));
    sprintf(batteryChargeString, "%d %%", value3);

    value3 = 0;
    Board.getBatteryHealth(value3);
    memset(batteryHealthString, 0, sizeof(batteryHealthString));
    sprintf(batteryHealthString, "%d %%", value3);

    value1 = 0;
    Board.getBatteryCycles(value1);
    memset(batteryCyclesString, 0, sizeof(batteryCyclesString));
    sprintf(batteryCyclesString, "%d", value1);

    value4 = 0;
    Result res = Board.getBatteryTimeLeft(value4);
    memset(batteryTimeLeftString, 0, sizeof(batteryTimeLeftString));
    if (res == Result::Ok)
    {
      sprintf(batteryTimeLeftString, "%.2f h", value4 / 60.0f);
    }
    else
    {
      memcpy(batteryTimeLeftString, "...", sizeof(batteryTimeLeftString));
    }

    value5 = 0.0f;
    Board.getBatteryTemperature(value5);
    memset(batteryTemperatureString, 0, sizeof(batteryTemperatureString));
    sprintf(batteryTemperatureString, "%.2f C", value5);

    ESPUI.updateLabel(supplyVoltageLabel, supplyVoltageString);
    ESPUI.updateLabel(supplyCurrentLabel, supplyCurrentString);
    ESPUI.updateLabel(batteryVoltageLabel, batteryVoltageString);
    ESPUI.updateLabel(batteryCurrentLabel, batteryCurrentString);
    ESPUI.updateLabel(batteryChargeLabel, batteryChargeString);
    ESPUI.updateLabel(batteryHealthLabel, batteryHealthString);
    ESPUI.updateLabel(batteryCyclesLabel, batteryCyclesString);
    ESPUI.updateLabel(batteryTimeLeftLabel, batteryTimeLeftString);
    ESPUI.updateLabel(batteryTemperatureLabel, batteryTemperatureString);

    lastTime = millis();
  }

  dnsServer.processNextRequest();
}


