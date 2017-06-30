#include <NewPing.h>
#include <MSMotorShield.h>

#define DO_DEBUG false

#define PING_PIN_0 A0
#define PING_PIN_1 A1
#define PING_PIN_2 A2
#define PING_PIN_3 A3

#define SONAR_NUM 2
#define PING_INTERVAL 50
#define MAX_DISTANCE 60

#define COLLISION_DISTANCE 35

unsigned long pingTimer[SONAR_NUM];
unsigned int cm[SONAR_NUM];
uint8_t currentSensor = 0;

NewPing sonar[SONAR_NUM] = {
  NewPing(PING_PIN_0, PING_PIN_0, MAX_DISTANCE),      // FRONT SENSOR
  //NewPing(PING_PIN_1, PING_PIN_1, MAX_DISTANCE),
  NewPing(PING_PIN_2, PING_PIN_2, MAX_DISTANCE)       // BACK SENSOR
  //NewPing(PING_PIN_3, PING_PIN_3, MAX_DISTANCE)
};


MS_DCMotor motor1(3);
MS_DCMotor motor2(4);

int forwardSpeed = 255;
int turnSpeed = 255;

unsigned long motorTimer = 0;
const long turnInterval = 3500;
boolean doTurn = false;
boolean turning = false;
boolean doStop = false;

void setup() {

  // SETUP MOTORS TO DO NOTHING, YET
  setTracks(RELEASE, 0, RELEASE, 0);


  // SETUP SONAR
  pingTimer[0] = millis() + 75;

  for (uint8_t i = 1; i < SONAR_NUM; i++) {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }


  // START SERIAL COMMUNICATION only if do_debug is true
  if ( DO_DEBUG ) {
    Serial.begin(115200);
  }
  
}

void loop() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {                 // Loop through all the sensors
    if (millis() >= pingTimer[i]) {                         // Is it this sensor's time to ping?
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;            // Set next time this sensor will be pinged
      
      if (i == 0 && currentSensor == SONAR_NUM - 1) {
        oneSensorCycle();                                   // Sensor ping cycle complete, do something with the results
      }

      sonar[currentSensor].timer_stop();                    // Make sure previous timer is canceled before starting a new ping (insurance)
      currentSensor = i;                                    // Sensor being accessed
      cm[currentSensor] = 0;                                // Make distance zero in case there's no ping echo for this sensor
      sonar[currentSensor].ping_timer(echoCheck);           // Do the ping (processing continues, interrupt will call echoCheck to look for echo)
    }
  }

  
  // MOVEMENT LOGIC

  if ( !turning && doTurn ) {
    motorTimer = millis();
    setTracks(FORWARD, turnSpeed, BACKWARD, turnSpeed);
    turning = true;
    doTurn = false;
  }

  if ( turning && millis() - motorTimer >= turnInterval) {
    setTracks(FORWARD, forwardSpeed, FORWARD, forwardSpeed);
    turning = false;
  }

  if ( doStop ) {
    setTracks(RELEASE, 0, RELEASE, 0);
  }
  
}


// If ping received, set the sensor distance to array.
void echoCheck() {
  if (sonar[currentSensor].check_timer()) {
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;
  }
}


// Sensor ping cycle complete, do something with the results.
void oneSensorCycle() {

  if ( DO_DEBUG ) {
    // The following code would be replaced with your code that does something with the ping results.
    for (uint8_t i = 0; i < SONAR_NUM; i++) {
      Serial.print(i);
      Serial.print("=");
      Serial.print(cm[i]);
      Serial.print("cm ");
    }
    Serial.println();
  }

  /*
  if (cm[1] > 0 && cm[1] < COLLISION_DISTANCE && !turning) {
    doTurn = true;
  }
  */

  if (cm[0] > 0 && cm[0] < COLLISION_DISTANCE && !turning) {
    doTurn = true;
  }


  // BACK SENSOR on TOP, so stop when hand over sensor
  if (cm[1] > 0 && cm[1] < COLLISION_DISTANCE) {
    doTurn = false;
    doStop = true;
  } else {
    doStop = false;
  }
  
}

void setTracks(int direction1, int speed1, int direction2, int speed2) {
  motor1.setSpeed(speed1);
  motor2.setSpeed(speed2);
  
  motor1.run(direction1);
  motor2.run(direction2);
}


