# EqualizerAPO (double precision edition)

This repo contains a modified 1.4.2 port of [EqualizerAPO](https://sourceforge.net/p/equalizerapo/) - a system wide equalizer for Windows, enhanced with double precision (64 bit) audio processing, expanded AVX support and a fully automated build pipeline using Visual Studio 2022.

This effort was inspired and founded on a previous conversion of EqualizerAPO to double precision processing (https://github.com/chebum/equalizer-apo-64), updated and later rebuilt here with the latest EAPO release, and further enhanced with expanded AVX usage for optimal performancee.

Double procession processing (64 bit internal pipeline) in EqualizerAPO maintains precision and quality when applying multiple overlapping effects. Examples include convolution, complex parametric EQ setups or GraphicEQ's. Support for AVX256 also ensures similar performance to the original 32 bit pipeline, with additional optional support for AVX512 to further enhance performance. ARM is also supported via a native start-to-end build of all dependencies and Qt projects.

## Features
- **Double precision processing filter engine** (32bit floats -> 64bit doubles) for improved fidelity with more complex setups
- Support for AVX2 & AVX-512 in both EAPO and all dependencies, including FFTW3's AVX2/AVX-512 optimizations.
- Expanded usage of AVX instructions to reduce processing latency
- Uses shared VC++ DLLs for improved compatibility
- Compiled with VS 2022 for all components, using the latest Windows SDK and additional compiler optimizations.
- Built with AOCL-FFTW 3.3.10 (optimized for AMD/modern CPUs)
- Updated Qt 6.10 for GUI components with native ARM support.
- Uses fp-precise instead of fp-fast for floating point math.
- Automated build pipeline for all components and the installer via Github Actions
- Fully native start-to-end ARM64 automated build.

## Installation
Download the latest version from the [Releases page](https://github.com/TheFireKahuna/equalizerAPO64/releases).

## Building EqualizerAPO
This project uses a fully automated Github Actions CI pipeline for both the main project and all dependencies.

The following forked repositories are used for a stable pipeline, each including a CI pipline and a packaged release using the latest Github Actions build:
- [AOCL-FFTW 5.1 (FFTW 3.3.10)](https://github.com/thefirekahuna/amd-fftw)
- [muparserx 4.0.12](https://github.com/thefirekahuna/muparserx)
- [libsndfile 1.2.2](https://github.com/thefirekahuna/libsndfile)
- [tclap 1.2.5](https://github.com/thefirekahuna/tclap)

Local builds are configured via shared environment variables, directly configurable in the relevant .vcxproj and .pro files as follows:
```
  <PropertyGroup>
    <LIBSNDFILE_INCLUDE Condition="'$(LIBSNDFILE_INCLUDE)'==''">F:\Git\libsndfile\include</LIBSNDFILE_INCLUDE>
    <LIBSNDFILE_LIB Condition="'$(LIBSNDFILE_LIB)'==''">F:\Git\libsndfile\out\build\x64-Release</LIBSNDFILE_LIB>
    <FFTW_INCLUDE Condition="'$(FFTW_INCLUDE)'==''">F:\Git\amd-fftw\build\include</FFTW_INCLUDE>
    <FFTW_LIB Condition="'$(FFTW_LIB)'==''">F:\Git\amd-fftw\out\build\x64-Release</FFTW_LIB>
    <MUPARSERX_INCLUDE Condition="'$(MUPARSERX_INCLUDE)'==''">F:\Git\muparserx\parser</MUPARSERX_INCLUDE>
    <MUPARSERX_LIB Condition="'$(MUPARSERX_LIB)'==''">F:\Git\muparserx\lib64</MUPARSERX_LIB>
    <TCLAP_ROOT Condition="'$(TCLAP_ROOT)'==''">F:\Git\tclap</TCLAP_ROOT>
  </PropertyGroup>
```
