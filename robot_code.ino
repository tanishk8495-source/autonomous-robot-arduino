// Autonomous Robot with Obstacle Detection
// Components: Arduino Uno, HC-SR04, L298N Motor Driver, 2 DC Motors

// ============ PIN DEFINITIONS ============
// Ultrasonic Sensor
#define TRIG_PIN 9
#define ECHO_PIN 10

// L298N Motor Driver
#define IN1 5    // Left Motor Forward
#define IN2 6    // Left Motor Backward
#define IN3 11   // Right Motor Forward
#define IN4 3    // Right Motor Backward
#define ENA 3    // Left Motor Speed (PWM)
#define ENB 11   // Right Motor Speed (PWM)

// Buttons
#define START_BTN 2   // Start Movement Button
#define RETURN_BTN 4  // Return to Origin Button

// ============ VARIABLES ============
int motorSpeed = 150;  // Normal Speed (0-255)
int obstacleDistance = 20;  // cm
long duration;
int distance;

// Store starting position
float startX = 0, startY = 0;
float currentX = 0, currentY = 0;
float currentDirection = 0;  // 0=Forward, 90=Right, 180=Backward, 270=Left

// Direction array: [distance_cm, direction]
struct Direction {
  int distance;
  int turn;  // 0=Straight, 1=Left, -1=Right
} directions[10];

int directionIndex = 0;
bool movementActive = false;
bool returning = false;

// ============ SETUP ============
void setup() {
  Serial.begin(9600);
  
  // Motor Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Sensor Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Button Pins
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(RETURN_BTN, INPUT_PULLUP);
  
  // Interrupts for buttons
  attachInterrupt(digitalPinToInterrupt(START_BTN), startMovement, FALLING);
  attachInterrupt(digitalPinToInterrupt(RETURN_BTN), returnToOrigin, FALLING);
  
  Serial.println("========================================");
  Serial.println("   AUTONOMOUS ROBOT - INITIALIZATION   ");
  Serial.println("========================================");
  Serial.println("\nInstructions:");
  Serial.println("Format: distance(cm),direction");
  Serial.println("Example: 150,0 (150cm straight)");
  Serial.println("Example: 100,1 (100cm then LEFT)");
  Serial.println("Example: 100,-1 (100cm then RIGHT)");
  Serial.println("\nMultiple commands (use semicolon): 150,0;100,1;50,0");
  Serial.println("\nPress START button to begin movement after 10 seconds");
  Serial.println("Press RETURN button to go back to starting position");
  Serial.println("========================================\n");
}

// ============ LOOP ============
void loop() {
  // Check for serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    parseDirections(input);
  }
  
  delay(100);
}

// ============ PARSE DIRECTIONS ============
void parseDirections(String input) {
  directionIndex = 0;
  int lastIndex = 0;
  
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == ';' || i == input.length() - 1) {
      int endIndex = (i == input.length() - 1) ? i + 1 : i;
      String command = input.substring(lastIndex, endIndex);
      
      int commaIndex = command.indexOf(',');
      if (commaIndex > 0) {
        int dist = command.substring(0, commaIndex).toInt();
        int dir = command.substring(commaIndex + 1).toInt();
        
        directions[directionIndex].distance = dist;
        directions[directionIndex].turn = dir;
        directionIndex++;
      }
      lastIndex = i + 1;
    }
  }
  
  Serial.print("✓ Received ");
  Serial.print(directionIndex);
  Serial.println(" directions");
  Serial.println("Waiting 10 seconds before movement...");
  delay(10000);
}

// ============ START MOVEMENT INTERRUPT ============
void startMovement() {
  if (!movementActive && directionIndex > 0) {
    movementActive = true;
    returning = false;
    Serial.println("\n>>> MOVEMENT STARTED <<<\n");
    
    for (int i = 0; i < directionIndex; i++) {
      if (!movementActive) break;
      
      // Move forward for specified distance
      moveForward(directions[i].distance);
      
      // Turn if needed
      if (directions[i].turn == 1) {
        turnLeft();
        currentDirection = (currentDirection + 90) % 360;
      } else if (directions[i].turn == -1) {
        turnRight();
        currentDirection = (currentDirection - 90 + 360) % 360;
      }
      
      delay(500);  // Pause between commands
    }
    
    stopMotors();
    movementActive = false;
    Serial.println("\n>>> MOVEMENT COMPLETED <<<\n");
  }
}

// ============ MOVE FORWARD WITH OBSTACLE DETECTION ============
void moveForward(int targetDistance) {
  int movedDistance = 0;
  
  Serial.print("Moving forward ");
  Serial.print(targetDistance);
  Serial.println("cm...");
  
  while (movedDistance < targetDistance && movementActive) {
    // Check obstacle
    distance = getDistance();
    
    if (distance > 0 && distance < obstacleDistance) {
      Serial.print("⚠ OBSTACLE DETECTED at ");
      Serial.print(distance);
      Serial.println("cm - STOPPING!");
      stopMotors();
      
      // Wait for obstacle to be removed
      while (getDistance() < obstacleDistance + 5) {
        delay(500);
      }
      Serial.println("✓ Obstacle cleared - Resuming...");
    }
    
    // Move motors
    motorForward();
    
    movedDistance += 2;  // Approximate distance per iteration
    delay(100);
  }
  
  stopMotors();
}

// ============ MOTOR CONTROL FUNCTIONS ============
void motorForward() {
  // Left Motor
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, motorSpeed);
  
  // Right Motor
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, motorSpeed);
}

void motorBackward() {
  // Left Motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, motorSpeed);
  
  // Right Motor
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, motorSpeed);
}

void turnLeft() {
  Serial.println("  ↻ Turning LEFT...");
  
  // Left Motor backward, Right Motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, motorSpeed);
  
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, motorSpeed);
  
  delay(600);  // Adjust time for 90-degree turn
  stopMotors();
}

void turnRight() {
  Serial.println("  ↺ Turning RIGHT...");
  
  // Left Motor forward, Right Motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, motorSpeed);
  
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, motorSpeed);
  
  delay(600);  // Adjust time for 90-degree turn
  stopMotors();
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ============ RETURN TO ORIGIN INTERRUPT ============
void returnToOrigin() {
  if (!movementActive) {
    movementActive = true;
    returning = true;
    Serial.println("\n>>> RETURNING TO START <<<\n");
    
    // Simple return: go backward then stop
    motorBackward();
    delay(3000);  // Adjust based on distance traveled
    stopMotors();
    
    currentX = 0;
    currentY = 0;
    movementActive = false;
    returning = false;
    Serial.println(">>> RETURNED TO START <<<\n");
  }
}

// ============ ULTRASONIC SENSOR ============
int getDistance() {
  // Send trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo pulse
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  // Calculate distance (speed of sound = 343 m/s)
  int dist = duration * 0.034 / 2;
  
  return dist;
}
