## Introduction

This project is a Demo of how to use Midi C++ Windows Apis to play Sample Sound Notes and Select Instruments.

Source Code:  
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
