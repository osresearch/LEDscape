/** \file
 * Teensy 2.0 firmware to read four analog knobs and print
 * their values periodically.
 */

#define LED_PIN 11

void setup(void)
{
	Serial.begin(9600);
	pinMode(LED_PIN, OUTPUT);
}

void loop(void)
{
	digitalWrite(LED_PIN, HIGH);
	uint16_t p1 = analogRead(A3);
	uint16_t p2 = analogRead(A2);
	uint16_t p3 = analogRead(A5);
	uint16_t p4 = analogRead(A4);
	digitalWrite(LED_PIN, LOW);

	Serial.print(p1);
	Serial.print(' ');
	Serial.print(p2);
	Serial.print(' ');
	Serial.print(p3);
	Serial.print(' ');
	Serial.println(p4);

	delay(100);
}
