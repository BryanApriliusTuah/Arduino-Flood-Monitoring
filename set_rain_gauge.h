const int pin_interrupt = 3; 
volatile int tip_count = 0;
float curah_hujan = 0.00;
float milimeter_per_tip = 0.30;
volatile unsigned long last_interrupt_time = 0;

unsigned long last_reset_time = 0;
const unsigned long RESET_INTERVAL = 24UL * 60UL * 60UL * 1000UL;  // 24 jam dalam ms

void ICACHE_RAM_ATTR tipping(){
  unsigned long now = millis();
  if (now - last_interrupt_time > 200) {
    tip_count++;
    last_interrupt_time = now;
  }
}

void init_rain_gauge(){
  pinMode(pin_interrupt, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_interrupt), tipping, FALLING);
}

float read_rainfall(){
  noInterrupts();
  int tips = tip_count;
  tip_count = 0;
  interrupts();
  curah_hujan += tips * milimeter_per_tip;
  return curah_hujan;
}

void check_and_reset_rainfall() {
  unsigned long now = millis();
  if (now - last_reset_time >= RESET_INTERVAL) {
    noInterrupts();
    curah_hujan = 0.00;
    tip_count = 0;
    interrupts();
    last_reset_time = now;
    Serial.println("[RainGauge] Curah hujan telah direset setelah 1 hari");
  }
}

