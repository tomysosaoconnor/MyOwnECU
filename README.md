# MyOwnECU
An electronic fuel injection system based on software and hardware simplicity, with the purpose of engine management and modification studying. Everything implemented in C.

The MyOwnEcu project is based on 2 sub-projects: MyEFI and MyCDI.

- MyEFI controls injection, with one fuel injector output, which is plenty for managing well any engine with multipoint batched injection. Sensors to read are points or any sort of tach signal that works for RPM measurement, ECT, IAT, battery voltage and MAP.
- MyCDI controls ignition based on the same tach signal with a known advance BTDC, so we use a delay, for example, from 45° BTDC to produce the correct timing. Inputs are the already mentioned tach signal and possibly a MAP sensor.

The idea, as already told, is to produce very simple to understand C code, with maybe some in-line assembly, in case a platform to merge both sub-projects into one single controller is selected.

Current platform is the ATMega328P microcontroller for each project. The idea of the use of C (and trying to use as few Arduino libraries as possible) is to provide compatibility with Atmel Studio and WinAVR, but still being able to use Arduino IDE. The Arduino stuff that is easy to transfer to Atmel Studio and WinAVR are the map(), millis() and micros() functions, as well as analogRead() which can be implemented with ease in custom made functions.

Nothing on the "probably not very good" code I'll post is tested, so there's plenty of work to do. The idea of using 2 microcontrollers in 2 independent modules stands as one of the basic project's axioms.

With a suitable communication interface (probably I2C) the creation of another sub-project that features a screen (i.e. a tft dashboard), or any sort of extension for the system (like a knock detector or a digital IO extension) is feasible.
