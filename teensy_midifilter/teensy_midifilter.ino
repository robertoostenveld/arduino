#include <MIDI.h>

// select "tools"->"usb type"->"serial + MIDI" to instantiate the usbMIDI device
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, serial1MIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, serial2MIDI);

/************************************************************************************************************/

void handleNoteOff(byte Channel, byte NoteNumber, byte Velocity) {
  serial2MIDI.sendNoteOn (NoteNumber, Velocity, Channel);
};

void handleNoteOn(byte Channel, byte NoteNumber, byte Velocity) {
  serial2MIDI.sendNoteOff(NoteNumber, Velocity, Channel);
};

void handleAfterTouchPoly(byte Channel, byte NoteNumber, byte Pressure) {
  serial2MIDI.sendPolyPressure(NoteNumber, Pressure, Channel);
};

void handleControlChange(byte Channel, byte ControlNumber, byte ControlValue) {
  serial2MIDI.sendControlChange(ControlNumber, ControlValue, Channel);
};

void handleProgramChange(byte Channel, byte ProgramNumber) {
  serial2MIDI.sendProgramChange(ProgramNumber, Channel);
};

void handleAfterTouchChannel(byte Channel, byte Pressure) {
  serial2MIDI.sendAfterTouch(Pressure, Channel);
};

void handlePitchBend(byte Channel, int PitchValue) {
  serial2MIDI.sendPitchBend (PitchValue, Channel);
};

void handleSystemExclusive(byte* Array, unsigned Size) {
  serial2MIDI.sendSysEx (Size, Array);
};

void handleTimeCodeQuarterFrame(byte Data) {
  serial2MIDI.sendTimeCodeQuarterFrame(Data);
};

void handleSongPosition(unsigned int Beats) {
  serial2MIDI.sendSongPosition (Beats);
};

void handleSongSelect(byte SongNumber) {
  serial2MIDI.sendSongSelect(SongNumber);
};

void handleTuneRequest(void) {
  serial2MIDI.sendTuneRequest ();
};

void handleClock(void) {
  serial2MIDI.sendRealTime(midi::Clock);
};

void handleStart(void) {
  serial2MIDI.sendRealTime(midi::Start);
};

void handleContinue(void) {
  serial2MIDI.sendRealTime(midi::Continue);
};

void handleStop(void) {
  serial2MIDI.sendRealTime(midi::Stop);
};

void handleActiveSensing(void) {
  serial2MIDI.sendRealTime(midi::ActiveSensing);
};

void handleSystemReset(void) {
  serial2MIDI.sendRealTime(midi::SystemReset);
};

/************************************************************************************************************/

const int channel = 1;
int cable = 0;
int note = 42;
int velocity = 127;
long prev = 0;
int led = 13;

/************************************************************************************************************/

void setup() {

  pinMode(led, OUTPUT);
  serial1MIDI.begin();
  serial2MIDI.begin();

  serial1MIDI.setHandleNoteOff(handleNoteOff);
  serial1MIDI.setHandleNoteOn(handleNoteOn);
  serial1MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  serial1MIDI.setHandleControlChange(handleControlChange);
  serial1MIDI.setHandleProgramChange(handleProgramChange);
  serial1MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  serial1MIDI.setHandlePitchBend(handlePitchBend);
  serial1MIDI.setHandleSystemExclusive(handleSystemExclusive);
  serial1MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  serial1MIDI.setHandleSongPosition(handleSongPosition);
  serial1MIDI.setHandleSongSelect(handleSongSelect);
  serial1MIDI.setHandleTuneRequest(handleTuneRequest);
  serial1MIDI.setHandleClock(handleClock);
  serial1MIDI.setHandleStart(handleStart);
  serial1MIDI.setHandleContinue(handleContinue);
  serial1MIDI.setHandleStop(handleStop);
  serial1MIDI.setHandleActiveSensing(handleActiveSensing);
  serial1MIDI.setHandleSystemReset(handleSystemReset);
}

/************************************************************************************************************/

void loop() {

  if ((millis() - prev) > 1000) {
    digitalWrite(led, HIGH);
    serial2MIDI.sendNoteOn(note, velocity, channel);
    usbMIDI.sendNoteOn(note, velocity, channel);
    Serial.println("note");
    prev = millis();

    delay(100);
    digitalWrite(led, LOW);
  }

  while (serial1MIDI.read()) {
    // ignore incoming messages
  }
}
