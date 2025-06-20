#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define SERVO_PIN 9
#define PING_PIN 7
#define LCD_ADDRESS 0x20
#define LCD_COLS 16
#define LCD_ROWS 2

#define BOWL_FULL_THRESHOLD 30
#define BOWL_EMPTY_THRESHOLD 150

#define TIMER_TICKS_100MS 100
#define FEEDING_DURATION 2000
#define PULSE_TIMEOUT 30000

#define SERVO_CLOSED 0
#define SERVO_OPEN 90

#define TIMER2_OCR_VALUE 249  // 16MHz / (64 * 1000Hz) - 1
#define TIMER2_PRESCALER_64 (1 << CS22)

Servo feederServo;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

volatile bool triggerFeed = false;
volatile long measuredDistance = 0;
long previousDistance = -1;
bool feedingState = false;


void setup() {
  Serial.begin(9600);
  
  feederServo.attach(SERVO_PIN);
  feederServo.write(SERVO_CLOSED);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder Ready");
  
  DDRD |= (1 << DDD7);
  PORTD &= ~(1 << PORTD7); 
  
  setupTimer2();
}

void loop() {
   if (!feedingState) {
    feederServo.write(SERVO_CLOSED);
    lcd.setCursor(0, 1);
  }
  
  if (triggerFeed) {
    triggerFeed = false;
    
    if (measuredDistance != previousDistance || feedingState != (measuredDistance > BOWL_EMPTY_THRESHOLD)) {
      previousDistance = measuredDistance;
      feedingState = (measuredDistance > BOWL_EMPTY_THRESHOLD);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Distance: ");
      lcd.print(measuredDistance);
      lcd.print("cm");
      
      if (feedingState) {
        lcd.setCursor(0, 1);
        lcd.print("Feeding...");
        Serial.println("FEEDING");
        feederServo.write(SERVO_OPEN);
        delay(FEEDING_DURATION);
      } else {
        if (measuredDistance < BOWL_FULL_THRESHOLD){
          Serial.println("FULL");
          lcd.setCursor(0, 1);
      	  lcd.print("FULL");
        }
        if (measuredDistance > BOWL_FULL_THRESHOLD) {
          int maxDots = LCD_COLS;
          int dotsCount = map(measuredDistance, 0, BOWL_EMPTY_THRESHOLD, maxDots, 0);
          dotsCount = constrain(dotsCount, 0, maxDots);
          
          lcd.setCursor(0, 1);
          for (int i = 0; i < dotsCount; i++) {
            lcd.print(".");
          }
          int percentFull = (dotsCount * 100) / 14;
		  Serial.println(String(percentFull) + "% FULL");
          for (int i = dotsCount; i < maxDots; i++) {
            lcd.print(" ");
          }
        }
      }
    }
  }
}

void setupTimer2() {
  
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  
  OCR2A = TIMER2_OCR_VALUE;
  
  TCCR2A |= (1 << WGM21);
  
  TCCR2B |= TIMER2_PRESCALER_64;
  
  TIMSK2 |= (1 << OCIE2A);
  
}

volatile unsigned int tickCount = 0;

ISR(TIMER2_COMPA_vect) {
  tickCount++;
  if (tickCount >= TIMER_TICKS_100MS) {
    tickCount = 0;
    measuredDistance = readPing();
    triggerFeed = true;
  }
}

long readPing() {
  DDRD |= (1 << DDD7);
  PORTD &= ~(1 << PORTD7);
  delayMicroseconds(2);
  
  PORTD |= (1 << PORTD7);
  delayMicroseconds(10);
  
  PORTD &= ~(1 << PORTD7);
  
  DDRD &= ~(1 << DDD7);
  PORTD &= ~(1 << PORTD7);
  
  unsigned long duration = pulseIn(PING_PIN, HIGH, PULSE_TIMEOUT);
  long distance = duration / 58;
  
  return distance;
}