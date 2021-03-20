## Introduction

This project is a Demo of how to use Midi C++ Windows Apis to play Sample Sound Notes and Select Instruments.

Here's Midi C++ code to play C Note on Grand Piano:

```C++
// Copyright (c) Kodi Studios 2021.
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

using namespace std::chrono_literals;

// Midi message is 4 bytes.
// Windows Midi midiOutShortMsg() Api passes
// those 4 bytes as DWORD type.
// Use Union structure to easily overlap DWORD onto 
// Midi's 4 bytes.
union MidiMessage {
	BYTE bData[4];
	DWORD dwData{ 0 }; // Note: because it's a "union", this also zeros out all 4 bytes in bData array
};

MidiMessage MakeSendNoteMessage(
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

	const uint8_t NoteOnSignature = 0b1001;    
	uint8_t statusByte = NoteOnSignature;      // 0b 0000 1001
	statusByte = statusByte << 4;              // 0b 1001 0000
	statusByte = statusByte | channel;         // 0b 1001 CCCC

	MidiMessage midiMessage;
	midiMessage.bData[0] = statusByte;  // MIDI Status byte
	midiMessage.bData[1] = pitch;       // First MIDI data byte
	midiMessage.bData[2] = velocity;    // Second MIDI data byte
	// Byte [3] is unused

	return midiMessage;
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
	MidiMessage midiMessage = MakeSendNoteMessage(channel, pitch, velocity);
	midiOutShortMsg(hMidiOut, midiMessage.dwData);
}

MidiMessage MakeSelectInstrumentMessage(
	uint8_t channel,       // 4 bits, 0 to 15
	uint8_t instrument     // 7 bits, 0 to 127
)
{
	// Select Midi Instrument Protocol:
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
	midiMessage.bData[0] = statusByte;       // MIDI Status byte
	midiMessage.bData[1] = instrument;       // First MIDI data byte
	// Bytes [2] and [3] are unused

	return midiMessage;
}

void SelectMidiInstrument(
	HMIDIOUT hMidiOut,
	uint8_t channel,       // 4 bits, 0 to 15
	uint8_t instrument     // 7 bits, 0 to 127
)
{
	MidiMessage midiMessage = MakeSelectInstrumentMessage(channel, instrument);
	midiOutShortMsg(hMidiOut, midiMessage.dwData);
}

void PlayNote()
{
	HMIDIOUT hMidiOut;
	midiOutOpen(
		/*out*/ &hMidiOut,
		/*uDeviceID*/ 0, // System's Midi device is at index 0
		/*dwCallback*/ NULL,
		/*dwInstance*/ NULL,
		/*fdwOpen*/ CALLBACK_NULL
	);

	// Select Grand Piano Instrument for Channel 0
	SelectMidiInstrument(hMidiOut, /*channel*/ 0, /*Grand Piano*/ 0);

	std::cout << "Play Piano C Note\n";
	SendMidiNote(hMidiOut, /*channel*/ 0, /*note*/ 60, /*velocity*/ 127);
	std::this_thread::sleep_for(3s);
	SendMidiNote(hMidiOut, /*channel*/ 0, /*note*/ 60, /*velocity*/ 0); // Stop

	midiOutClose(hMidiOut);
}
```

Full Source Code:  
[MidiCppConsole.cpp](https://github.com/KodiStudios/midi-cpp-console/blob/main/MidiCppConsole/MidiCppConsole.cpp)

Compiled App:  
[MidiCppConsole.exe](https://github.com/KodiStudios/midi-cpp-console/releases/latest)

## Details

Project is a simple Console App and should run on any Windows OS, eg Windows 7, Windows 10.

Midi Apis used:  
```C++
MMRESULT midiOutOpen(...)
MMRESULT midiOutShortMsg(...)
MMRESULT midiOutClose(...)
```

## Midi Documentation

Full Windows Midi Api Official Documentation:  
[mmeapi](https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/)

## Links

Midi Protocol:  
https://www.cs.cmu.edu/~music/cmsip/readings/MIDI%20tutorial%20for%20programmers.html

Cheers!
