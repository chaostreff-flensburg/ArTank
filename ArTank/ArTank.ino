#include <NewPing.h>
#include <MSMotorShield.h>

#define PING_PIN  12
#define MAX_DISTANCE 20

MS_DCMotor motor1(3);
MS_DCMotor motor2(4);

NewPing sonar(PING_PIN, PING_PIN, MAX_DISTANCE);

//Kommunikation via Serial
char current = 0;            // Einzelzeichen über serial
char data[15] = "";          // Zeichenpuffer Gesamtdaten
int incount = 0;             // Zähler für Pufferzeiger
bool lineComplete = false;   // Merker ob Zeilenende erreicht wurde

char *X = 0;                 // X-Wert vom Nunchuck aus "data"
int xWert = 0;               // integer X-Wert nach atoi Operation
char *Y = 0;                 // Y-Wert vom Nunchuck aus "data"
int yWert = 0;               // integer Y-Wert nach atoi Operation

// timeout timer
unsigned long timeout;
bool timedout = false;


void setup() {
  motor1.setSpeed(0);
  motor2.setSpeed(0);

  motor1.run(RELEASE);
  motor2.run(RELEASE);

  Serial.begin(115200);
}

void loop() {
  readSerialData();
  checkTimeOut();
  checkDistance();
}

void checkDistance()
{
  delay(50);
  Serial.print("Ping: ");
  Serial.print(sonar.ping_cm());
  Serial.println("cm");
}

void checkTimeOut() {
  if ( !timedout && millis() - timeout > 1000) {
    setTracks(RELEASE, 0, RELEASE, 0);
    Serial.println("timeout");
    timedout = true;
  }
}

void resetTimeOut() {
  timeout = millis();
  timedout = false;
}

void readSerialData() {
  while ( Serial.available() & incount < 15 & !lineComplete )
  {
    current = Serial.read();
    if (current != 42) //ASCII 42 == '*'   // solange Zeichen lesen bis das Sternchen den Datensatz abschließt
    {
      data[incount] = current;
      incount++;
    }
    else
    {
      data[incount] = '\0'; // puffer mit NULL Zeichen abschließen, damit das Ende der Zeichenkette durch string Operationen erkannt wird
      lineComplete = true;
    }
  }

  if (lineComplete)  // Wenn der volle Datensatz gelesen wurde
  {
    daten_auswerten();
    werte_beurteilen();

    reset();
    resetTimeOut();
  }
  else if (incount >= 15)
  {
    reset();
  }
}

void reset() {
  incount = 0;
  lineComplete = false;
}

void daten_auswerten() {
  X = strtok ( data, ";");
  Y = strtok ( NULL, "*");
  xWert = atoi (X);
  yWert = atoi (Y);
}

void werte_beurteilen() {
  /*
    Serial.println(X); //Datenausgabe zur Kontrolle (können abgeschaltet werden)
    Serial.println(xWert);
    Serial.println(Y);
    Serial.println(yWert);
  */


  if (xWert > 0) {
    xWert = constrain(xWert, 200, 255);
    setTrack1(FORWARD, xWert);
  } else if (xWert < 0) {
    xWert = xWert * -1;
    xWert = constrain(xWert, 200, 255);
    setTrack1(BACKWARD, xWert);
  } else if (xWert == 0) {
    setTrack1(RELEASE, 0);
  }

  if (yWert > 0) {
    yWert = constrain(yWert, 200, 255);
    setTrack2(FORWARD, yWert);
  } else if (yWert < 0) {
    yWert = yWert * -1;
    yWert = constrain(yWert, 200, 255);
    setTrack2(BACKWARD, yWert);
  } else if (yWert == 0) {
    setTrack2(RELEASE, 0);
  }
}

void setTracks(int direction1, int speed1, int direction2, int speed2) {
  motor1.setSpeed(speed1);
  motor2.setSpeed(speed2);

  motor1.run(direction1);
  motor2.run(direction2);
}

void setTrack1(int direction1, int speed1) {
  motor1.setSpeed(speed1);
  motor1.run(direction1);
}

void setTrack2(int direction2, int speed2) {
  motor2.setSpeed(speed2);
  motor2.run(direction2);
}
