# EqualizerAPO - 64bit port

This repo contains an updated, 1.2.1 64-bit port of [EqualizerAPO](https://sourceforge.net/p/equalizerapo/) - system wide equalizer for Windows.

The port here is inspired and checked against a previous port of EqualizerAPO (https://github.com/chebum/equalizer-apo-64) to double precision processing, while also instead using AVX2 for reduced CPU usage and latency.

Double procession processing (64 bit internal pipeline) in EqualizerAPO maintains precision and quality when applying multiple overlapping effects, namely convolution, large parametric EQ setups or GraphicEQ's. While its a general quality increase, its mainly noticable in more complex EqualierAPO usage.

No functionality was added or modified, though VSTs will likely not work with the present code (needs testing).

## Installation

**Method 1 (requires Reboot)**
1. You will need standard Equalizer APO 1.2.1 installed on your system.
2. Disable Equalizer APO for all audio devices.
3. Reboot your computer.
4. Unpack the [attached zip archive](https://github.com/TheFireKahuna/equalizerAPO64/releases/) into the Equalizer APO installation folder. Replace EqualizerAPO.dll. Consider backing up the original directory before doing this, otherwise you will need to reinstall EqualizerAPO to remove this.
5. Re-enable Equalizer APO with the following settings on Windows 10:
	- Select your device, tick its box and then tick the Troubleshooting options box below.
	- Untick Pre-mix, tick Post-mix for Install APO. Switch the drop-down menu to Install as SFX/EFX (experimental).
6. Reboot. This will load the modified 64-bit audio processing engine.

**Method 2 (no Reboot)**
1. You will need standard Equalizer APO 1.2.1 installed on your system.
2. Launch Task Manager, go to Services, right-click stop the Windows Audio (Audiosrv) and Windows Audio Endpoint Builder (AudioEndpointBuilder) services.
4. Unpack the [attached zip archive](https://github.com/TheFireKahuna/equalizerAPO64/releases/) into the Equalizer APO installation folder. Replace EqualizerAPO.dll. Consider backing up the original directory before doing this, otherwise you will need to reinstall EqualizerAPO to remove this.
5. Right-click start the two audio services. This will load the modified 64-bit audio processing engine.
- Note, either way I still recommend installing your APO with the settings in Method 1 if your running Windows 10, as otherwise distortion can occur.

## Uninstallation

1. Reinstall EqualizerAPO or restore a backup of the installation folder.
