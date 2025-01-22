#include "MorseEncoder.h"

// #define DEBUG

// configuración de e/s
#define PTT_PIN 10
#define AUDIO_IN_PIN A3
#define AUDIO_OUT_PIN 12
#define LED_PIN 13
#define ADC_OFFSET 512

// configuración de PTT
#define PTT_AUDIO_TRIGGER 250  // umbral de detección (nivel de audio) para activación
#define PTT_ACTIVATION_LIMIT_MS 180000  // "anti-poncho" [ms]
//#define PTT_ACTIVATION_LIMIT_MS 30000  // "anti-poncho" [ms]
#define PTT_ACTIVATION_WARNING_MS PTT_ACTIVATION_LIMIT_MS * 0.9  // warning del "anti-poncho"
#define PTT_ACTIVATION_WARNING_INTERVAL_MS 5000  // intervalo de repetición del warning

// configuración de baliza
#define BEACON_INTERVAL 1200000  // intervalo en [ms]
#define BEACON_MESSAGE "RCV LXR 25 DE MAYO BA"  // baliza CW

// generador de CW (baliza)
MorseEncoder morse(AUDIO_OUT_PIN);


/// @brief inicialización de e/s y periféricos
void setup() {
  pinMode(PTT_PIN, OUTPUT);
  digitalWrite(PTT_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(AUDIO_IN_PIN, INPUT);
  pinMode(AUDIO_OUT_PIN, INPUT);

#ifdef DEBUG
  Serial.begin(115200);
#endif

  morse.beginAudio(20, 880);
}

/// @brief samplea ADC y calcula integral, integral en valor absoluto y valor máximo
/// @note el tiempo de sampleo actualmente es de 300ms a 1khz
/// @return 
long getAudioLevel() {
  int sample;
  long integral = 0;
  long maxSample = 0;
  long absIntegral = 0;
  for (int i = 0; i < 300; i++) {  // 300ms
    sample = analogRead(AUDIO_IN_PIN) - ADC_OFFSET;
    integral += sample;
    absIntegral += abs(sample);
    maxSample = max(abs(sample), maxSample);
    delayMicroseconds(1000);  // 1khz
  }
  //Serial.print(integral); Serial.print(",");
  //Serial.print(absIntegral); Serial.print(",");
  //Serial.print(maxSample); Serial.print(",");
  //Serial.println(maxSample > PTT_TRIGGER ? 1 : 0);
  return absIntegral;
}

bool active = false;
long lastActivation = millis();
long lastWarning = millis();
long lastBeacon = millis();
int forceBeaconCnt = 0;
void loop() {
  long activeTime = active ? millis() - lastActivation : 0;

  // activación (ptt) de la repetidora por actividad de audio
  long audioLevel = getAudioLevel();
  if (audioLevel > PTT_AUDIO_TRIGGER) {
    if (!active) {
      active = true;
      lastActivation = millis();
    }
  } else {
    if (active) {
      active = false;
      // baliza forzada
      if (activeTime < 1000)  // activación corta?
        forceBeaconCnt++;  //incrementarmos contador para generar baliza forzada
    }
  }

  // inhibe baliza por actividad (>10s)
  // genera warnings (beeps) por proximidad del "anti-poncho"
  if (active) {
    if (activeTime >= 5000) {
      // reiniciamos temporizador de baliza
      lastBeacon = millis();
      // reiniciamos contador de baliza forzada
      forceBeaconCnt = 0;
    }
    // warning del "anti-poncho"
    if (activeTime > PTT_ACTIVATION_WARNING_MS) {
      if (millis() - lastWarning > PTT_ACTIVATION_WARNING_INTERVAL_MS) {
        morse.print('E');
        lastWarning = millis();
      }
    }
  }

  // control de ptt
  // depende de actividad de audio y "anti-poncho"
  bool ptt = active && activeTime < PTT_ACTIVATION_LIMIT_MS;
  digitalWrite(PTT_PIN, ptt);

  // baliza de identificación
  // activada por tiempo de inactividad / "peteteos"
  if ((millis() - lastBeacon > BEACON_INTERVAL) || (forceBeaconCnt >= 5)) {
    forceBeaconCnt = 0;
    lastBeacon = millis();
    digitalWrite(PTT_PIN, true);
    delay(200);
    morse.print(BEACON_MESSAGE);
    delay(300);
    digitalWrite(PTT_PIN, false);
  }

#ifdef DEBUG
  Serial.print(audioLevel); Serial.print(",");
  Serial.print(active); Serial.print(",");
  Serial.print(activeTime / 1000); Serial.print(",");
  Serial.print(forceBeaconCnt); Serial.print(",");
  Serial.println(ptt);
#endif

}
