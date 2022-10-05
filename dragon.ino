/*
   Smoke-breathing Dragon

   Uses HC-SCO4 ultrasonic sensor to determine if a child's
   hand is reaching into the Dragon's mouth to steal his precious
   CANDY.
   The Dragon's eyes will light up immediately.
   If the hand persists, the Dragon's fire will glow red, then orange, then smoke will be released
   If somehow the hand is detected for more than 30 seconds, everything is reset.

   The ultrasonic circuit:
   Module HR-SC04 (four pins) or PING))) (and other with
     three pins), attached to digital pins as follows:
   ---------------------
   | HC-SC04 | Arduino |
   ---------------------
   |   Vcc   |   5V    |
   |   Trig  |   12    |
   |   Echo  |   13    |
   |   Gnd   |   GND   |
   ---------------------

   By default, the distance returned by the read()
   method is in centimeters. To get the distance in inches,
   pass INC as a parameter.
   Example: ultrasonic.read(INC)

   Example code stolen with love from Erick Simões (github: @ErickSimoes | twitter: @AloErickSimoes)
*/
#define PIN_EYES 5
#define PIN_SMOKE 6
#define PIN_TRIG 12
#define PIN_ECHO 13
// if the kid is too close, i.e. their distance is less than this number, then BAD THINGS happen.
#define DIST_TOO_CLOSE 50

// powers the distance sensor
#include <Ultrasonic.h>

#include <Adafruit_NeoPixel.h>
#define PIN_FIRE 2

// How many NeoPixels are attached to the Arduino?
//#define NUMPIXELS 24 // Popular NeoPixel ring size
#define NUMPIXELS 60 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_FIRE, NEO_GRB + NEO_KHZ800);

/*
   Pass as a parameter the trigger and echo pin, respectively,
   or only the signal pin (for sensors 3 pins), like:
   Ultrasonic ultrasonic(13);
*/
Ultrasonic ultrasonic(PIN_TRIG, PIN_ECHO);
int distance = 1;
int previousD = 300;
int level = 0;
int iterationsTilDeescalation = 20;

// number of milliseconds to wait between active iterations of the loop()
int desiredHertz = 30; // will probably do 20Hz
double loopInterval = 1000 / desiredHertz; // 50ms iterations mean 20Hz adjustments

const unsigned int levels[10][3] = {{0, 0, 0}, {32, 0, 0}, {48, 0, 0}, {96, 32, 0}, {128, 64, 0}, {128, 92, 32}, {192, 128, 32}, {192, 192, 164}, {200, 200, 200}, {255, 255, 255} };
// rather than sequentially light the LEDs (i.e. in a circle), do them pseudo-randomly.
const unsigned int ledSequence[24] = {
  0, 3, 6, 9, 12, 15, 18, 21,
  8, 11, 14, 17, 20, 23, 2, 5,
  4, 7, 10, 13, 16, 19, 22, 1
};

double nowTime = 0;
unsigned int previousTime = 0;
unsigned int lastEscalationTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_EYES, OUTPUT);
  Serial.println("testing PIN_EYES");
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_EYES, HIGH);
    delay(100);
    digitalWrite(PIN_EYES, LOW);
    delay(200);
  }

  pinMode(PIN_SMOKE, OUTPUT);
  /*
    Serial.println("testing PIN_SMOKE");
    for (int i = 0; i < 5; i++) {
      digitalWrite(PIN_SMOKE, HIGH);
      delay(100);
      digitalWrite(PIN_SMOKE, LOW);
      delay(900);
    }
  */

  Serial.println("Setting up NeoPixels");
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(64, 0, 0));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 64, 0));
    pixels.show();
    delay(10);
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 64));
    pixels.show();
    delay(10);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(ledSequence[i], pixels.Color(127, 127, 127));
    pixels.show();
    delay(10);
  }
  previousTime = 0;
}

void loop() {
  static int d = 0;

  // ensure that the loop only does anything at
  // at predictable intervals - i.e., since the pumps
  // work on 60Hz AC there's no point in updating the state
  // of the pump switch any faster than that.
  // also there's a latency as the mechanical relays move, so again
  // no point in toggling power to the pumps at faster than perhaps 20Hz
  // However, if the previous loop detected a zero distance, try again immediately
  nowTime = millis();
  if (nowTime <= previousTime + loopInterval)
  {
    return;
  }
  // ... else, run all the previous loop's updates
  previousTime = nowTime - ((int) nowTime % (int) loopInterval);

  // TODO: run all the updates first. Then we measure distance again.
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  //  pixels.clear(); // Set all pixel colors to 'off'

  doUpdates();

  // If the escalation level has been high enough, long enough, it's time to de-escalate
  if (iterationsTilDeescalation * loopInterval + lastEscalationTime < nowTime) {
    deescalate();
  }

  // measure the distance and decide whether to escalate output signals
  static int tries = 1;
  do {
    d = measure();
  } while (d == 0 && tries++ < 20);
  if (tries >= 20) {
    Serial.println("20 failures??");
  }
  tries = 1;
  if (d < DIST_TOO_CLOSE && previousD < DIST_TOO_CLOSE) {
    Serial.println("Distance is too close. Time to Escalate!!");
    escalate();
    escalate();
  }
  previousD = d;
}

int measure() {
  distance = ultrasonic.read();
  Serial.print("Distance in CM: ");
  Serial.println(distance);
  Serial.print("Previous in CM: ");
  Serial.println(previousD);
  return distance;
}

void escalate() {
  level++;
  level = min(level, 9);
  //  lastEscalationTime = millis();
  lastEscalationTime = nowTime;
}

void deescalate() {
  level--;
  level = max(level, 0);
}

void doUpdates() {
  lightEyes(shouldLightEyes());
  lightFire(level);
  if (getSmokeLevel()) {
    // decide if smoke has been on a large-enough percentage of the time yet.
  }
}

bool shouldLightEyes() {
  return level > 0;
}

bool shouldLightFire() {
  return level > 1;
}

float getSmokeLevel() {
  return level / 60;
}

void lightEyes(bool b) {
  digitalWrite(PIN_EYES, b ? HIGH : LOW);
}

void lightFire(int l) {
  Serial.print("Level is ");
  Serial.println(l);
  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(ledSequence[i], pixels.Color(levels[level][0], levels[level][1], levels[level][2]));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(5); // Pause before next pass through loop
  }
}
