# EqualizerAPO (double precision edition)

This repo contains an updated, 1.4.2 port of [EqualizerAPO](https://sourceforge.net/p/equalizerapo/) - a system wide equalizer for Windows, enhanced with double precision (64 bit) audio processing and AVX support.

This effort is inspired and founded on a previous conversion of EqualizerAPO to double precision processing (https://github.com/chebum/equalizer-apo-64), updated here with the latest release, and further enhanced to use AVX for optimal performance/cpu usage.

Double procession processing (64 bit internal pipeline) in EqualizerAPO maintains precision and quality when applying multiple overlapping effects. Examples include convolution, complex parametric EQ setups or GraphicEQ's. Support for AVX256 also ensures similar performance to the original 32 bit pipeline, with additional optional support for AVX512/AVX10.1 to further enhance performance.

No functionality was added or modified otherwise.

## Installation
1. You will need standard Equalizer APO 1.4.2 installed on your system.
2. Disable Equalizer APO for all audio devices via the **Equalizer APO Device Selector**.
3. Unpack the [attached zip archive](https://github.com/TheFireKahuna/equalizerAPO64/releases/) into the Equalizer APO installation folder. Replace EqualizerAPO.dll. *Consider backing up the original directory before doing this, otherwise you will need to reinstall EqualizerAPO to remove this.*
4. Re-enable Equalizer APO for any relevant audio devices via the **Equalizer APO Device Selector**. This will load the modified 64-bit audio processing engine for immediate use.

## Uninstall

1. Reinstall EqualizerAPO to overwrite all files with original copies, or restore a backup of the installation folder.
