#include <Wire.h>
#include <LiquidCrystal_I2C.h> // LCD display library
#include <Servo.h>             // Servo motor library

// Pin and device configuration
#define SERVO_PIN 9            // Servo motor control pin
#define PING_PIN 7             // Ultrasonic sensor trigger/echo pin
#define LCD_ADDRESS 0x20       // I2C address of the LCD
#define LCD_COLS 16            // LCD width
#define LCD_ROWS 2             // LCD height

// Bowl distance thresholds (in cm)
#define BOWL_FULL_THRESHOLD 30
#define BOWL_EMPTY_THRESHOLD 150

// Timer and feeding settings
#define TIMER_TICKS_100MS 100          // 100 timer ticks = 1 second (assuming 1ms per tick)
#define FEEDING_DURATION 2000          // Time to keep servo open (2 seconds)
#define PULSE_TIMEOUT 30000            // Timeout for ultrasonic pulse

// Servo positions
#define SERVO_CLOSED 0
#define SERVO_OPEN 90

// Timer2 configuration for 1kHz interrupts (every 1ms)
#define TIMER2_OCR_VALUE 249           // Compare match value
#define TIMER2_PRESCALER_64 (1 << CS22)

Servo feederServo;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// Shared state
volatile bool triggerFeed = false;     // Flag set by timer ISR when distance needs updating
volatile long measuredDistance = 0;    // Distance read from ultrasonic sensor
long previousDistance = -1;            // Last measured distance
bool feedingState = false;             // Whether we’re currently feeding

void setup() {
  Serial.begin(9600);

  // Setup servo
  feederServo.attach(SERVO_PIN);
  feederServo.write(SERVO_CLOSED);     // Start with servo in closed position

  // Setup LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder Ready");

  // Set PING_PIN as output low (initial state)
  DDRD |= (1 << DDD7);     // Set pin 7 as output
  PORTD &= ~(1 << PORTD7); // Set pin 7 LOW

  setupTimer2(); // Start the timer for periodic updates
}

void loop() {
  // Always keep servo closed unless feeding
  if (!feedingState) {
    feederServo.write(SERVO_CLOSED);
  }

  // If timer ISR signaled a distance update
  if (triggerFeed) {
    triggerFeed = false;
    processDistanceUpdate();
  }
}

// Handles logic after new distance is measured
void processDistanceUpdate() {
  // If distance has changed or feeding state needs to change
  if (distanceChanged() || feedingStateChanged()) {
    previousDistance = measuredDistance;
    feedingState = (measuredDistance > BOWL_EMPTY_THRESHOLD);
    updateDisplay(); // Show updated state
  }
}

bool distanceChanged() {
  return measuredDistance != previousDistance;
}

bool feedingStateChanged() {
  return feedingState != (measuredDistance > BOWL_EMPTY_THRESHOLD);
}

// Updates the LCD based on the distance and feeding status
void updateDisplay() {
  displayDistance();

  if (feedingState) {
    handleFeeding();
  } else if (measuredDistance < BOWL_FULL_THRESHOLD) {
    displayFullBowl();
  } else {
    displayPartialBowl();
  }
}

// Shows the distance on the LCD
void displayDistance() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance: ");
  lcd.print(measuredDistance);
  lcd.print("cm");
}

// Activates the servo to dispense food
void handleFeeding() {
  lcd.setCursor(0, 1);
  lcd.print("Feeding...");
  Serial.println("FEEDING");
  feederServo.write(SERVO_OPEN);
  delay(FEEDING_DURATION);  // Wait with servo open
}

// Shows "FULL" message on LCD
void displayFullBowl() {
  Serial.println("FULL");
  lcd.setCursor(0, 1);
  lcd.print("FULL");
}

// Displays a progress bar based on bowl fill level
void displayPartialBowl() {
  int maxDots = LCD_COLS; // Full width of LCD row
  int dotsCount = map(measuredDistance, 0, BOWL_EMPTY_THRESHOLD, maxDots, 0);
  dotsCount = constrain(dotsCount, 0, maxDots); // Clamp to 0–16

  lcd.setCursor(0, 1);
  for (int i = 0; i < dotsCount; i++) {
    lcd.print(".");
  }
  for (int i = dotsCount; i < maxDots; i++) {
    lcd.print(" ");
  }

  int percentFull = (dotsCount * 100) / maxDots;
  Serial.println(String(percentFull) + "% FULL");
}

// Setup Timer2 to trigger interrupt every 1ms
void setupTimer2() {
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  OCR2A = TIMER2_OCR_VALUE;        // Compare match at 249
  TCCR2A |= (1 << WGM21);          // CTC mode
  TCCR2B |= TIMER2_PRESCALER_64;   // Prescaler = 64
  TIMSK2 |= (1 << OCIE2A);         // Enable compare match interrupt
}

volatile unsigned int tickCount = 0;

// Timer2 interrupt service routine – runs every 1ms
ISR(TIMER2_COMPA_vect) {
  tickCount++;
  if (tickCount >= TIMER_TICKS_100MS) {  // Every 100ms
    tickCount = 0;
    measuredDistance = readPing();       // Read new distance
    triggerFeed = true;                  // Signal loop to update logic
  }
}

// Sends ultrasonic pulse and reads echo to measure distance
long readPing() {
  // Trigger ultrasonic pulse (pin 7)
  DDRD |= (1 << DDD7);       // Output mode
  PORTD &= ~(1 << PORTD7);   // LOW
  delayMicroseconds(2);
  PORTD |= (1 << PORTD7);    // HIGH for 10us
  delayMicroseconds(10);
  PORTD &= ~(1 << PORTD7);   // LOW again

  DDRD &= ~(1 << DDD7);      // Switch to input
  PORTD &= ~(1 << PORTD7);   // Disable pull-up

  // Read echo duration
  unsigned long duration = pulseIn(PING_PIN, HIGH, PULSE_TIMEOUT);

  // Convert duration to distance in cm
  long distance = duration / 58;
  return distance;
}
