# SDR Emulator

An emulator/simulator for Sun Devil Rocketry's embedded controllers. Runs embedded code at the mod level and above on local hardware.

**Currently supported projects:**
- None yet

**Project support in development for:**
- Flight Computer Rev 2 (A0002-Rev2)

### Testing

As this project is still in its early stages, automated test support has not yet been introduced. Despite this, we want to be very careful to maintain cross-platform compatibility across different systems used by our developers. 
Below are the hashes of the most recent recorded tests on different toolchains/architectures.

- Eli's Laptop (Win 11, 4-core AMD64) - e4552f6c656f5e4a2b195dff5530959055d6a153 ("Checkpoint: I2C IT") - 1/26/26
- Eli's PC (Linux (Arch), 10-core AMD64) - 9e42849ec05efefb5d74ae0af6668ef437e98769 ("Fix a fclose call that was analogous to a double free.") - 1/26/26
