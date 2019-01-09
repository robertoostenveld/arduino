#include <MIDI.h>

// select "tools"->"usb type"->"serial + MIDI" to instantiate the usbMIDI device
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, inMIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, outMIDI);

// #define DEBUG_NOTE
// #define DEBUG_SERIAL

/************************************************************************************************************/

void handleNoteOff(byte Channel, byte NoteNumber, byte Velocity) {
  outMIDI.sendNoteOn(NoteNumber, Velocity, Channel);
};

void handleNoteOn(byte Channel, byte NoteNumber, byte Velocity) {
  outMIDI.sendNoteOff(NoteNumber, Velocity, Channel);
};

void handleAfterTouchPoly(byte Channel, byte NoteNumber, byte Pressure) {
  outMIDI.sendPolyPressure(NoteNumber, Pressure, Channel);
};

void handleControlChange(byte Channel, byte ControlNumber, byte ControlValue) {
  outMIDI.sendControlChange(ControlNumber, ControlValue, Channel);
};

void handleProgramChange(byte Channel, byte ProgramNumber) {
  outMIDI.sendProgramChange(ProgramNumber, Channel);
};

void handleAfterTouchChannel(byte Channel, byte Pressure) {
  outMIDI.sendAfterTouch(Pressure, Channel);
};

void handlePitchBend(byte Channel, int PitchValue) {
  outMIDI.sendPitchBend(PitchValue, Channel);
};

void handleSystemExclusive(byte* Array, unsigned Size) {
  outMIDI.sendSysEx(Size, Array);
};

void handleTimeCodeQuarterFrame(byte Data) {
  outMIDI.sendTimeCodeQuarterFrame(Data);
};

void handleSongPosition(unsigned int Beats) {
  outMIDI.sendSongPosition(Beats);
};

void handleSongSelect(byte SongNumber) {
  outMIDI.sendSongSelect(SongNumber);
};

void handleTuneRequest(void) {
  outMIDI.sendTuneRequest();
};

void handleClock(void) {
  outMIDI.sendRealTime(midi::Clock);
};

void handleStart(void) {
  outMIDI.sendRealTime(midi::Start);
};

void handleContinue(void) {
  outMIDI.sendRealTime(midi::Continue);
};

void handleStop(void) {
  outMIDI.sendRealTime(midi::Stop);
};

void handleActiveSensing(void) {
  outMIDI.sendRealTime(midi::ActiveSensing);
};

void handleSystemReset(void) {
  outMIDI.sendRealTime(midi::SystemReset);
};

/************************************************************************************************************/

long prev = 0;
const int led = 13;

#ifdef DEBUG_NOTE
const int channel = 1;
const int note = 42;
const int velocity = 127;
#endif

/************************************************************************************************************/

void setup() {

  pinMode(led, OUTPUT);

  outMIDI.begin(MIDI_CHANNEL_OMNI);
  inMIDI.begin(MIDI_CHANNEL_OMNI);

  inMIDI.setHandleNoteOff(handleNoteOff);
  inMIDI.setHandleNoteOn(handleNoteOn);
  inMIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  inMIDI.setHandleControlChange(handleControlChange);
  inMIDI.setHandleProgramChange(handleProgramChange);
  inMIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  inMIDI.setHandlePitchBend(handlePitchBend);
  inMIDI.setHandleSystemExclusive(handleSystemExclusive);
  inMIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  inMIDI.setHandleSongPosition(handleSongPosition);
  inMIDI.setHandleSongSelect(handleSongSelect);
  inMIDI.setHandleTuneRequest(handleTuneRequest);
  //inMIDI.setHandleClock(handleClock);
  inMIDI.setHandleStart(handleStart);
  inMIDI.setHandleContinue(handleContinue);
  inMIDI.setHandleStop(handleStop);
  inMIDI.setHandleActiveSensing(handleActiveSensing);
  inMIDI.setHandleSystemReset(handleSystemReset);
}

/************************************************************************************************************/

void loop() {

  if ((millis() - prev) > 1000) {
    digitalWrite(led, !digitalRead(led));

#ifdef DEBUG_NOTE
    // send a MIDI note every second
    outMIDI.sendNoteOn(note, velocity, channel);
    usbMIDI.sendNoteOn(note, velocity, channel);
#endif
    prev = millis();
  }

  while (inMIDI.read()) {
#ifdef DEBUG_SERIAL
    Serial.print(inMIDI.getType());
    Serial.print(" ");
    switch (inMIDI.getType()) {
      case midi::ActiveSensing:
        Serial.println("ActiveSensing");
        break;
      case midi::AfterTouchChannel:
        Serial.println("AfterTouchChannel");
        break;
      case midi::AfterTouchPoly:
        Serial.println("AfterTouchPoly");
        break;
      case midi::Clock:
        Serial.println("Clock");
        break;
      case midi::Continue:
        Serial.println("Continue");
        break;
      case midi::ControlChange:
        Serial.println("ControlChange");
        break;
      case midi::InvalidType:
        Serial.println("InvalidType");
        break;
      case midi::NoteOff:
        Serial.println("NoteOff");
        break;
      case midi::NoteOn:
        Serial.println("NoteOn");
        break;
      case midi::PitchBend:
        Serial.println("PitchBend");
        break;
      case midi::ProgramChange:
        Serial.println("ProgramChange");
        break;
      case midi::SongPosition:
        Serial.println("SongPosition");
        break;
      case midi::SongSelect:
        Serial.println("SongSelect");
        break;
      case midi::Start:
        Serial.println("Start");
        break;
      case midi::Stop:
        Serial.println("Stop");
        break;
      case midi::SystemExclusive:
        Serial.println("SystemExclusive");
        break;
      case midi::SystemReset:
        Serial.println("SystemReset");
        break;
      case midi::TimeCodeQuarterFrame:
        Serial.println("TimeCodeQuarterFrame");
        break;
      case midi::TuneRequest:
        Serial.println("TuneRequest");
      default:
        Serial.println("Other");
        break;
    }
#endif
  }
}
