# SDR Emulator

An emulator/simulator for Sun Devil Rocketry's embedded controllers. Runs embedded code at the mod level and above on local hardware. This project
replaces the "lib" layer and below in SDR's software stack. Supported projects allow emulator builds by invoking their top level makefile with a
specific argument for each project.

**Currently supported projects:**
- Flight Computer Rev 2 (A0002-Rev2)

### Testing

The emulator is also used to support automated tests of the Flight Computer. This test suite runs checks that were previously performed manually
and took a significant amount of time.

The emulator's only supported project runs an automated test suite including multiple platforms of emulator builds. We want to be very careful 
to maintain cross-platform compatibility across different systems used by our developers, and these tests ensure that. Please respond to build
breakages promptly. We test frequently on Windows and X11 Linux, including in our CI. We want Wayland and MacOS support, but lack developers 
with those systems.