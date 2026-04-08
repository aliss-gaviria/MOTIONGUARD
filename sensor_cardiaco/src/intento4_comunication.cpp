#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "AsyncTaskLib.h"
#include "StateMachineLib.h"
#include "esp_sleep.h"
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

QueueHandle_t bpmQueue; 
const int QUEUE_LEN = 10; 

void readSensor();
void reportData();
void stabilizationDone();
void monitoringDone();
void setupStateMachine();
void onInit();
void onStabilizing();
void onMonitoring();
void onSleeping();
void sendData();


uint8_t receiverMac[] = {0xB4, 0x3A, 0x45, 0x29, 0xA0, 0x78}; 


bool stabilizationFinished = false;
bool monitoringFinished = false;


MAX30105 particleSensor;


const byte RATE_SIZE = 12;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;
long irValue;

enum State {
  INIT = 0,
  STABILIZING,
  MONITORING,
  SLEEPING
};

StateMachine stateMachine(4, 5);
State currentState;


AsyncTask TaskReadSensor(5, true, readSensor);  
AsyncTask TaskReport(2000, true, reportData);
AsyncTask TaskStabilizationTimer(15000, false, stabilizationDone);
AsyncTask TaskMonitoringTimer(10000, false, monitoringDone);


bool fingerDetected = false;


void setup() {
  Serial.begin(115200);

  bpmQueue = xQueueCreate(QUEUE_LEN, sizeof(int));
  
  if (bpmQueue == NULL) {
    Serial.println("Error creando la cola");
  }

  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 70;
  }

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Sensor MAX30105 no encontrado");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x3F);
  particleSensor.setPulseAmplitudeGreen(0);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); 

  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA; 

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error agregando peer");
    return;
  }

  Serial.println("ESP-NOW listo");

  setupStateMachine();
  stateMachine.SetState(INIT, true, true);
}


void loop() {

  TaskReadSensor.Update();
  TaskReport.Update();
  TaskStabilizationTimer.Update();
  TaskMonitoringTimer.Update();

  if (currentState == INIT && !fingerDetected) {
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 1000) {
      Serial.println("Esperando dedo...");
      lastMsg = millis();
    }
  }

  stateMachine.Update();
}

void setupStateMachine() {

  stateMachine.AddTransition(INIT, STABILIZING, []() {
    return fingerDetected;
  });

  stateMachine.AddTransition(STABILIZING, INIT, []() {
    return !fingerDetected;
  });

  stateMachine.AddTransition(STABILIZING, MONITORING, []() {
    return stabilizationFinished;
  });

  stateMachine.AddTransition(MONITORING, INIT, []() {
    return !fingerDetected;
  });

  stateMachine.AddTransition(MONITORING, SLEEPING, []() {
    return monitoringFinished;
  });

  stateMachine.SetOnEntering(INIT, onInit);
  stateMachine.SetOnEntering(STABILIZING, onStabilizing);
  stateMachine.SetOnEntering(MONITORING, onMonitoring);
  stateMachine.SetOnEntering(SLEEPING, onSleeping);
}

void onInit() {

  Serial.println("\nEstado: INIT - Esperando dedo");

  currentState = INIT;

  stabilizationFinished = false;
  monitoringFinished = false;
  TaskReadSensor.Start();
  TaskReport.Stop();
  TaskStabilizationTimer.Stop();
  TaskMonitoringTimer.Stop();
}

void onStabilizing() {
  Serial.println("\nEstado: Estabilizando (15s)");

  currentState = STABILIZING;

  TaskReport.Stop();
  TaskStabilizationTimer.Start();
}

void onMonitoring() {
  Serial.println("\nEstado: MONITOREO (10s)");
  currentState = MONITORING;

  xQueueReset(bpmQueue);

  TaskReport.Start();
  TaskMonitoringTimer.Start();
}

void onSleeping() {
  Serial.println("\nEstado: SLEEPING (50s)");

  TaskReadSensor.Stop();
  TaskReport.Stop();

  delay(100);

  esp_sleep_enable_timer_wakeup(50 * 1000000);
  esp_deep_sleep_start();
}

void readSensor() {

  irValue = particleSensor.getIR();

  fingerDetected = (irValue > 50000);

  if (!fingerDetected) return;

  if (checkForBeat(irValue)) {

    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute > 40 && beatsPerMinute < 180) {

      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      int validCount = 0;
      beatAvg = 0;

      for (byte i = 0; i < RATE_SIZE; i++) {
        if (rates[i] > 0) {
          beatAvg += rates[i];
          validCount++;
        }
      }

      if (validCount > 0) beatAvg /= validCount;

      if (currentState == MONITORING) {

        xQueueSend(bpmQueue, &beatAvg, 0);
      }
    }
  }
}

void sendData(int beatAvg) {

  StaticJsonDocument<200> doc;

  doc["id"] = "Nodo_ritmo_cardiaco";
  doc["mac"] = WiFi.macAddress();
  doc["BPM"] = beatAvg;

  char buffer[200];
  serializeJson(doc, buffer);

  esp_now_send(receiverMac, (uint8_t *)buffer, strlen(buffer));
}

void reportData() {

  int dataReceived;

  if (!fingerDetected) {
    Serial.println("Dedo retirado - datos no válidos");
    return;
  }

  while (xQueueReceive(bpmQueue, &dataReceived, 0) == pdTRUE) {
    Serial.print("BPM: ");
    Serial.println(dataReceived);
    sendData(dataReceived); 
    delay(10);
  }
 
}

void stabilizationDone() {
  stabilizationFinished = true;
}

void monitoringDone() {
  monitoringFinished = true;
}