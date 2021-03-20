// Copyright (c) Kodi Studios 2021.
// Licensed under the MIT license.

// REQUIREMENTS!
// Add following lib file to C++ Linker:
// winmm.lib
//
// For example, in Visual Studio:
// Project > Properties > Configuration Properties > Linker > Input > Additional Dependencies
// Add: winmm.lib

#include <Windows.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

class MidiException : public std::exception
{
public:
	explicit MidiException(const char* message)
		: std::exception(message)
	{}
};

// Helper Macro
// If Midi Api returns error, throws exception
#define VerifyMidi(midiFuncAndParams)                              \
{                                                                  \
	MMRESULT midiFuncResult;                                       \
	midiFuncResult = midiFuncAndParams;                            \
	if (midiFuncResult != MMSYSERR_NOERROR)                        \
	{                                                              \
		std::stringstream s;                                       \
		s << "Midi Error: " << midiFuncResult;                     \
		throw MidiException(s.str().c_str());                     \
	}                                                              \
}

// Helper Function
// If value falls out of limit, throws exception
void VerifyLimit(uint32_t currentValue, uint32_t maxValue, const char* valueName)
{
	if (currentValue > maxValue)
	{
		std::stringstream s;
		s << valueName << " Current: " << currentValue << " Max: " << maxValue;
		throw std::invalid_argument(s.str().c_str());
	}
}

// Midi message is 4 bytes
// Windows Midi midiOutShortMsg Api passes
// those 4 bytes as DWORD type.
// Use Union structure to easily overlap DWORD onto 
// Midi's 4 bytes.
union MidiData {
	BYTE bData[4];
	DWORD dwData{ 0 }; // Note: this zeros out all 4 bytes in bData array
};

void SetMidiInstrument(
	HMIDIOUT hMidiOut,
	uint8_t channel,       // 4 bits, 0 to 15
	uint8_t instrument     // 7 bits, 0 to 127
)
{
	// Set Midi Instrument Protocol:
	// [0] Status byte          : 0b 1100 CCCC
	//     Set Instrument Signature      : 0b 1100
	//     Channel 4-bits                : 0b CCCC
	// [1] Instrument 7-bits    : 0b 0III IIII
	// [2] Unused               : 0b 0000 0000
	// [3] Unused               : 0b 0000 0000

	VerifyLimit(channel, 15, "Channel");
	VerifyLimit(instrument, 127, "Instrument");

	const uint8_t SetInstrumentSignature = 0b1100;
	uint8_t statusByte = SetInstrumentSignature << 4; // 0b 1100 0000
	statusByte |= channel; // 0b 1100 CCCC

	MidiData midiData;
	midiData.bData[0] = statusByte;       // MIDI Status byte
	midiData.bData[1] = instrument;       // First MIDI data byte
	// Bytes [2] and [3] are unused

	VerifyMidi(midiOutShortMsg(hMidiOut, midiData.dwData));
}

// Plays Midi Note
// To Stop playing, set velocity parameter to 0
void SendMidiNote(
	HMIDIOUT hMidiOut,
	uint8_t channel,  // 4 bits, 0 to 15
	uint8_t pitch,    // 7 bits, 0 to 127
	uint8_t velocity  // 7 bits, 0 to 127
)
{
	// Note On Protocol:
	// [0] Status byte     : 0b 1001 CCCC
	//     Note On Signature   : 0b 1001
	//     Channel 4-bits      : 0b CCCC
	// [1] Pitch 7-bits    : 0b 0PPP PPPP
	// [2] Velocity 7-bits : 0b 0VVV VVVV
	// [3] Unused          : 0b 0000 0000
	// Reference: https://www.cs.cmu.edu/~music/cmsip/readings/MIDI%20tutorial%20for%20programmers.html

	// To Turn Note Off, simply pass 0 as Velocity (Volume)
	VerifyLimit(channel, 15, "Channel");
	VerifyLimit(pitch, 127, "Pitch");
	VerifyLimit(velocity, 127, "Velocity");

	const uint8_t NoteOnSignature = 0b1001;
	uint8_t statusByte = NoteOnSignature << 4; // 0b 1001 0000
	statusByte |= channel; // 0b 1001 CCCC

	MidiData midiData;
	midiData.bData[0] = statusByte;  // MIDI Status byte
	midiData.bData[1] = pitch;       // First MIDI data byte
	midiData.bData[2] = velocity;    // Second MIDI data byte
	// Byte [3] is unused

	VerifyMidi(midiOutShortMsg(hMidiOut, midiData.dwData));
}

int main()
{
	try
	{
		HMIDIOUT hMidiOut;
		VerifyMidi(midiOutOpen(
			/*out*/ &hMidiOut,
			/*uDeviceID*/ 0, // System's Midi device is at index 0
			/*dwCallback*/ NULL,
			/*dwInstance*/ NULL,
			/*fdwOpen*/ CALLBACK_NULL
		));

		// Set Instruments for Channels 0 and 1
		SetMidiInstrument(hMidiOut, /*channel*/ 0, /*Grand Piano*/ 0);
		SetMidiInstrument(hMidiOut, /*channel*/ 1, /*Guitar*/ 24);

		std::cout << "Play Piano C Note\n";
		SendMidiNote(hMidiOut, /*channel*/ 0, /*note*/ 60, /*velocity*/ 127);
		std::this_thread::sleep_for(3s);
		SendMidiNote(hMidiOut, /*channel*/ 0, /*note*/ 60, /*velocity*/ 0); // Stop

		std::cout << "Play Guitar C Note\n";
		SendMidiNote(hMidiOut, /*channel*/ 1, /*note*/ 60, /*velocity*/ 127);
		std::this_thread::sleep_for(3s);
		SendMidiNote(hMidiOut, /*channel*/ 1, /*note*/ 60, /*velocity*/ 0); // Stop

		VerifyMidi(midiOutClose(hMidiOut));
	}
	catch (const std::exception& e)
	{
		std::cout << "Exception: " << e.what();
		throw;
	}

	return 0;
}
