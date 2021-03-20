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

void PlayNote(
	uint8_t channel,
	uint8_t instrument,
	uint8_t pitch, // Note
	uint8_t velocity, // Volume
	uint32_t noteLength)
{
	HMIDIOUT hMidiOut;
	midiOutOpen(
		/*out*/ &hMidiOut,
		/*uDeviceID*/ 0, // System's Midi device is at index 0
		/*dwCallback*/ NULL,
		/*dwInstance*/ NULL,
		/*fdwOpen*/ CALLBACK_NULL
	);

	// Select Instrument for given Channel
	SelectMidiInstrument(hMidiOut, channel, instrument);

	// Start Playing Note
	SendMidiNote(hMidiOut, channel, pitch, velocity);
	std::this_thread::sleep_for(std::chrono::milliseconds(noteLength));
	SendMidiNote(hMidiOut, channel, pitch, 0); // Stop

	midiOutClose(hMidiOut);
}

// Helper namespace to report any errors coming from Midi Apis
namespace Robust
{
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
			throw MidiException(s.str().c_str());                      \
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

	void SelectMidiInstrument(
		HMIDIOUT hMidiOut,
		uint8_t channel,       // 4 bits, 0 to 15
		uint8_t instrument     // 7 bits, 0 to 127
	)
	{
		VerifyLimit(channel, 15, "Channel");
		VerifyLimit(instrument, 127, "Instrument");

		MidiMessage midiMessage = MakeSelectInstrumentMessage(channel, instrument);

		VerifyMidi(midiOutShortMsg(hMidiOut, midiMessage.dwData));
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

		MidiMessage midiMessage = MakeSendNoteMessage(channel, pitch, velocity);

		VerifyMidi(midiOutShortMsg(hMidiOut, midiMessage.dwData));
	}

	void PlayNote(
		uint8_t channel,
		uint8_t instrument,
		uint8_t pitch, // Note
		uint8_t velocity, // Volume
		uint32_t noteLength)
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

			// Select Instrument for Channel 0
			Robust::SelectMidiInstrument(hMidiOut, /*channel*/ channel, /*Grand Piano*/ instrument);

			// Start Playing Note
			Robust::SendMidiNote(hMidiOut, channel, pitch, velocity);
			std::this_thread::sleep_for(std::chrono::milliseconds(noteLength));
			Robust::SendMidiNote(hMidiOut, channel, pitch, 0); // Stop

			VerifyMidi(midiOutClose(hMidiOut));
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception: " << e.what();
			throw;
		}
	}
}

class AppArguments
{
public:
	uint8_t Channel{ 0 };
	uint8_t Instrument{ 0 };       // 0 is Grand Piano
	uint8_t Pitch{ 60 };           // 60 is C Note
	uint8_t Velocity{ 127 };       // 127 is Max Velocity (Volume)
	uint32_t Length{ 3000 };       // Note Length, in Milliseconds
	bool SimpleApi{ false };       // Use Simple Midi Api, without error detection
	bool UserOverridden{ false };  // User had specified some flags
	bool Help{ false };
};

namespace ArgumentParsing {
	template<class T>
	T GetValue(char* v)
	{
		return (T)atoi(v);
	}

	std::unique_ptr<AppArguments> ParseArguments(int argc, char* argv[])
	{
		std::unique_ptr<AppArguments> myArguments{ std::make_unique<AppArguments>() };
		
		// argv[0] is the name of the Program
		// argv[1] is the first argument
		for (int i = 1; i < argc; i++)
		{
			myArguments->UserOverridden = true;
			if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "/c")) { i++; Robust::VerifyLimit(i, argc - 1, "Channel"); myArguments->Channel = GetValue<uint8_t>(argv[i]); }
			if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "/i")) { i++; Robust::VerifyLimit(i, argc - 1, "Instrument"); myArguments->Instrument = GetValue<uint8_t>(argv[i]); }
			if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "/p")) { i++; Robust::VerifyLimit(i, argc - 1, "Pitch"); myArguments->Pitch = GetValue<uint8_t>(argv[i]); }
			if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "/v")) { i++; Robust::VerifyLimit(i, argc - 1, "Velocity"); myArguments->Velocity = GetValue<uint8_t>(argv[i]); }
			if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "/l")) { i++; Robust::VerifyLimit(i, argc - 1, "Length"); myArguments->Length = GetValue<uint32_t>(argv[i]); }
			if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "/s")) { myArguments->SimpleApi = true; }
			if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "/?")) { myArguments->Help = true; }
		}

		return myArguments;
	}

	void PrintHelp(char* appName)
	{
		AppArguments defaultArguments;
		std::cout << "All flags are Optional!\n";
		std::cout << "-c [0-15]" << "     Channel. Default: " << defaultArguments.Channel << "\n";
		std::cout << "-i [0-127]" << "    Instrument. Default: " << defaultArguments.Instrument << "(Grand Piano)\n";
		std::cout << "-p [0-127]" << "    Pitch (Note). Default: " << defaultArguments.Pitch << "(Middle C Note)\n";
		std::cout << "-v [0-127]" << "    Velocity (Volume). Default: " << defaultArguments.Velocity << "\n";
		std::cout << "-l [milliseconds]" << "   Length (Note Length), in Milliseconds. Default: " << defaultArguments.Length << "(3 Seconds)\n";
		std::cout << "-s" << "            Use Simple Midi Api, no error detection.\n";
		std::cout << "-?" << "            Print this help\n";
		std::cout << "\n";
		std::cout << "Sample Usage:\n";
		std::cout << "\n";
		std::cout << appName << "-i 24 -p 80\n";
		std::cout << "Play Guitar Note\n";
		std::cout << "\n";
		std::cout << appName << "-c 1 -i 24 -p 81 -v 120 -l 2000\n";
		std::cout << "Sets Guitar to Channel 1, Plays G Note, at Volume 120, for 2 seconds\n";
	}
}

int main(int argc, char* argv[])
{
	std::unique_ptr<AppArguments> appArguments;
	try
	{
		appArguments = ArgumentParsing::ParseArguments(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::cout << "Incorrect Arguments\n";
		std::cout << "Exception: " << e.what() << "\n";
		ArgumentParsing::PrintHelp(/*appName*/ argv[0]);
		throw;
	}

	if (appArguments->Help)
	{
		ArgumentParsing::PrintHelp(/*appName*/ argv[0]);
		return 0;
	}

	if (!appArguments->UserOverridden)
	{
		std::cout << "Play Piano C Note\n";
	}

	if (appArguments->SimpleApi)
	{
		// No error checks
		PlayNote(appArguments->Channel, appArguments->Instrument, appArguments->Pitch, appArguments->Velocity, appArguments->Length);
	}
	else
	{
		// With error checks
		Robust::PlayNote(appArguments->Channel, appArguments->Instrument, appArguments->Pitch, appArguments->Velocity, appArguments->Length);
	}
	
	return 0;
}
