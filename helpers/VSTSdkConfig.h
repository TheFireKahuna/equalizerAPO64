#pragma once

#if defined(__has_include)
#  if __has_include("pluginterfaces/base/ftypes.h") || __has_include("public.sdk/source/vst/vstcomponent.h")
#    define MODDEDAPO_HAS_VST3_SDK 1
#  else
#    define MODDEDAPO_HAS_VST3_SDK 0
#  endif
#else
#  define MODDEDAPO_HAS_VST3_SDK 0
#endif

#if !MODDEDAPO_HAS_VST3_SDK
#  define MODDEDAPO_VST3_UNAVAILABLE 1
#endif

#if defined(__has_include)
#  if __has_include("aeffect.h") || __has_include("pluginterfaces/vst2.x/aeffect.h")
#    define MODDEDAPO_HAS_VST2_SDK 1
#  else
#    define MODDEDAPO_HAS_VST2_SDK 0
#  endif
#else
#  define MODDEDAPO_HAS_VST2_SDK 0
#endif

#if !MODDEDAPO_HAS_VST2_SDK
#  define MODDEDAPO_VST2_UNAVAILABLE 1
#endif
