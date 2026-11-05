#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// LoRa PINS
// =====================================================

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

// =====================================================
// BUTTON PINS
// =====================================================

#define BUTTON_FORWARD   32
#define BUTTON_BACKWARD  33
#define BUTTON_LEFT      25
#define BUTTON_RIGHT     27

// =====================================================
// OLED PINS
// =====================================================

#define OLED_SDA  21
#define OLED_SCL  22

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 13

// =====================================================
// GAS ALERT
// =====================================================

#define GAS_ALERT_THRESHOLD 500.0
#define MQ9_MAX_PPM 1000.0

#define BUZZER_ON_TIME   300
#define BUZZER_OFF_TIME 1200

// =====================================================
// GPS
// =====================================================

#define GPS_RX_PIN 34
#define GPS_TX_PIN 15
#define GPS_BAUD 9600

// =====================================================
// MQ-9
// =====================================================

#define MQ9_PIN 35
#define MQ9_RL 10000.0
#define MQ9_CALIBRATION_TIME 30000UL

float mq9_R0 = 10.0;

// =====================================================
// OLED OBJECT
// =====================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE,
  OLED_SCL,
  OLED_SDA
);

// =====================================================
// GPS OBJECT
// =====================================================

HardwareSerial GPS(2);
TinyGPSPlus gps;

// =====================================================
// MQ-6 GAS DATA
// =====================================================

float gasPPM = 0.0;
bool gasReceived = false;
unsigned long lastGasReceived = 0;

// =====================================================
// MQ-9 DATA
// =====================================================

int mq9Raw = 0;
float mq9ADCVoltage = 0.0;
float mq9SensorVoltage = 0.0;
float mq9Rs = 0.0;
float mq9Ratio = 0.0;
float mq9PPM = 0.0;

// =====================================================
// GAS ALERT STATES
// =====================================================

bool mq6Alert = false;
bool mq9Alert = false;
bool bothGasAlert = false;

// =====================================================
// GPS DATA
// =====================================================

double gpsLatitude = 0.0;
double gpsLongitude = 0.0;
bool gpsFix = false;

unsigned long lastGPSPrint = 0;
unsigned long lastAlertMessage = 0;

// =====================================================
// BUZZER STATE
// =====================================================

bool buzzerState = false;
unsigned long buzzerTimer = 0;

// =====================================================
// WEB DASHBOARD
// =====================================================

// ENTER YOUR ROUTER WIFI DETAILS HERE
const char* WIFI_SSID = "HACKATHON";
const char* WIFI_PASSWORD = "Hacker@786";

WebServer server(80);

// =====================================================
// DASHBOARD HTML
// =====================================================

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Coal Mine Safety Dashboard</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, Helvetica, sans-serif;
  background: #101820;
  color: white;
}

.header {
  background: #182631;
  padding: 20px;
  text-align: center;
  border-bottom: 2px solid #344955;
}

.header h1 {
  margin: 0;
  font-size: 26px;
}

.header p {
  margin: 8px 0 0;
  color: #b9c7d0;
}

.container {
  max-width: 1200px;
  margin: auto;
  padding: 20px;
}

.gas-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
}

.card {
  background: #182631;
  border-radius: 14px;
  padding: 20px;
  box-shadow: 0 4px 15px rgba(0,0,0,0.35);
}

.card h2 {
  margin-top: 0;
}

.value {
  font-size: 42px;
  font-weight: bold;
  margin: 10px 0;
}

.unit {
  font-size: 18px;
  color: #b9c7d0;
}

.info {
  display: flex;
  justify-content: space-between;
  margin: 12px 0;
  color: #cbd5db;
}

.status {
  padding: 10px;
  border-radius: 8px;
  text-align: center;
  font-weight: bold;
  margin-top: 12px;
}

.normal {
  background: #174d32;
  color: #7ff0ae;
}

.high {
  background: #653f16;
  color: #ffd37a;
}

canvas {
  width: 100%;
  height: 220px;
  background: #0c141a;
  border-radius: 10px;
  margin-top: 15px;
}

.safety {
  margin-top: 20px;
  text-align: center;
  padding: 22px;
  border-radius: 14px;
  background: #174d32;
}

.safety h2 {
  margin: 0 0 10px;
}

.safety.safe {
  background: #174d32;
  color: #7ff0ae;
}

.safety.warning {
  background: #653f16;
  color: #ffd37a;
}

.safety.emergency {
  background: #6d1111;
  color: #ffb0b0;
  animation: emergencyFlash 0.8s infinite alternate;
}

@keyframes emergencyFlash {
  from {
    box-shadow: 0 0 5px rgba(255,0,0,0.2);
  }
  to {
    box-shadow: 0 0 30px rgba(255,0,0,0.8);
  }
}

.emergency-box {
  display: none;
  margin-top: 20px;
  background: #780d0d;
  border: 3px solid #ff4040;
  border-radius: 14px;
  padding: 25px;
  text-align: center;
  animation: emergencyFlash 0.8s infinite alternate;
}

.emergency-box h1 {
  margin-top: 0;
  font-size: 30px;
}

.emergency-main {
  font-size: 24px;
  font-weight: bold;
  margin: 15px 0;
}

.danger {
  font-size: 22px;
  font-weight: bold;
  margin-top: 15px;
}

.move-out {
  font-size: 30px;
  font-weight: bold;
  margin-top: 15px;
}

.footer {
  text-align: center;
  padding: 20px;
  color: #8798a3;
}

@media(max-width: 800px) {

  .gas-grid {
    grid-template-columns: 1fr;
  }

  .header h1 {
    font-size: 21px;
  }

  .value {
    font-size: 36px;
  }

}

</style>

</head>

<body>

<div class="header">

  <h1>⛏ COAL MINE SAFETY DASHBOARD</h1>

  <p>ESP32 Live Gas Monitoring System</p>

</div>

<div class="container">

  <div class="gas-grid">

    <!-- ================= MQ-6 ================= -->

    <div class="card">

      <h2>MQ-6 GAS LEVEL</h2>

      <div class="value">
        <span id="mq6Value">0</span>
        <span class="unit">PPM</span>
      </div>

      <div class="info">
        <span>Threshold</span>
        <b>500 PPM</b>
      </div>

      <div class="info">
        <span>Maximum</span>
        <b>1000 PPM</b>
      </div>

      <div id="mq6Status" class="status normal">
        NORMAL
      </div>

      <canvas id="mq6Graph"></canvas>

    </div>

    <!-- ================= MQ-9 ================= -->

    <div class="card">

      <h2>MQ-9 GAS LEVEL</h2>

      <div class="value">
        <span id="mq9Value">0</span>
        <span class="unit">PPM</span>
      </div>

      <div class="info">
        <span>Threshold</span>
        <b>500 PPM</b>
      </div>

      <div class="info">
        <span>Maximum</span>
        <b>1000 PPM</b>
      </div>

      <div id="mq9Status" class="status normal">
        NORMAL
      </div>

      <canvas id="mq9Graph"></canvas>

    </div>

  </div>

  <!-- ================= SAFETY STATUS ================= -->

  <div id="safetyBox" class="safety safe">

    <h2>SAFETY STATUS</h2>

    <div id="safetyText">
      SAFE
    </div>

  </div>

  <!-- ================= EMERGENCY ALERT ================= -->

  <div id="emergencyBox" class="emergency-box">

    <h1>🚨 EMERGENCY ALERT 🚨</h1>

    <div class="emergency-main">
      MQ-9 AND MQ-6 HIGH
    </div>

    <div>
      MQ-6:
      <b><span id="emergencyMQ6">0</span> PPM</b>
    </div>

    <div>
      MQ-9:
      <b><span id="emergencyMQ9">0</span> PPM</b>
    </div>

    <div class="danger">
      ⚠️ DANGEROUS LEVEL
    </div>

    <div class="move-out">
      MOVE OUT IMMEDIATELY
    </div>

  </div>

</div>

<div class="footer">
  COAL MINE SAFETY SYSTEM
</div>

<script>

const THRESHOLD = 500;
const MAX_PPM = 1000;

const MAX_POINTS = 40;

let mq6History = [];
let mq9History = [];

// =====================================================
// CANVAS GRAPH
// =====================================================

function drawGraph(canvasId, data, label)
{

  const canvas = document.getElementById(canvasId);

  const ctx = canvas.getContext("2d");

  const width = canvas.clientWidth;
  const height = canvas.clientHeight;

  const ratio = window.devicePixelRatio || 1;

  canvas.width = width * ratio;
  canvas.height = height * ratio;

  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);

  ctx.clearRect(0, 0, width, height);

  // Background

  ctx.fillStyle = "#0c141a";
  ctx.fillRect(0, 0, width, height);

  // Grid

  ctx.strokeStyle = "#263640";
  ctx.lineWidth = 1;

  for(let i = 0; i <= 4; i++)
  {

    let y = 20 + i * ((height - 40) / 4);

    ctx.beginPath();
    ctx.moveTo(45, y);
    ctx.lineTo(width - 10, y);
    ctx.stroke();

  }

  // Y-axis labels

  ctx.fillStyle = "#aebbc3";
  ctx.font = "12px Arial";

  ctx.fillText("1000", 5, 24);
  ctx.fillText("750", 10, height / 4 + 4);
  ctx.fillText("500", 10, height / 2 + 4);
  ctx.fillText("250", 10, height * 3 / 4 + 4);
  ctx.fillText("0", 20, height - 15);

  // Threshold line

  let thresholdY =
    height - 20 -
    (THRESHOLD / MAX_PPM) *
    (height - 40);

  ctx.strokeStyle = "#ffb020";
  ctx.setLineDash([6, 5]);

  ctx.beginPath();
  ctx.moveTo(45, thresholdY);
  ctx.lineTo(width - 10, thresholdY);
  ctx.stroke();

  ctx.setLineDash([]);

  // Threshold text

  ctx.fillStyle = "#ffb020";
  ctx.fillText(
    "Threshold 500 PPM",
    width - 125,
    thresholdY - 5
  );

  // Graph line

  if(data.length > 1)
  {

    ctx.strokeStyle = "#4dc3ff";
    ctx.lineWidth = 2;

    ctx.beginPath();

    for(let i = 0; i < data.length; i++)
    {

      let x =
        45 +
        (i / (MAX_POINTS - 1)) *
        (width - 55);

      let value =
        Math.max(
          0,
          Math.min(MAX_PPM, data[i])
        );

      let y =
        height - 20 -
        (value / MAX_PPM) *
        (height - 40);

      if(i === 0)
        ctx.moveTo(x, y);
      else
        ctx.lineTo(x, y);

    }

    ctx.stroke();

  }

  // Graph title

  ctx.fillStyle = "#ffffff";
  ctx.font = "bold 13px Arial";

  ctx.fillText(
    label,
    50,
    height - 5
  );

}

// =====================================================
// UPDATE DASHBOARD
// =====================================================

function updateDashboard(data)
{

  const mq6 =
    Number(data.mq6) || 0;

  const mq9 =
    Number(data.mq9) || 0;

  // CURRENT VALUES

  document.getElementById("mq6Value")
    .textContent = Math.round(mq6);

  document.getElementById("mq9Value")
    .textContent = Math.round(mq9);

  // HISTORY

  mq6History.push(mq6);
  mq9History.push(mq9);

  if(mq6History.length > MAX_POINTS)
    mq6History.shift();

  if(mq9History.length > MAX_POINTS)
    mq9History.shift();

  // GRAPHS

  drawGraph(
    "mq6Graph",
    mq6History,
    "MQ-6 PPM"
  );

  drawGraph(
    "mq9Graph",
    mq9History,
    "MQ-9 PPM"
  );

  // INDIVIDUAL STATUS

  const mq6Status =
    document.getElementById("mq6Status");

  const mq9Status =
    document.getElementById("mq9Status");

  if(mq6 >= THRESHOLD)
  {

    mq6Status.textContent =
      "HIGH";

    mq6Status.className =
      "status high";

  }
  else
  {

    mq6Status.textContent =
      "NORMAL";

    mq6Status.className =
      "status normal";

  }

  if(mq9 >= THRESHOLD)
  {

    mq9Status.textContent =
      "HIGH";

    mq9Status.className =
      "status high";

  }
  else
  {

    mq9Status.textContent =
      "NORMAL";

    mq9Status.className =
      "status normal";

  }

  // SAFETY STATUS

  const safetyBox =
    document.getElementById("safetyBox");

  const safetyText =
    document.getElementById("safetyText");

  if(mq6 >= THRESHOLD &&
     mq9 >= THRESHOLD)
  {

    safetyBox.className =
      "safety emergency";

    safetyText.textContent =
      "EMERGENCY";

  }

  else if(mq6 >= THRESHOLD)
  {

    safetyBox.className =
      "safety warning";

    safetyText.textContent =
      "WARNING - MQ-6 GAS HIGH";

  }

  else if(mq9 >= THRESHOLD)
  {

    safetyBox.className =
      "safety warning";

    safetyText.textContent =
      "WARNING - MQ-9 GAS HIGH";

  }

  else
  {

    safetyBox.className =
      "safety safe";

    safetyText.textContent =
      "SAFE";

  }

  // EMERGENCY ALERT

  const emergencyBox =
    document.getElementById("emergencyBox");

  if(mq6 >= THRESHOLD &&
     mq9 >= THRESHOLD)
  {

    emergencyBox.style.display =
      "block";

    document.getElementById("emergencyMQ6")
      .textContent = Math.round(mq6);

    document.getElementById("emergencyMQ9")
      .textContent = Math.round(mq9);

  }

  else
  {

    emergencyBox.style.display =
      "none";

  }

}

// =====================================================
// GET DATA FROM ESP32
// =====================================================

async function getData()
{

  try
  {

    const response =
      await fetch("/data");

    const data =
      await response.json();

    updateDashboard(data);

  }

  catch(error)
  {

    console.log(
      "Dashboard connection error:",
      error
    );

  }

}

// =====================================================
// START
// =====================================================

getData();

setInterval(
  getData,
  1000
);

window.addEventListener(
  "resize",
  function()
  {

    drawGraph(
      "mq6Graph",
      mq6History,
      "MQ-6 PPM"
    );

    drawGraph(
      "mq9Graph",
      mq9History,
      "MQ-9 PPM"
    );

  }
);

</script>

</body>
</html>
)rawliteral";

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

void calibrateMQ9();
void readMQ9();
void readGPS();
void receiveGasPacket();
void updateGasAlerts();
void updateBuzzer();
void sendGPSAlert();
void updateOLED();

void handleDashboard();
void handleDashboardData();

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // ---------------------------------------------------
  // BUTTONS
  // ---------------------------------------------------

  pinMode(BUTTON_FORWARD, INPUT_PULLUP);
  pinMode(BUTTON_BACKWARD, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  buzzerState = false;
  buzzerTimer = millis() - BUZZER_OFF_TIME;

  // ---------------------------------------------------
  // MQ-9
  // ---------------------------------------------------

  pinMode(MQ9_PIN, INPUT);
  analogReadResolution(12);

  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  GPS.begin(
    GPS_BAUD,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );

  // ---------------------------------------------------
  // OLED
  // ---------------------------------------------------

  Wire.begin(OLED_SDA, OLED_SCL);

  oled.begin();

  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);

  oled.drawStr(15, 20, "COAL MINE");
  oled.drawStr(15, 35, "SAFETY SYSTEM");
  oled.drawStr(15, 50, "STARTING...");

  oled.sendBuffer();

  delay(1500);

  // ---------------------------------------------------
  // MQ-9 CALIBRATION
  // ---------------------------------------------------

  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);

  oled.drawStr(5, 20, "MQ-9 CALIBRATION");
  oled.drawStr(5, 35, "KEEP IN CLEAN AIR");
  oled.drawStr(5, 50, "PLEASE WAIT...");

  oled.sendBuffer();

  delay(1000);

  calibrateMQ9();

  // ---------------------------------------------------
  // LoRa
  // ---------------------------------------------------

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  if (!LoRa.begin(LORA_FREQ))
  {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tf);

    oled.drawStr(10, 25, "LoRa ERROR!");
    oled.drawStr(10, 42, "CHECK WIRING");

    oled.sendBuffer();

    while (1)
    {
      digitalWrite(BUZZER_PIN, LOW);
      delay(1000);
    }
  }

  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  // ---------------------------------------------------
  // WIFI DASHBOARD - ROUTER WIFI
  // ---------------------------------------------------

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.println();
  Serial.println("================================");
  Serial.println("CONNECTING TO ROUTER WIFI");
  Serial.print("WiFi SSID: ");
  Serial.println(WIFI_SSID);

  unsigned long wifiStartTime = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    // Wait maximum 20 seconds
    if (millis() - wifiStartTime >= 20000)
    {
      break;
    }
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WIFI CONNECTED!");
    Serial.print("ESP32 IP ADDRESS: ");
    Serial.println(WiFi.localIP());

    Serial.print("Dashboard: http://");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WIFI CONNECTION FAILED!");
    Serial.println("CHECK WIFI SSID AND PASSWORD");
  }

  Serial.println("================================");
  Serial.println();

  // ---------------------------------------------------
  // DASHBOARD ROUTES
  // ---------------------------------------------------

  server.on(
    "/",
    handleDashboard
  );

  server.on(
    "/data",
    handleDashboardData
  );

  server.begin();

  Serial.println("WEB SERVER STARTED");

  // ---------------------------------------------------
  // READY SCREEN
  // ---------------------------------------------------

  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);

  oled.drawStr(15, 20, "MQ-6 + MQ-9");
  oled.drawStr(15, 38, "WAITING...");

  oled.sendBuffer();
}

// =====================================================
// MQ-9 CALIBRATION
// =====================================================

void calibrateMQ9()
{
  unsigned long startTime = millis();

  float rsSum = 0.0;
  int samples = 0;

  while (millis() - startTime < MQ9_CALIBRATION_TIME)
  {
    int raw = analogRead(MQ9_PIN);

    float adcVoltage =
      (raw / 4095.0) * 3.3;

    // 22k + 22k voltage divider
    float sensorVoltage =
      adcVoltage * 2.0;

    if (sensorVoltage > 0.05 &&
        sensorVoltage < 4.95)
    {
      float rs =
        MQ9_RL *
        (5.0 - sensorVoltage) /
        sensorVoltage;

      if (rs > 0)
      {
        rsSum += rs;
        samples++;
      }
    }

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tf);

    oled.drawStr(5, 15, "MQ-9 CALIBRATING");

    oled.setCursor(5, 32);
    oled.print("TIME: ");
    oled.print(
      (millis() - startTime) / 1000
    );
    oled.print("s");

    oled.drawStr(
      5,
      48,
      "KEEP IN CLEAN AIR"
    );

    oled.sendBuffer();

    delay(100);
  }

  if (samples > 0)
  {
    float averageRs =
      rsSum / samples;

    mq9_R0 =
      averageRs / 9.83;
  }
  else
  {
    mq9_R0 = 10.0;
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("MQ-9 CALIBRATION COMPLETE");
  Serial.print("MQ-9 R0 = ");
  Serial.println(mq9_R0);
  Serial.println("================================");
  Serial.println();
}

// =====================================================
// READ MQ-9
// =====================================================

void readMQ9()
{
  mq9Raw = analogRead(MQ9_PIN);

  mq9ADCVoltage =
    (mq9Raw / 4095.0) * 3.3;

  mq9SensorVoltage =
    mq9ADCVoltage * 2.0;

  if (mq9SensorVoltage > 0.05 &&
      mq9SensorVoltage < 4.95)
  {
    mq9Rs =
      MQ9_RL *
      (5.0 - mq9SensorVoltage) /
      mq9SensorVoltage;

    if (mq9_R0 > 0)
    {
      mq9Ratio =
        mq9Rs / mq9_R0;

      if (mq9Ratio > 0)
      {
        mq9PPM =
          100.0 *
          pow(mq9Ratio, -1.5);
      }
      else
      {
        mq9PPM = 0.0;
      }
    }
  }
  else
  {
    mq9Rs = 0.0;
    mq9Ratio = 0.0;
    mq9PPM = 0.0;
  }

  if (mq9PPM < 0)
    mq9PPM = 0;

  if (mq9PPM > MQ9_MAX_PPM)
    mq9PPM = MQ9_MAX_PPM;

  mq9Alert =
    (mq9PPM >= GAS_ALERT_THRESHOLD);

  static unsigned long lastMQ9Print = 0;

  if (millis() - lastMQ9Print >= 1000)
  {
    lastMQ9Print = millis();

    Serial.print("MQ9 RAW = ");
    Serial.print(mq9Raw);

    Serial.print(" | MQ9 V = ");
    Serial.print(mq9SensorVoltage, 3);

    Serial.print(" | MQ9 EST PPM = ");
    Serial.print(mq9PPM, 0);

    Serial.print(" | MQ9 ");

    if (mq9Alert)
      Serial.println("ALERT");
    else
      Serial.println("NORMAL");
  }
}

// =====================================================
// READ GPS
// =====================================================

void readGPS()
{
  while (GPS.available())
  {
    char c = GPS.read();
    gps.encode(c);
  }

  if (gps.location.isValid())
  {
    gpsFix = true;

    gpsLatitude =
      gps.location.lat();

    gpsLongitude =
      gps.location.lng();
  }
  else
  {
    gpsFix = false;
  }

  if (millis() - lastGPSPrint >= 2000)
  {
    lastGPSPrint = millis();

    if (gpsFix)
    {
      Serial.print("GPS FIX: ");

      Serial.print(
        gpsLatitude,
        6
      );

      Serial.print(", ");

      Serial.println(
        gpsLongitude,
        6
      );
    }
    else
    {
      Serial.println(
        "GPS: WAITING FOR FIX..."
      );
    }
  }
}

// =====================================================
// RECEIVE MQ-6 GAS PACKET
// =====================================================

void receiveGasPacket()
{
  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    String packet = "";

    while (LoRa.available())
    {
      packet +=
        (char)LoRa.read();
    }

    Serial.print("Received: ");
    Serial.println(packet);

    if (packet.startsWith("GAS:"))
    {
      int commaIndex =
        packet.indexOf(',');

      if (commaIndex > 4)
      {
        String gasValue =
          packet.substring(
            4,
            commaIndex
          );

        gasPPM =
          gasValue.toFloat();

        if (gasPPM < 0)
          gasPPM = 0;

        if (gasPPM > 1000)
          gasPPM = 1000;

        gasReceived = true;

        lastGasReceived =
          millis();

        mq6Alert =
          (gasPPM >= GAS_ALERT_THRESHOLD);
      }
    }

    LoRa.receive();
  }
}

// =====================================================
// UPDATE GAS ALERTS
// =====================================================

void updateGasAlerts()
{
  mq6Alert =
    (gasPPM >= GAS_ALERT_THRESHOLD);

  mq9Alert =
    (mq9PPM >= GAS_ALERT_THRESHOLD);

  bothGasAlert =
    mq6Alert && mq9Alert;
}

// =====================================================
// BUZZER
// =====================================================

void updateBuzzer()
{
  updateGasAlerts();

  if (bothGasAlert)
  {
    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    buzzerState = true;

    return;
  }

  if (mq6Alert || mq9Alert)
  {
    unsigned long currentTime =
      millis();

    if (buzzerState)
    {
      if (currentTime - buzzerTimer >=
          BUZZER_ON_TIME)
      {
        buzzerState = false;
        buzzerTimer = currentTime;

        digitalWrite(
          BUZZER_PIN,
          LOW
        );
      }
    }
    else
    {
      if (currentTime - buzzerTimer >=
          BUZZER_OFF_TIME)
      {
        buzzerState = true;
        buzzerTimer = currentTime;

        digitalWrite(
          BUZZER_PIN,
          HIGH
        );
      }
    }

    return;
  }

  buzzerState = false;

  digitalWrite(
    BUZZER_PIN,
    LOW
  );
}

// =====================================================
// SEND GPS ALERT
// =====================================================

void sendGPSAlert()
{
  if (millis() - lastAlertMessage < 5000)
  {
    return;
  }

  lastAlertMessage = millis();

  if (gpsFix)
  {
    Serial.print("ALERT,GAS=");
    Serial.print(gasPPM, 0);

    Serial.print(",MQ9=");
    Serial.print(mq9PPM, 0);

    Serial.print(",LAT=");
    Serial.print(gpsLatitude, 6);

    Serial.print(",LON=");
    Serial.println(gpsLongitude, 6);
  }

  else
  {
    Serial.print("ALERT,GAS=");
    Serial.print(gasPPM, 0);

    Serial.print(",MQ9=");
    Serial.print(mq9PPM, 0);

    Serial.print(",LAT=");
    Serial.print(11.016800, 6);

    Serial.print(",LON=");
    Serial.print(76.955800, 6);

    Serial.println(",GPS_DEMO");
  }
}

// =====================================================
// UPDATE OLED
// =====================================================

void updateOLED()
{
  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    9,
    "COAL MINE GAS MONITOR"
  );

  oled.setCursor(0, 21);

  oled.print("MQ6:");

  if (gasReceived)
  {
    oled.print(gasPPM, 0);
    oled.print("ppm");
  }
  else
  {
    oled.print("WAIT");
  }

  oled.setCursor(68, 21);

  oled.print("MQ9:");
  oled.print(mq9PPM, 0);
  oled.print("ppm");

  if (bothGasAlert)
  {
    oled.drawStr(
      5,
      35,
      "!! BOTH GAS ALERT !!"
    );

    oled.drawStr(
      12,
      47,
      "DANGER - EVACUATE"
    );
  }

  else if (mq6Alert)
  {
    oled.drawStr(
      10,
      35,
      "!! MQ6 GAS ALERT !!"
    );

    oled.drawStr(
      20,
      47,
      "MQ6 > 500 ppm"
    );
  }

  else if (mq9Alert)
  {
    oled.drawStr(
      10,
      35,
      "!! MQ9 GAS ALERT !!"
    );

    oled.drawStr(
      20,
      47,
      "MQ9 > 500 ppm"
    );
  }

  else
  {
    oled.drawStr(
      25,
      35,
      "GAS NORMAL"
    );

    oled.drawStr(
      18,
      47,
      "MQ6 + MQ9 SAFE"
    );
  }

  if (gpsFix)
  {
    oled.setCursor(0, 62);

    oled.print(
      gpsLatitude,
      4
    );

    oled.print(",");

    oled.print(
      gpsLongitude,
      4
    );
  }
  else
  {
    oled.drawStr(
      0,
      62,
      "GPS: WAITING FIX"
    );
  }

  oled.sendBuffer();
}

// =====================================================
// DASHBOARD MAIN PAGE
// =====================================================

void handleDashboard()
{
  server.send_P(
    200,
    "text/html",
    DASHBOARD_HTML
  );
}

// =====================================================
// DASHBOARD DATA
// =====================================================

void handleDashboardData()
{
  String json = "{";

  json += "\"mq6\":";
  json += String(gasPPM, 0);

  json += ",\"mq9\":";
  json += String(mq9PPM, 0);

  json += ",\"threshold\":";
  json += String(GAS_ALERT_THRESHOLD, 0);

  json += ",\"max\":";
  json += String(MQ9_MAX_PPM, 0);

  json += ",\"mq6Alert\":";
  json += (mq6Alert ? "true" : "false");

  json += ",\"mq9Alert\":";
  json += (mq9Alert ? "true" : "false");

  json += ",\"bothAlert\":";
  json += (bothGasAlert ? "true" : "false");

  json += ",\"gpsFix\":";
  json += (gpsFix ? "true" : "false");

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  // ---------------------------------------------------
  // WEB DASHBOARD
  // ---------------------------------------------------

  server.handleClient();

  // ---------------------------------------------------
  // READ GPS
  // ---------------------------------------------------

  readGPS();

  // ---------------------------------------------------
  // READ MQ-9
  // ---------------------------------------------------

  readMQ9();

  // ---------------------------------------------------
  // READ BUTTONS
  // ---------------------------------------------------

  bool forwardPressed =
    digitalRead(BUTTON_FORWARD) == LOW;

  bool backwardPressed =
    digitalRead(BUTTON_BACKWARD) == LOW;

  bool leftPressed =
    digitalRead(BUTTON_LEFT) == LOW;

  bool rightPressed =
    digitalRead(BUTTON_RIGHT) == LOW;

  // ---------------------------------------------------
  // COMMAND
  // ---------------------------------------------------

  String command = "";

  if (
    forwardPressed &&
    !backwardPressed &&
    !leftPressed &&
    !rightPressed
  )
  {
    command =
      "CH1:ON,CH2:OFF,CH3:OFF,CH4:OFF";
  }

  else if (
    backwardPressed &&
    !forwardPressed &&
    !leftPressed &&
    !rightPressed
  )
  {
    command =
      "CH1:OFF,CH2:ON,CH3:OFF,CH4:OFF";
  }

  else if (
    leftPressed &&
    !forwardPressed &&
    !backwardPressed &&
    !rightPressed
  )
  {
    command =
      "CH1:OFF,CH2:OFF,CH3:ON,CH4:OFF";
  }

  else if (
    rightPressed &&
    !forwardPressed &&
    !backwardPressed &&
    !leftPressed
  )
  {
    command =
      "CH1:OFF,CH2:OFF,CH3:OFF,CH4:ON";
  }

  else
  {
    command =
      "CH1:OFF,CH2:OFF,CH3:OFF,CH4:OFF";
  }

  // ---------------------------------------------------
  // SEND LoRa COMMAND
  // ---------------------------------------------------

  LoRa.beginPacket();

  LoRa.print(command);

  LoRa.endPacket();

  Serial.print("Sent: ");
  Serial.println(command);

  LoRa.receive();

  // ---------------------------------------------------
  // RECEIVE WINDOW
  // ---------------------------------------------------

  unsigned long receiveStart =
    millis();

  while (
    millis() - receiveStart < 80
  )
  {
    // Dashboard remains responsive
    server.handleClient();

    readGPS();

    receiveGasPacket();

    readMQ9();

    updateBuzzer();

    delay(1);
  }

  // ---------------------------------------------------
  // UPDATE ALERTS
  // ---------------------------------------------------

  updateGasAlerts();

  updateBuzzer();

  // ---------------------------------------------------
  // SEND GPS ALERT
  // ---------------------------------------------------

  if (mq6Alert || mq9Alert)
  {
    sendGPSAlert();
  }

  // ---------------------------------------------------
  // OLED
  // ---------------------------------------------------

  updateOLED();

  // ---------------------------------------------------
  // DASHBOARD
  // ---------------------------------------------------

  server.handleClient();

  delay(20);
}
