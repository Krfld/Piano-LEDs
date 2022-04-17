#include <Arduino.h>
#include <BLEMidi.h>
#include <Adafruit_NeoPixel.h>

bool SHIFT = false;
uint8_t LOWEST_OCTAVE = 3;

#define PIN 15
#define NUM_LEDS 60

#define LOWEST_NOTE 12 * LOWEST_OCTAVE

#define PEDAL_CONTROLLER 64
#define BRIGHTNESS_CONTROLLER 63

Adafruit_NeoPixel leds(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

#define WHITE_KEYS leds.Color(0, 255, 255)
#define BLACK_KEYS leds.Color(255, 0, 255)
#define LEDS_ON leds.Color(255, 255, 255)
#define LEDS_OFF leds.Color(0, 0, 0)

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);
void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);
void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp);

bool suspended = false;

void setup()
{
	log_i("Setup");

	BLEMidiServer.begin("ESP32");
	BLEMidiServer.setOnConnectCallback([]()
									   { log_i("Connected"); });
	BLEMidiServer.setOnDisconnectCallback([]()
										  { log_i("Disconnected"); });
	BLEMidiServer.setNoteOnCallback(onNoteOn);
	BLEMidiServer.setNoteOffCallback(onNoteOff);
	BLEMidiServer.setControlChangeCallback(onControlChange);
	// BLEMidiServer.enableDebugging();

	leds.begin();
	leds.clear();
	leds.setBrightness(255);
	leds.show();

	log_i("Ready");
}

void loop()
{
	uint8_t *pixels = leds.getPixels();
	for (int i = 0; i < NUM_LEDS * 3; i++)
		if (pixels[i] != 0)
			pixels[i] *= 0.95;
	leds.show();
	delay(25);
}

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
	// log_i("Note on: %d, %d, %d", channel, note, velocity);

	if (channel != 0)
	{
		if (note == 50 && LOWEST_OCTAVE > 0)
			LOWEST_OCTAVE--;
		else if (note == 36 && LOWEST_OCTAVE < 6)
			LOWEST_OCTAVE++;
		else if (note == 46)
			SHIFT = !SHIFT;
		return;
	}

	switch (note % 12)
	{
	case 0:
	case 2:
	case 4:
	case 5:
	case 7:
	case 9:
	case 11:
		leds.setPixelColor(note - LOWEST_NOTE - (SHIFT ? 1 : 0), leds.Color(0, velocity * 2, velocity * 2));
		break;

	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		leds.setPixelColor(note - LOWEST_NOTE - (SHIFT ? 1 : 0), leds.Color(velocity * 2, 0, velocity * 2));
		break;

	default:
		break;
	}

	leds.show();
}

void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
	if (suspended)
		return;

	leds.setPixelColor(note - LOWEST_NOTE - (SHIFT ? 1 : 0), LEDS_OFF);
	leds.show();
}

void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp)
{
	// log_i("Control change: %d, %d, %d", channel, controller, value);

	switch (controller)
	{
	case PEDAL_CONTROLLER:
		if (value)
			suspended = true;
		else
		{
			suspended = false;
			leds.clear();
			leds.show();
		}
		break;

	case BRIGHTNESS_CONTROLLER:
		leds.setBrightness(value * 2);
		leds.show();

	default:
		break;
	}
}