#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_VL53L0X.h>


// =====================================================
// LoRa PINS
// =====================================================

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26


// =====================================================
// L298N MOTOR DRIVER PINS
// =====================================================

// LEFT MOTOR
#define IN1 25
#define IN2 27

// RIGHT MOTOR
#define IN3 32
#define IN4 33


// =====================================================
// TOF SENSOR PINS
// =====================================================

#define TOF_SDA 21
#define TOF_SCL 22

// 10 cm = 100 mm
#define OBSTACLE_DISTANCE 100


// =====================================================
// MQ-6 SENSOR
// =====================================================

// MQ-6 AO -> 22K -> GPIO34
// GPIO34  -> 22K -> GND

#define MQ6_PIN 34

#define ADC_MAX       4095.0
#define ADC_REFERENCE 3.3

// 22K / 22K voltage divider
#define DIVIDER_RATIO 2.0


// =====================================================
// MQ-6 CALIBRATION
// =====================================================

#define RL_KOHM 10.0

#define R0_KOHM 10.0


// =====================================================
// GAS SETTINGS
// =====================================================

// Maximum displayed/calculated PPM
#define MAX_PPM 1000.0

// Alert when PPM is ABOVE 500
#define GAS_ALERT_THRESHOLD 500.0


// =====================================================
// TOF OBJECT
// =====================================================

Adafruit_VL53L0X tof;


// =====================================================
// MOTOR CHANNEL STATES
// =====================================================

bool CH1 = false;
bool CH2 = false;
bool CH3 = false;
bool CH4 = false;


// =====================================================
// OBSTACLE STATE
// =====================================================

bool obstacleDetected = false;


// =====================================================
// MQ-6 VARIABLES
// =====================================================

int mq6Raw = 0;

float adcVoltage = 0.0;
float mq6Voltage = 0.0;

float Rs_Kohm = 0.0;
float Rs_R0 = 0.0;

float estimatedPPM = 0.0;

bool gasAlert = false;


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("        LoRa RC CAR RECEIVER");
  Serial.println("========================================");


  // ===================================================
  // MOTOR PINS
  // ===================================================

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopCar();


  // ===================================================
  // MQ-6
  // ===================================================

  pinMode(MQ6_PIN, INPUT);

  analogReadResolution(12);

  Serial.println("MQ-6 initialized");
  Serial.println("MQ-6 AO -> GPIO34");
  Serial.println("Voltage divider -> 22K / 22K");
  Serial.println("Maximum PPM = 1000");
  Serial.println("Gas alert threshold = ABOVE 500 ppm");
  Serial.println();


  // ===================================================
  // TOF
  // ===================================================

  Serial.println("Starting ToF sensor...");

  Wire.begin(TOF_SDA, TOF_SCL);

  if (!tof.begin())
  {
    Serial.println("********************************");
    Serial.println("ToF SENSOR INITIALIZATION FAILED");
    Serial.println("Check VL53L0X wiring");
    Serial.println("********************************");

    while (1)
    {
      stopCar();
      delay(1000);
    }
  }

  Serial.println("ToF sensor initialized successfully");
  Serial.println("Obstacle limit = 10 cm");
  Serial.println();


  // ===================================================
  // LoRa SPI
  // ===================================================

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );


  // ===================================================
  // LoRa PINS
  // ===================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


  // ===================================================
  // START LoRa
  // ===================================================

  Serial.println("Starting LoRa...");

  if (!LoRa.begin(433E6))
  {
    Serial.println("********************************");
    Serial.println("LoRa INITIALIZATION FAILED");
    Serial.println("Check LoRa wiring");
    Serial.println("********************************");

    while (1)
    {
      stopCar();
      delay(1000);
    }
  }


  // ===================================================
  // LoRa SETTINGS
  // MUST MATCH TX
  // ===================================================

  LoRa.setTxPower(17);

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  LoRa.setCodingRate4(5);


  // ===================================================
  // READY
  // ===================================================

  Serial.println("LoRa initialized successfully");
  Serial.println("RX READY");
  Serial.println("Waiting for commands...");
  Serial.println();
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  // ===================================================
  // READ MQ-6
  // ===================================================

  readMQ6();


  // ===================================================
  // CHECK FRONT OBSTACLE
  // ===================================================

  checkObstacle();


  // ===================================================
  // RECEIVE COMMAND FROM TX
  // ===================================================

  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    String command = "";

    while (LoRa.available())
    {
      command += (char)LoRa.read();
    }

    command.trim();


    // -------------------------------------------------
    // DISPLAY RECEIVED COMMAND
    // -------------------------------------------------

    Serial.print("Received: ");
    Serial.println(command);


    // =================================================
    // CH1 - FORWARD
    // =================================================

    if (command.indexOf("CH1:ON") >= 0)
      CH1 = true;
    else
      CH1 = false;


    // =================================================
    // CH2 - BACKWARD
    // =================================================

    if (command.indexOf("CH2:ON") >= 0)
      CH2 = true;
    else
      CH2 = false;


    // =================================================
    // CH3 - LEFT
    // =================================================

    if (command.indexOf("CH3:ON") >= 0)
      CH3 = true;
    else
      CH3 = false;


    // =================================================
    // CH4 - RIGHT
    // =================================================

    if (command.indexOf("CH4:ON") >= 0)
      CH4 = true;
    else
      CH4 = false;


    // =================================================
    // MOTOR CONTROL
    // =================================================

    controlCar();


    // =================================================
    // SEND GAS DATA IMMEDIATELY
    // =================================================

    sendGasData();
  }
}


// =====================================================
// READ MQ-6
// =====================================================

void readMQ6()
{
  mq6Raw = analogRead(MQ6_PIN);


  // ===================================================
  // ESP32 ADC VOLTAGE
  // ===================================================

  adcVoltage =
    ((float)mq6Raw / ADC_MAX) * ADC_REFERENCE;


  // ===================================================
  // ACTUAL MQ-6 AO VOLTAGE
  // AFTER REMOVING VOLTAGE DIVIDER
  // ===================================================

  mq6Voltage =
    adcVoltage * DIVIDER_RATIO;


  // ===================================================
  // CALCULATE Rs
  // ===================================================

  if (mq6Voltage > 0.05 && mq6Voltage < 4.95)
  {
    Rs_Kohm =
      RL_KOHM * (5.0 - mq6Voltage) / mq6Voltage;
  }
  else
  {
    Rs_Kohm = 0;
  }


  // ===================================================
  // Rs / R0
  // ===================================================

  if (R0_KOHM > 0)
  {
    Rs_R0 = Rs_Kohm / R0_KOHM;
  }
  else
  {
    Rs_R0 = 0;
  }


  // ===================================================
  // ESTIMATED PPM
  // ===================================================

  estimatedPPM = calculatePPM(Rs_R0);


  // ===================================================
  // GAS ALERT
  // ===================================================

  if (estimatedPPM > GAS_ALERT_THRESHOLD)
  {
    gasAlert = true;
  }
  else
  {
    gasAlert = false;
  }


  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.print("MQ-6 ADC: ");
  Serial.print(mq6Raw);

  Serial.print(" | ESP32 ADC: ");
  Serial.print(adcVoltage, 3);

  Serial.print(" V | MQ-6 AO: ");
  Serial.print(mq6Voltage, 3);

  Serial.print(" V | Rs: ");
  Serial.print(Rs_Kohm, 2);

  Serial.print(" KOhm | Rs/R0: ");
  Serial.print(Rs_R0, 2);

  Serial.print(" | Estimated Gas: ");
  Serial.print(estimatedPPM, 0);

  Serial.print(" ppm | ");

  if (gasAlert)
  {
    Serial.println("!! GAS ALERT !!");
  }
  else
  {
    Serial.println("GAS NORMAL");
  }
}


// =====================================================
// CALCULATE ESTIMATED PPM
// =====================================================

float calculatePPM(float ratio)
{
  if (ratio <= 0)
  {
    return 0;
  }


  // Approximate MQ-6 response relationship.
  // This is NOT a final calibrated methane curve.

  float ppm =
    2000.0 * pow((ratio / 0.55), -2.2);


  // ===================================================
  // LIMIT PPM TO 1000
  // ===================================================

  if (ppm < 0)
  {
    ppm = 0;
  }

  if (ppm > MAX_PPM)
  {
    ppm = MAX_PPM;
  }


  return ppm;
}


// =====================================================
// SEND GAS DATA TO TX
// =====================================================

void sendGasData()
{
  String packet = "GAS:";

  packet += String(estimatedPPM, 0);

  packet += ",V:";

  packet += String(mq6Voltage, 2);

  packet += ",ALERT:";

  if (gasAlert)
  {
    packet += "1";
  }
  else
  {
    packet += "0";
  }


  // ===================================================
  // TRANSMIT
  // ===================================================

  Serial.print("Sending gas packet: ");
  Serial.println(packet);

  LoRa.beginPacket();

  LoRa.print(packet);

  LoRa.endPacket();


  // ===================================================
  // RETURN TO RECEIVE MODE
  // ===================================================

  LoRa.receive();


  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.print("Sent to TX: ");
  Serial.println(packet);
}


// =====================================================
// CHECK TOF
// =====================================================

void checkObstacle()
{
  VL53L0X_RangingMeasurementData_t measure;

  tof.rangingTest(&measure, false);


  // ===================================================
  // VALID READING
  // ===================================================

  if (measure.RangeStatus != 4)
  {
    int distance = measure.RangeMilliMeter;


    Serial.print("ToF Distance: ");
    Serial.print(distance);
    Serial.println(" mm");


    // =================================================
    // OBSTACLE LESS THAN 10 CM
    // =================================================

    if (distance < OBSTACLE_DISTANCE)
    {
      obstacleDetected = true;

      // IMMEDIATE STOP
      stopCar();
    }
    else
    {
      obstacleDetected = false;
    }
  }
}


// =====================================================
// CONTROL CAR
// =====================================================

void controlCar()
{
  // ===================================================
  // OBSTACLE DETECTED
  // ===================================================

  if (obstacleDetected)
  {
    stopCar();
    return;
  }


  // ===================================================
  // FORWARD
  // ===================================================

  if (CH1 && !CH2 && !CH3 && !CH4)
  {
    moveForward();
  }


  // ===================================================
  // BACKWARD
  // ===================================================

  else if (CH2 && !CH1 && !CH3 && !CH4)
  {
    moveBackward();
  }


  // ===================================================
  // LEFT
  // ===================================================

  else if (CH3 && !CH1 && !CH2 && !CH4)
  {
    turnLeft();
  }


  // ===================================================
  // RIGHT
  // ===================================================

  else if (CH4 && !CH1 && !CH2 && !CH3)
  {
    turnRight();
  }


  // ===================================================
  // NOTHING PRESSED
  // ===================================================

  else if (!CH1 && !CH2 && !CH3 && !CH4)
  {
    stopCar();
  }


  // ===================================================
  // MULTIPLE BUTTONS
  // ===================================================

  else
  {
    Serial.println("Multiple commands detected!");
    Serial.println("ACTION: SAFETY STOP");

    stopCar();
  }
}


// =====================================================
// FORWARD
// =====================================================

void moveForward()
{
  Serial.println("ACTION: FORWARD");

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}


// =====================================================
// BACKWARD
// =====================================================

void moveBackward()
{
  Serial.println("ACTION: BACKWARD");

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


// =====================================================
// LEFT
// =====================================================

void turnLeft()
{
  Serial.println("ACTION: LEFT");

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}


// =====================================================
// RIGHT
// =====================================================

void turnRight()
{
  Serial.println("ACTION: RIGHT");

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


// =====================================================
// STOP
// =====================================================

void stopCar()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
