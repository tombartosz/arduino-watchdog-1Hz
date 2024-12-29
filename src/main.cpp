#include <Arduino.h>
#include <TaskScheduler.h>


// Konfiguracja pinów
const int inputPin = 2;       // Pin sygnału wejściowego
const int ledPin = 13;        // Pin diody ostrzegawczej (wbudowana dioda na Arduino Nano)
const int relayPin = 3;       // Pin sterujący przekaźnikiem

// Parametry czasowe
const unsigned long SHORT_INTERVAL_MS = 10000; // 10 sekund
const unsigned long LONG_INTERVAL_MS = 60000; // 60 sekund
const unsigned long RELAY_ON_TIME_MS = 15000; // 15 sekund dla przekaźnika
const unsigned long PAUSE_AFTER_RELAY_MS = 15000; // 15 sekund pauzy po przekaźniku
const unsigned long STARTUP_DELAY_MS = 15000; // 15 sekund opóźnienia przy starcie
const unsigned long INIT_DELAY_MS = 5000; // 5 sekund opóźnienia przy starcie programu dla przekaznika

// Zmienna do liczenia impulsów
volatile unsigned int pulseCountShort = 0;
volatile unsigned int pulseCountLong = 0;

const unsigned int LED_ON = 1;
const unsigned int LED_BLINK_SLOW = 2;
const unsigned int LED_BLINK_FAST = 3;
const unsigned int LED_OFF = 4;

int ledMode = LED_OFF;
bool ledState = false;

Scheduler runner;


void warn();
void error();
void nowarn();
void resetProgram();
void toggleLED();
void blinkSlow();
void blinkFast();
void countPulse();

void checkShort();
void checkLong();

bool initRunShort = true;
bool initRunLong = true;

Task taskBlinkLEDSLOW(1000, TASK_FOREVER, &blinkSlow);
Task taskBlinkLEDFAST(250, TASK_FOREVER, &blinkFast);

Task taskShort(5000, TASK_FOREVER, &checkShort);
Task taskLong(30000, TASK_FOREVER, &checkLong);

bool warnRaised = false;
int resetCounter = 10;

void printTime() {
  Serial.print(millis()/1000);
  Serial.print(": ");
}

// Funkcja inicjalizacyjna
void setup() {
  pinMode(inputPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(ledPin, HIGH); 
  digitalWrite(relayPin, LOW);
  delay(INIT_DELAY_MS);

  digitalWrite(ledPin, LOW);   // Wyłącz diodę
  digitalWrite(relayPin, HIGH); // Wyłącz przekaźnik

  Serial.begin(115200); // Ustawienie prędkości komunikacji
  printTime(); Serial.println("Program uruchomiony. Czekam 15 sekund...");
  
  delay(STARTUP_DELAY_MS); // Opóźnienie startu
  printTime(); Serial.println("Rozpoczęcie pracy programu.");
  
  resetProgram(); // Reset programu do stanu początkowego

  runner.addTask(taskBlinkLEDSLOW);
  runner.addTask(taskBlinkLEDFAST);
  runner.addTask(taskShort);
  runner.addTask(taskLong);

  taskBlinkLEDSLOW.enable(); // Włącz zadanie
  taskBlinkLEDFAST.enable();
  taskShort.enable();
  taskLong.enable();

  attachInterrupt(digitalPinToInterrupt(inputPin), countPulse, RISING);

}

void checkShort() {
  if (!initRunShort) {
    if (pulseCountShort < 3 || pulseCountShort > 8) {
      printTime(); Serial.print("Warning. Impulsow: " ); Serial.println(pulseCountShort);
      warn();
    }
    else {
      printTime(); Serial.print("OK Short. Impulsow: " ); Serial.println(pulseCountShort);
      nowarn();
    }
  }
  initRunShort = false;
  noInterrupts();
  pulseCountShort = 0;
  interrupts();
}

void checkLong() {
  if (!initRunLong) {
    if (pulseCountLong < 20 || pulseCountLong > 50) {
      if (warnRaised) {
        printTime(); Serial.print("Error. Impulsow: "); Serial.println(pulseCountLong);      
        error();
      }
    }
    else {
      printTime(); Serial.print("OK Long. Impulsow: "); Serial.println(pulseCountLong);
      nowarn();
    }
  }
  initRunLong = false;
  noInterrupts();
  pulseCountLong = 0;
  interrupts();
}

void blinkSlow() {
  if (ledMode == LED_BLINK_SLOW) {
    toggleLED();
  }
}

void blinkFast() {
  if (ledMode == LED_BLINK_FAST) {
    toggleLED();
  }
}

void toggleLED() {
  if (ledMode == LED_ON) {
    ledState = true;
  }
  else if (ledMode == LED_OFF) {
    ledState = false;
  }
  else {
    ledState = !ledState;           // Zmień stan diody
  }
  digitalWrite(ledPin, ledState); // Ustaw nowy stan diody
}



// Główna pętla
void loop() {
  runner.execute();
  
}

// Funkcja obsługi przerwań (liczenie impulsów)
void countPulse() {
  pulseCountShort++;
  pulseCountLong++;
}

// Funkcje obsługujące różne zdarzenia
void warn() {
  warnRaised = true;
  ledMode = LED_BLINK_FAST;
}

void error() {
  taskShort.disable();
  taskLong.disable();
  resetCounter--;
  if (resetCounter > 0) {
  printTime(); Serial.println("Błąd!");
  ledMode = LED_ON;
  
  // Załącz przekaźnik
  digitalWrite(relayPin, LOW); 
  printTime(); Serial.println("Przekaźnik załączony na 15 sekund...");
  delay(RELAY_ON_TIME_MS); // Poczekaj 15 sekund z włączonym przekaźnikiem
  
  // Wyłącz przekaźnik
  digitalWrite(relayPin, HIGH); 
  printTime(); Serial.println("Przekaźnik wyłączony.");

  // Pauza programu na dodatkowe 15 sekund
  printTime(); Serial.println("Program wstrzymany na 15 sekund...");
  delay(PAUSE_AFTER_RELAY_MS);
  }
  else {
    printTime(); Serial.println("Brak możliwości resetu. Wykorzystano wszystkie.");
  }

  // Reset programu do początkowego stanu
  resetProgram();
  taskShort.enable();
  taskLong.enable();
}

void nowarn() {
 warnRaised = false;
 ledMode = LED_BLINK_SLOW;
}

// Funkcja resetująca program do stanu początkowego
void resetProgram() {
  warnRaised = false;
  noInterrupts(); // Wyłączenie przerwań na czas resetu
  pulseCountShort = 0;
  pulseCountLong = 0;
  interrupts(); // Ponowne włączenie przerwań
  ledMode = LED_BLINK_SLOW;

  digitalWrite(relayPin, HIGH);
  initRunLong = true;
  initRunShort = true;

  printTime(); Serial.println("Program zresetowany do stanu początkowego.");
}