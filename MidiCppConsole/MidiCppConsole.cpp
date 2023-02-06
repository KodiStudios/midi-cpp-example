// Copyright (c) Kodi Studios 2023.
// Licensed under the MIT license.

// REQUIREMENTS!
// 
// Add following lib file to C++ Linker:
// winmm.lib
// For example, in Visual Studio:
// Project > Properties > Configuration Properties > Linker > Input > Additional Dependencies
// Add: winmm.lib

#include <Windows.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

// Midi Message is 4 bytes.
// Windows Midi midiOutShortMsg() Api passes
// those 4 bytes as DWORD type.
// Use C++ Union structure to easily overlap DWORD onto 
// Midi's 4 bytes and initialize these bytes to 0
union MidiMessage {
	BYTE dataByte[4];
	DWORD dataDWord{ 0 }; // Note: because it's a "union", this also zeros out all 4 bytes in bData array
};

// Plays Midi Note
// To Stop playing, set velocity parameter to 0
void SendMidiNote(
	HMIDIOUT hMidiOut,
	uint8_t channel,  // 4 bits, 0 to 15
	uint8_t pitch,    // 7 bits, 0 to 127
	uint8_t velocity  // 7 bits, 0 to 127
)
{
	// "Note On" Protocol:
	// [0] Status byte     : 0b 1001 CCCC
	//     Note On Signature   : 0b 1001
	//     Channel 4-bits      : 0b CCCC
	// [1] Pitch 7-bits    : 0b 0PPP PPPP
	// [2] Velocity 7-bits : 0b 0VVV VVVV
	// [3] Unused          : 0b 0000 0000
	// Reference: https://www.cs.cmu.edu/~music/cmsip/readings/MIDI%20tutorial%20for%20programmers.html

	// To Turn "Note Off", simply pass 0 as Velocity (Volume)

	const uint8_t NoteOnSignature = 0b1001;
	uint8_t statusByte = NoteOnSignature;      // 0b 0000 1001
	statusByte = statusByte << 4;              // 0b 1001 0000
	statusByte = statusByte | channel;         // 0b 1001 CCCC

	MidiMessage midiMessage;
	midiMessage.dataByte[0] = statusByte;  // MIDI Status byte
	midiMessage.dataByte[1] = pitch;       // First MIDI data byte
	midiMessage.dataByte[2] = velocity;    // Second MIDI data byte
	// Byte [3] is unused

	midiOutShortMsg(hMidiOut, midiMessage.dataDWord);
}

void SelectMidiInstrument(
	HMIDIOUT hMidiOut,
	uint8_t channel,       // 4 bits, 0 to 15
	uint8_t instrument     // 7 bits, 0 to 127
)
{
	// "Select Midi Instrument" Protocol:
	// [0] Status byte          : 0b 1100 CCCC
	//     Select Instrument Signature      : 0b 1100
	//     Channel 4-bits                   : 0b CCCC
	// [1] Instrument 7-bits    : 0b 0III IIII
	// [2] Unused               : 0b 0000 0000
	// [3] Unused               : 0b 0000 0000

	const uint8_t SetInstrumentSignature = 0b1100;
	uint8_t statusByte = SetInstrumentSignature; // 0b 0000 1100
	statusByte = statusByte << 4;                // 0b 1100 0000
	statusByte |= channel;                       // 0b 1100 CCCC

	MidiMessage midiMessage;
	midiMessage.dataByte[0] = statusByte;       // MIDI Status byte
	midiMessage.dataByte[1] = instrument;       // First MIDI data byte
	// Bytes [2] and [3] are unused

	midiOutShortMsg(hMidiOut, midiMessage.dataDWord);
}

int main()
{
	HMIDIOUT hMidiOut;

	// Open Midi Handle
	midiOutOpen(
		/*out*/ &hMidiOut,
		/*uDeviceID*/ 0, // System's Midi device is at index 0
		/*dwCallback*/ NULL,
		/*dwInstance*/ NULL,
		/*fdwOpen*/ CALLBACK_NULL
	);

	std::cout << "Select Midi Instrument: Guitar\n";
	SelectMidiInstrument(hMidiOut, /*channel*/ 0, /*instrument: Guitar*/ 24);

	std::cout << "Start Playing Note: Middle C\n";
	SendMidiNote(
		hMidiOut,
		/*channel*/ 0,
		/*pitch (Note): Middle C*/ 60,
		/*velocity (Volume)*/ 90);

	std::cout << "Continue Playing for 2 Seconds\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(2000)); 

	std::cout << "Stop Playing Note\n";
	SendMidiNote(
		hMidiOut,
		/*channel*/ 0,
		/*pitch*/ 60,
		/*velocity (Volume): Min*/ 0); 

	// Close Midi Handle
	midiOutClose(hMidiOut);

	return 0;
}
