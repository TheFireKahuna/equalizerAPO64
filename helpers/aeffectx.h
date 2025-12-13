/* An attempt at an untained clean room reimplementation of the widely popular VST 2.x SDK.
 * Copyright (c) 2020 Xaymar Dirks <info@xaymar.com> (previously known as Michael Fabian Dirks)
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *    disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *    following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Please refer to README.md and LICENSE for further information. */

/* Protect against double inclusion in practically every compiler available. */
#pragma once
#ifndef VST2SDK_VST_H
#define VST2SDK_VST_H


/* The VST 2.x alignment appears to be 8 for both 32 and 64-bit. This alignment is ignored by some earlier Windows
 * platforms and compilers, but we don't care about those.
 */
#pragma pack(push, 8)

#ifdef __cplusplus
#if __cplusplus < 201103L
#include <inttypes.h>
#else
#include <cinttypes>
#endif
extern "C" {
#else
#include <inttypes.h>
#endif

/** Standard calling convention across plug-ins and hosts.
 * On some older Windows platforms this is not __cdecl but something similar to __stdcall. We don't really care about
 * those old platforms anyway so __cdecl it is.
 */
#define VST_FUNCTION_INTERFACE __cdecl

/** Maximum number of channels/streams/inputs/outputs supported by VST 2.x
 * Couldn't find any audio editing software which would attempt to add more channels.
 *
 * @todo Is 32 channels really the maximum?
 */
#define VST_MAX_CHANNELS 32

/** Convert four numbers into a FourCC
 */
#define VST_FOURCC(a,b,c,d) ((((uint32_t)a) << 24) | (((uint32_t)b) << 16) | (((uint32_t)c) << 8) | (((uint32_t)d) << 0))

/** Known Status Codes
 */
enum VST_STATUS {
	/** Unknown / False
	 * We either don't know the answer or we can't handle the data/notification.
	 *
	 * @sa VST_HOST_OPCODE
	 * @sa VST_EFFECT_OPCODE
	 */
	VST_STATUS_0 = 0,
	/** @sa VST_STATUS_0 */
	VST_STATUS_FALSE = 0,
	/** @sa VST_STATUS_0 */
	VST_STATUS_ERROR = 0,
	/** @sa VST_STATUS_0 */
	VST_STATUS_UNKNOWN = 0,

	/** Yes / True
	 * We've handled the data/notification.
	 *
	 * @sa VST_HOST_OPCODE
	 * @sa VST_EFFECT_OPCODE
	 */
	VST_STATUS_1 = 1,
	/** @sa VST_STATUS_1 */
	VST_STATUS_TRUE = 1,
	/** @sa VST_STATUS_1 */
	VST_STATUS_SUCCESS = 1,
	/** @sa VST_STATUS_1 */
	VST_STATUS_YES = 1,

	/** No
	 * We're unable to handle the data/notification.
	 *
	 * @sa VST_HOST_OPCODE
	 * @sa VST_EFFECT_OPCODE
	 */
	VST_STATUS_m1 = -1,
	/** @sa VST_STATUS_m1 */
	VST_STATUS_NO = -1,

	_VST_STATUS_PAD = (-1l)
};

/** Known Buffer Sizes
 */
enum VST_BUFFER_SIZE {
	VST_BUFFER_SIZE_PARAM_LABEL = 8,
	VST_BUFFER_SIZE_PARAM_NAME = 8,
	VST_BUFFER_SIZE_PARAM_VALUE = 8,
	VST_BUFFER_SIZE_STREAM_LABEL = 8,
	VST_BUFFER_SIZE_CATEGORY_LABEL = 24,
	VST_BUFFER_SIZE_PROGRAM_NAME = 24,
	VST_BUFFER_SIZE_EFFECT_NAME = 32,
	VST_BUFFER_SIZE_PARAM_LONG_NAME = 64,
	VST_BUFFER_SIZE_PRODUCT_NAME = 64,
	VST_BUFFER_SIZE_SPEAKER_NAME = 64,
	VST_BUFFER_SIZE_STREAM_NAME = 64,
	VST_BUFFER_SIZE_VENDOR_NAME = 64
};

/** Valid VST 1.x and 2.x versions
 * The format is either a single digit or four digits in Base10 format.
 *
 * @code{.c}
 * // Converts a Base10 VST version to a uint8_t[4] representation of the version.
 * uint32_t expand_vst_version(uint32_t v) {
 *   if (v < 10) { //
 *     return v << 24;
 *   }
 *   uint8_t major = v / 1000;
 *   uint8_t minor = (v / 100) % 10;
 *   uint8_t revision = (v / 10) % 10;
 *   uint8_t patch = v % 10;
 *   return (major << 24) | (minor << 16) | (revision << 8) | patch;
 * }
 * @endcode
 */
enum VST_VERSION {
	/** Private SDK Version 1.0
	 *
	 * Many types likely won't quite match up with what we expect.
	 */
	VST_VERSION_1       = 0,
	/** SDK Version 1.0. */
	VST_VERSION_1_0_0_0 = 1000,
	/** SDK Version 1.1. */
	VST_VERSION_1_1_0_0 = 1100,
	/** Private SDK Version 2.0
	 *
	 * Many types likely won't quite match up with what we expect.
	 */
	VST_VERSION_2       = 2,
	/** SDK Version 2.0 */
	VST_VERSION_2_0_0_0 = 2000,
	/** SDK Version 2.1 */
	VST_VERSION_2_1_0_0 = 2100,
	/** SDK Version 2.2 */
	VST_VERSION_2_2_0_0 = 2200,
	/** SDK Version 2.3 */
	VST_VERSION_2_3_0_0 = 2300,
	/** SDK Version 2.4 */
	VST_VERSION_2_4_0_0 = 2400,

	/* @private Pad to 32-bit. */
	_VST_VERSION_PAD = (-1l)
};

/** Window/Editor Rectangle.
 * The order is counter-clockwise starting from the top.
 */
struct vst_rect_t {
	int16_t top;
	int16_t left;
	int16_t bottom;
	int16_t right;
};

/** Virtual Key codes.
 *
 * Steinberg seems to like reinventing the wheel. What was the problem with just using the platform specific key codes?
 */
enum VST_VKEY {
	VST_VKEY_00 = 0,

	VST_VKEY_01 = 1,
	VST_VKEY_BACKSPACE = 1,

	VST_VKEY_02 = 2,
	VST_VKEY_TAB = 2,

	VST_VKEY_03 = 3,

	VST_VKEY_04 = 4,
	VST_VKEY_RETURN = 4,

	VST_VKEY_05 = 5,
	VST_VKEY_PAUSE = 5,

	VST_VKEY_06 = 6,
	VST_VKEY_ESCAPE = 6,

	VST_VKEY_07 = 7,
	VST_VKEY_SPACE = 7,

	VST_VKEY_08 = 8,

	VST_VKEY_09 = 9,
	VST_VKEY_END = 9,

	VST_VKEY_10 = 10,
	VST_VKEY_HOME = 10,

	VST_VKEY_11 = 11,
	VST_VKEY_ARROW_LEFT = 11,

	VST_VKEY_12 = 12,
	VST_VKEY_ARROW_UP = 12,

	VST_VKEY_13 = 13,
	VST_VKEY_ARROW_RIGHT = 13,

	VST_VKEY_14 = 14,
	VST_VKEY_ARROW_DOWN = 14,

	VST_VKEY_15 = 15,
	VST_VKEY_PAGE_UP = 15,

	VST_VKEY_16 = 16,
	VST_VKEY_PAGE_DOWN = 16,

	VST_VKEY_17 = 17,

	VST_VKEY_18 = 18,
	VST_VKEY_PRINT = 18,

	VST_VKEY_19 = 19,
	VST_VKEY_NUMPAD_ENTER = 19,

	VST_VKEY_20 = 20,

	VST_VKEY_21 = 21,
	VST_VKEY_INSERT = 21,

	VST_VKEY_22 = 22,
	VST_VKEY_DELETE = 22,

	VST_VKEY_23 = 23,

	VST_VKEY_24 = 24,
	VST_VKEY_NUMPAD_0 = 24,

	VST_VKEY_25 = 25,
	VST_VKEY_NUMPAD_1 = 25,

	VST_VKEY_26 = 26,
	VST_VKEY_NUMPAD_2 = 26,

	VST_VKEY_27 = 27,
	VST_VKEY_NUMPAD_3 = 27,

	VST_VKEY_28 = 28,
	VST_VKEY_NUMPAD_4 = 28,

	VST_VKEY_29 = 29,
	VST_VKEY_NUMPAD_5 = 29,

	VST_VKEY_30 = 30,
	VST_VKEY_NUMPAD_6 = 30,

	VST_VKEY_31 = 31,
	VST_VKEY_NUMPAD_7 = 31,

	VST_VKEY_32 = 32,
	VST_VKEY_NUMPAD_8 = 32,

	VST_VKEY_33 = 33,
	VST_VKEY_NUMPAD_9 = 33,

	VST_VKEY_34 = 34,
	VST_VKEY_NUMPAD_MULTIPLY = 34,

	VST_VKEY_35 = 35,
	VST_VKEY_NUMPAD_ADD = 35,

	VST_VKEY_36 = 36,
	VST_VKEY_NUMPAD_COMMA_OR_DOT = 36,

	VST_VKEY_37 = 37,
	VST_VKEY_NUMPAD_SUBTRACT = 37,

	VST_VKEY_38 = 38,

	VST_VKEY_39 = 39,
	VST_VKEY_NUMPAD_DIVIDE = 39,

	VST_VKEY_40 = 40,
	VST_VKEY_F1 = 40,

	VST_VKEY_41 = 41,
	VST_VKEY_F2 = 41,

	VST_VKEY_42 = 42,
	VST_VKEY_F3 = 42,

	VST_VKEY_43 = 43,
	VST_VKEY_F4 = 43,

	VST_VKEY_44 = 44,
	VST_VKEY_F5 = 44,

	VST_VKEY_45 = 45,
	VST_VKEY_F6 = 45,

	VST_VKEY_46 = 46,
	VST_VKEY_F7 = 46,

	VST_VKEY_47 = 47,
	VST_VKEY_F8 = 47,

	VST_VKEY_48 = 48,
	VST_VKEY_F9 = 48,

	VST_VKEY_49 = 49,
	VST_VKEY_F10 = 49,

	VST_VKEY_50 = 50,
	VST_VKEY_F11 = 50,

	VST_VKEY_51 = 51,
	VST_VKEY_F12 = 51,

	VST_VKEY_52 = 52,
	VST_VKEY_NUMLOCK = 52,

	VST_VKEY_53 = 53,
	VST_VKEY_SCROLLLOCK = 53,

	VST_VKEY_54 = 54,
	VST_VKEY_SHIFT = 54,

	VST_VKEY_55 = 55,
	VST_VKEY_CONTROL = 55,

	VST_VKEY_56 = 56,
	VST_VKEY_ALT = 56,

	VST_VKEY_57 = 57,
	VST_VKEY_58 = 58,
	VST_VKEY_59 = 59,
	VST_VKEY_60 = 60,
	VST_VKEY_61 = 61,
	VST_VKEY_62 = 62,
	VST_VKEY_63 = 63,
	VST_VKEY_64 = 64,
	VST_VKEY_65 = 65,
	VST_VKEY_66 = 66,
	VST_VKEY_67 = 67,
	VST_VKEY_68 = 68,
	VST_VKEY_69 = 69
};

enum VST_VKEY_MODIFIER {
	/** One of the shift keys is held down. */
	VST_VKEY_MODIFIER_1ls0 = 1 << 0,
	/** @sa VST_VKEY_MODIFIER_1ls0 */
	VST_VKEY_MODIFIER_SHIFT = 1 << 0,

	/** One of the alt keys is held down. */
	VST_VKEY_MODIFIER_1ls1 = 1 << 1,
	/** @sa VST_VKEY_MODIFIER_1ls1 */
	VST_VKEY_MODIFIER_ALT = 1 << 1,

	/** Control on MacOS, System (Windows Logo) on Windows.
	 *
	 * Very funny Steinberg.
	 */
	VST_VKEY_MODIFIER_1ls2 = 1 << 2,
	/** @sa VST_VKEY_MODIFIER_1ls2 */
	VST_VKEY_MODIFIER_SYSTEM = 1 << 2,

	/** Control on PC, System (Apple Logo) on Mac OS.
	 *
	 * I have questions. They're all "Why?!".
	 */
	VST_VKEY_MODIFIER_1ls3 = 1 << 3,
	/** @sa VST_VKEY_MODIFIER_1ls3 */
	VST_VKEY_MODIFIER_CONTROL = 1 << 3
};

/*------------------------------------------------------------------------------------------------------------------------*/
/* VST Parameters */
/*------------------------------------------------------------------------------------------------------------------------*/

/** Flags for parameters.
 * @sa vst_parameter_properties_t
 */
enum VST_PARAMETER_FLAG {
	/** Parameter is an on/off switch.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls0 = 1 << 0,
	/** @sa VST_PARAMETER_FLAG_1ls0 */
	VST_PARAMETER_FLAG_SWITCH = 1,

	/** Parameter limits are set as integers.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls1 = 1 << 1,
	/** @sa VST_PARAMETER_FLAG_1ls1 */
	VST_PARAMETER_FLAG_INTEGER_LIMITS = 1 << 1,

	/** Parameter uses float steps.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls2 = 1 << 2,
	/** @sa VST_PARAMETER_FLAG_1ls2 */
	VST_PARAMETER_FLAG_STEP_FLOAT = 1 << 2,

	/** Parameter uses integer steps.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls3 = 1 << 3,
	/** @sa VST_PARAMETER_FLAG_1ls3 */
	VST_PARAMETER_FLAG_STEP_INT = 1 << 3,

	/** Parameter has an display order index for the default editor.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls4 = 1 << 4,
	/** @sa VST_PARAMETER_FLAG_1ls4 */
	VST_PARAMETER_FLAG_INDEX = 1 << 4,

	/** Parameter has a category for the default editor.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	VST_PARAMETER_FLAG_1ls5 = 1 << 5,
	/** @sa VST_PARAMETER_FLAG_1ls5 */
	VST_PARAMETER_FLAG_CATEGORY = 1 << 5,

	/** Parameter can be gradually increased/decreased.
	 *
	 * @sa VST_EFFECT_OPCODE_PARAM_IS_AUTOMATABLE
	 */
	VST_PARAMETER_FLAG_1ls6 = 1 << 6,
	/** @sa VST_PARAMETER_FLAG_1ls6 */
	VST_PARAMETER_FLAG_RAMPING = 1 << 6,

	_VST_PARAMETER_FLAG_PAD     = (-1l)
};

/** Information about a parameter.
 *
 * @important Many VST hosts and plug-ins expect their parameters to be normalized within 0.0 and 1.0.
 */
struct vst_parameter_properties_t {
	/** Float Step value
	 *
	 * Some hosts and plug-ins expect this to be within 0 and 1.0.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_STEP_FLOAT to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	float step_f32;

	/** Float small step value
	 * This is used for "tiny" changes.
	 *
	 * Some hosts and plug-ins expect this to be within 0 and 1.0.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_STEP_FLOAT to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	float step_small_f32;

	/** Float large step value
	 * This is used for "huge" changes.
	 *
	 * Some hosts and plug-ins expect this to be within 0 and 1.0.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_STEP_FLOAT to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	float step_large_f32;

	/** Human-readable name for this parameter.
	 *
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	char name[VST_BUFFER_SIZE_PARAM_LONG_NAME];

	/** Parameter Flags
	 *
	 * Any combination of @ref VST_PARAMETER_FLAG.
	 */
	uint32_t flags;

	/** Minimum Integer value
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_INTEGER_LIMITS to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	int32_t  min_value_i32;

	/** Maximum Integer value
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_INTEGER_LIMITS to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	int32_t  max_value_i32;

	/** Integer Step value
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_STEP_INT to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	int32_t  step_i32;

	/** Short Human-readable label for this parameter.
	 *
	 * I have no idea why this exists?
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	char label[VST_BUFFER_SIZE_PARAM_LABEL];

	/** Display order index.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_INDEX to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	uint16_t index;

	/** Category index
	 *
	 * Must either be 0 for no category, or any number increasing from 1 onwards.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_CATEGORY to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	uint16_t category;

	/** How many parameters are in this category?
	 * This allows the plug-in to specify the same category multiple times.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_CATEGORY to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	uint16_t num_parameters_in_category;

	/** @private Must be zero anyway. */
	uint16_t _unknown_00;

	/** Human-readable name for the category this parameter is in.
	 *
	 * @note Requires @ref VST_PARAMETER_FLAG_CATEGORY to be set.
	 * @note Ignored if @ref VST_EFFECT_FLAG_EDITOR is set.
	 */
	char category_label[VST_BUFFER_SIZE_CATEGORY_LABEL];

	/** @private Reserved for future expansion? */
	char _reserved[16];
};

/*------------------------------------------------------------------------------------------------------------------------*/
/* VST Input Microphones/Output Speakers */
/*------------------------------------------------------------------------------------------------------------------------*/

/** Default speaker types.
 *
 * @todo Are there more?
 */
enum VST_SPEAKER_TYPE {
	/** Mono */
	VST_SPEAKER_TYPE_MONO       = 0,
	/** (Front) Left */
	VST_SPEAKER_TYPE_LEFT       = 1,
	/** (Front) Right */
	VST_SPEAKER_TYPE_RIGHT      = 2,
	/** (Front) Center */
	VST_SPEAKER_TYPE_CENTER     = 3,
	/** LFE / Subwoofer */
	VST_SPEAKER_TYPE_LFE        = 4,
	/** Rear/Surround Left */
	VST_SPEAKER_TYPE_LEFT_REAR  = 5,
	/** Rear/Surround Right */
	VST_SPEAKER_TYPE_RIGHT_REAR = 6,
	/** Side Left */
	VST_SPEAKER_TYPE_LEFT_SIDE  = 10,
	/** Side Right */
	VST_SPEAKER_TYPE_RIGHT_SIDE = 11,

	VST_SPEAKER_TYPE_USER_32 = -32,
	VST_SPEAKER_TYPE_USER_31,
	VST_SPEAKER_TYPE_USER_30,
	VST_SPEAKER_TYPE_USER_29,
	VST_SPEAKER_TYPE_USER_28,
	VST_SPEAKER_TYPE_USER_27,
	VST_SPEAKER_TYPE_USER_26,
	VST_SPEAKER_TYPE_USER_25,
	VST_SPEAKER_TYPE_USER_24,
	VST_SPEAKER_TYPE_USER_23,
	VST_SPEAKER_TYPE_USER_22,
	VST_SPEAKER_TYPE_USER_21,
	VST_SPEAKER_TYPE_USER_20,
	VST_SPEAKER_TYPE_USER_19,
	VST_SPEAKER_TYPE_USER_18,
	VST_SPEAKER_TYPE_USER_17,
	VST_SPEAKER_TYPE_USER_16,
	VST_SPEAKER_TYPE_USER_15,
	VST_SPEAKER_TYPE_USER_14,
	VST_SPEAKER_TYPE_USER_13,
	VST_SPEAKER_TYPE_USER_12,
	VST_SPEAKER_TYPE_USER_11,
	VST_SPEAKER_TYPE_USER_10,
	VST_SPEAKER_TYPE_USER_09,
	VST_SPEAKER_TYPE_USER_08,
	VST_SPEAKER_TYPE_USER_07,
	VST_SPEAKER_TYPE_USER_06,
	VST_SPEAKER_TYPE_USER_05,
	VST_SPEAKER_TYPE_USER_04,
	VST_SPEAKER_TYPE_USER_03,
	VST_SPEAKER_TYPE_USER_02,
	VST_SPEAKER_TYPE_USER_01,


	/* @private Pad to 32-bit. */
	_VST_SPEAKER_TYPE_PAD = (-1l)
};

/** Speaker properties.
 */
struct vst_speaker_properties_t {
	/** Azimuth in Radians
	 * Range: -PI (Left) through 0.0 (Right) to PI (Left)
	 *
	 * @note Must be 10.0 if this is a LFE.
	 */
	float azimuth;

	/** Altitude in Radians
	 * Range: -PI/2 (Bottom) to PI/2 (Top)
	 *
	 * @note Must be 10.0 if this is a LFE.
	 */
	float altitude;

	/** Distance in Meters
	 * range: 0 to +-Infinity
	 *
	 * @note Must be 0.0 if this is a LFE.
	 */
	float distance;

	/** @private Must be zero. */
	float _unknown_00;

	/** Human readable name for this speaker.
	 *
	 * Some hosts will behave weird if you use "L", "R", "C", "Ls", "Rs", "Lc", "Rc", "LFE", "Lfe", "Sl", "Sr", "Cs",
	 * and other 2 to 3 letter short codes. Best not to use those if you like your plug-in in a not-crashy state.
	 */
	char name[VST_BUFFER_SIZE_SPEAKER_NAME];

	/** The type of the speaker
	 *
	 * See VST_SPEAKER_TYPE
	 *
	 * If the above is one of those short codes some host seems to overwrite this with their own. Memory safety is
	 * optional apparently.
	 */
	int32_t type;

	/** @private Reserved for future expansion? */
	uint8_t _reserved[28];
};

/** Known default speaker arrangements.
 *
 * @todo There's got to be a lot more right?
 */
enum VST_SPEAKER_ARRANGEMENT_TYPE {
	/** Custom speaker arrangement.
	 *
	 * Accidentally discovered through random testing.
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_CUSTOM = -2,

	/** Unknown/Empty speaker layout.
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_UNKNOWN = -1,

	/** Mono
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_MONO = 0x00,

	/** Stereo
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_STEREO = 0x01,

	/** Quadraphonic
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_4_0 = 0x0B,

	/** 5.0 (Old Surround)
	 *
	 * L, R, C, RL, RR
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_5_0 = 0x0E,

	/** 5.1 (Old Surround)
	 *
	 * L, R, C, LFE, RL, RR
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_5_1 = 0x0F,

	/** 7.1 (Full Surround)
	 *
	 * L, R, C, LFE, SL, SR, RL, RR
	 */
	VST_SPEAKER_ARRANGEMENT_TYPE_7_1 = 0x17,

	/* @private Pad to 32-bit. */
	_VST_SPEAKER_ARRANGEMENT_TYPE_PAD = (-1l)
};

/** Speaker arrangement definition.
 */
struct vst_speaker_arrangement_t {
	/** Any of @ref VST_SPEAKER_ARRANGEMENT_TYPE.
	 *
	 */
	int32_t type;

	/** Number of channels used in @ref speakers.
	 *
	 * Appears to be limited to @ref VST_MAX_CHANNELS.
	 */
	int32_t channels;

	/** Array of @ref vst_speaker_properties_t with size @ref channels.
	 *
	 * @note This is defined as @ref VST_MAX_CHANNELS as there's currently no host that supports more than that.
	 */
	struct vst_speaker_properties_t speakers[VST_MAX_CHANNELS];
};

/*------------------------------------------------------------------------------------------------------------------------*/
/* VST Input/Output Streams */
/*------------------------------------------------------------------------------------------------------------------------*/

enum VST_STREAM_FLAG {
	/** Ignored?
	 */
	VST_STREAM_FLAG_1ls0 = 1 << 0,

	/** Stream is in Stereo
	 *
	 * Can't be used with VST_STREAM_FLAG_USE_TYPE.
	 */
	VST_STREAM_FLAG_1ls1 = 1 << 1,
	VST_STREAM_FLAG_STEREO = 1 << 1,

	/** Stream is defined by VST_SPEAKER_ARRANGEMENT_TYPE
	 *
	 * Can't be used with VST_STREAM_FLAG_STEREO.
	 */
	VST_STREAM_FLAG_1ls2 = 1 << 2,
	VST_STREAM_FLAG_USE_TYPE = 1 << 2
};

struct vst_stream_properties_t {
	/** Human-readable name for this stream.
	 */
	char name[VST_BUFFER_SIZE_STREAM_NAME];

	/** Stream flags
	 * Any combination of VST_STREAM_FLAG
	 */
	int32_t flags;

	/** Stream arrangement (optional)
	 * See VST_SPEAKER_ARRANGEMENT_TYPE
	 */
	int32_t type;

	/** Human-readable label for this stream.
	 */
	char label[VST_BUFFER_SIZE_STREAM_LABEL];

	/** @private Reserved for future expansion? */
	uint8_t _reserved[48];
};

/*------------------------------------------------------------------------------------------------------------------------*/
/* VST Events */
/*------------------------------------------------------------------------------------------------------------------------*/

/** Available event types.
 *
 * Seems like we can implement our own events for smooth automation and similar.
 */
enum VST_EVENT_TYPE {
	/** Invalid event.
	 *
	 * Crashes the host or plug-in if used.
	 */
	VST_EVENT_TYPE_00 = 0,
	/** @sa VST_EVENT_TYPE_00 */
	VST_EVENT_TYPE_INVALID = 0,

	/** MIDI Event.
	 *
	 * Allows casting @ref vst_event_t to @ref vst_event_midi_t.
	 */
	VST_EVENT_TYPE_01 = 1,
	/** @sa VST_EVENT_TYPE_01 */
	VST_EVENT_TYPE_MIDI = 1,

	VST_EVENT_TYPE_02 = 2,
	VST_EVENT_TYPE_03 = 3,

	/** @todo Seems to be related to parameter automation in some hosts. Structure varies by host, only the first section (vst_event_t) is identical.
	 */
	VST_EVENT_TYPE_04 = 4,

	/** @todo Seems to be related to switch parameter automation in some hosts. Structure varies by host, only the first section (vst_event_t) is identical.
	 */
	VST_EVENT_TYPE_05 = 5,

	/** MIDI SysEx Event.
	 *
	 * Allows casting @ref vst_event_t to @ref vst_event_midi_sysex_t.
	 * See: https://blog.landr.com/midi-sysex/
	 */
	VST_EVENT_TYPE_MIDI_SYSEX = 6
};

/** A generic event.
 *
 * @sa vst_events_t
 * @sa vst_host_supports_t.sendVstEvents
 * @sa vst_host_supports_t.receiveVstEvents
 * @sa vst_effect_supports_t.sendVstEvents
 * @sa vst_effect_supports_t.receiveVstEvents
 * @sa VST_EFFECT_OPCODE_EVENT
 * @sa VST_HOST_OPCODE_EVENT
 */
struct vst_event_t {
	/** What event type was triggered?
	 * Any of @ref VST_EVENT_TYPE
	 */
	int32_t type;

	/** Content size in bytes.
	 *
	 * The size is calculated excluding @ref type and @ref size.
	 * @code{.c}
	 *   vst_event_t myevent;
	 *   myevent.size = sizeof(vst_event_t) - sizeof(vst_event_t.type) - sizeof(vst_event_t.size);
	 * @endcode
	 */
	int32_t size;

	/** Offset of the event relative to some position.
	 *
	 * @todo What position is this relative to?
	 */
	int32_t offset;

	/** @private Set by the event itself. */
	int32_t _pad_00[5];
};

/** A MIDI event.
 *
 * @sa VST_EVENT_TYPE_MIDI
 * @sa vst_host_supports_t.sendVstMidiEvents
 * @sa vst_host_supports_t.receiveVstMidiEvents
 * @sa vst_host_supports_t.sendVstMidiEventFlagIsRealtime
 * @sa vst_effect_supports_t.sendVstMidiEvents
 * @sa vst_effect_supports_t.receiveVstMidiEvents
 */
union vst_event_midi_t {
	/** Shared event structure. */
	struct vst_event_t event;

	struct {
		/** @private */
		int32_t _pad_00[3];

		/** Is this note played in real time (played live)?
		 * Can only ever be 0 (sequencer) or 1 (live).
		 *
		 * @todo Can this be 1 in VST 2.3 and earlier or only 2.4?
		 * @sa vst_host_supports_t.sendVstMidiEventFlagIsRealtime
		 */
		int32_t is_real_time;

		/** Note Length (in samples/frames) of the played note if available.
		 */
		int32_t length;

		/** Some kind of offset (in samples/frames).
		 */
		int32_t offset;

		/** Zero terminated array containing up to 3 bytes of MIDI information.
		 *
		 * @note @ref data[3] is always zero.
		 */
		char data[4];

		/** Tune (in cents) for anything that isn't the default scale.
		 *
		 * Range: -64 to 63
		 */
		int8_t tune;

		/** Note velocity.
		 *
		 * Range: 0 to 127
		 * @todo Are negative values possible?
		 */
		int8_t velocity;

		/** @private Padding */
		char _pad_01[2];
	} midi;
};

/** A MIDI SysEx event.
 *
 * See: https://blog.landr.com/midi-sysex/
 *
 * @sa VST_EVENT_TYPE_MIDI_SYSEX
 * @sa vst_host_supports_t.sendVstMidiEvents
 * @sa vst_host_supports_t.receiveVstMidiEvents
 * @sa vst_host_supports_t.sendVstMidiEventFlagIsRealtime
 * @sa vst_effect_supports_t.sendVstMidiEvents
 * @sa vst_effect_supports_t.receiveVstMidiEvents
 */
union vst_event_midi_sysex_t {
	/** Shared event structure. */
	struct vst_event_t event;

	struct {
		/** @private */
		int32_t _pad_00[4];

		/** Size (in bytes) of the SysEx event.
		 */
		int32_t size;

		/** @private Must be zero. */
		intptr_t _pad_01;

		/** Zero terminated buffer of size @ref size.
		 *
		 * Format is specific to the MIDI device that is used.
		 */
		char* data;

		/** @private Must be zero. */
		intptr_t _pad_02;
	} sysex;
};

/** A collection of events.
 *
 * @sa vst_event_t
 * @sa vst_host_supports_t.sendVstEvents
 * @sa vst_host_supports_t.receiveVstEvents
 * @sa vst_effect_supports_t.sendVstEvents
 * @sa vst_effect_supports_t.receiveVstEvents
 * @sa VST_EFFECT_OPCODE_EVENT
 * @sa VST_HOST_OPCODE_EVENT
 */
struct vst_events_t {
	/** Number of events stored in @ref vst_events_t.events.
	 */
	int32_t count;

	/** @private Reserved, must be zero. */
	int32_t _reserved_00;

	/** An array of pointers to valid @ref vst_event_t structures.
	 *
	 * The size of this array is defined by @ref vst_events_t.count.
	 */
	struct vst_event_t** events;
};

/*------------------------------------------------------------------------------------------------------------------------*/
/* VST Host related Things */
/*------------------------------------------------------------------------------------------------------------------------*/

/* Pre-define vst_effect_t so we can use it below. */
struct vst_effect_t;

/**
 * @sa VST_HOST_OPCODE_ACTIVE_THREAD
 */
enum VST_HOST_ACTIVE_THREAD {
	/** The active thread has no special usage assigned.
	 */
	VST_HOST_ACTIVE_THREAD_UNKNOWN = 0,

	/** The active thread is used for user interface work.
	 */
	VST_HOST_ACTIVE_THREAD_INTERFACE = 1,

	/** The active thread is used for audio processing.
	 */
	VST_HOST_ACTIVE_THREAD_AUDIO = 2,

	/** The active thread is related to events and event handling.
	 *
	 * @sa VST_HOST_OPCODE_EVENT
	 * @sa VST_EFFECT_OPCODE_EVENT
	 */
	VST_HOST_ACTIVE_THREAD_EVENT = 3,

	/** The active thread was created by an effect.
	 */
	VST_HOST_ACTIVE_THREAD_USER = 4,

	/** @private */
	VST_HOST_ACTIVE_THREAD_MAX,
	/** @private */
	_VST_HOST_ACTIVE_THREAD_PAD = (-1l)
};

/** Plug-in to Host Op-Codes
 * These Op-Codes are emitted by the plug-in and the host _may_ handle them or return 0 (false).
 * We have no guarantees about anything actually happening.
 */
enum VST_HOST_OPCODE {
	/** Update automation for a given Parameter
	 *
	 * Must be used to notify the host that the parameter was changed by the user if a custom editor is used.
	 *
	 * @param p_int1 Parameter Index
	 * @param p_float Parameter Value
	 * @return Expected to return... something.
	 */
	VST_HOST_OPCODE_00 = 0x00,
	/** @sa VST_HOST_OPCODE_00 */
	VST_HOST_OPCODE_AUTOMATE = 0x00,
	/** @sa VST_HOST_OPCODE_00 */
	VST_HOST_OPCODE_PARAM_UPDATE = 0x00,

	/** Retrieve the Hosts VST Version.
	 *
	 * @return See VST_VERSION enumeration.
	 */
	VST_HOST_OPCODE_01 = 0x01,
	/** @sa VST_HOST_OPCODE_01 */
	VST_HOST_OPCODE_VST_VERSION = 0x01,

	/** Get the currently selected effect id in container plug-ins.
	 *
	 * Used in combination with @ref VST_EFFECT_CATEGORY_CONTAINER.
	 *
	 * @return The currently selected unique effect id in this container.
	 */
	VST_HOST_OPCODE_02 = 0x02,
	/** @sa VST_HOST_OPCODE_02 */
	VST_HOST_OPCODE_CURRENT_EFFECT_ID = 0x02,

	/** Some sort of idle keep-alive?
	 *
	 * Seems to be called only in editor windows when a modal popup is present.
	 */
	VST_HOST_OPCODE_03 = 0x03,
	/** @sa VST_HOST_OPCODE_03 */
	VST_HOST_OPCODE_KEEPALIVE_OR_IDLE = 0x03,

	/** @todo */
	VST_HOST_OPCODE_04 = 0x04,
	VST_HOST_OPCODE_PIN_CONNECTED = 0x04,

	/*-------------------------------------------------------------------------------- */
	/* VST 2.0 */
	/*--------------------------------------------------------------------------------*/

	/** @todo */
	VST_HOST_OPCODE_05 = 0x05,

	/** @todo */
	VST_HOST_OPCODE_06 = 0x06,
	VST_HOST_OPCODE_WANT_MIDI = 0x06,

	/** @todo */
	VST_HOST_OPCODE_07 = 0x07,
	VST_HOST_OPCODE_GET_TIME = 0x07,

	/** @todo */
	VST_HOST_OPCODE_08 = 0x08,

	/** Send events from plug-in to host.
	 * The host must support receiving events (see @ref vst_host_supports_t.receiveVstEvents) while the plug-in may
	 * optionally signal to the host that it wants to send events to the host (see @ref
	 * vst_effect_supports_t.sendVstEvents).
	 *
	 * @sa vst_event_t
	 * @sa vst_events_t
	 * @sa vst_effect_supports_t.sendVstEvents
	 * @sa vst_host_supports_t.receiveVstEvents
	 * @sa vst_effect_supports_t.sendVstMidiEvents
	 * @sa vst_host_supports_t.receiveVstMidiEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_ptr A valid pointer to a @ref vst_events_t structure.
	 */
	VST_HOST_OPCODE_09 = 0x09,
	/** @sa VST_HOST_OPCODE_09 */
	VST_HOST_OPCODE_EVENT = 0x09,

	/** @todo */
	VST_HOST_OPCODE_0A = 0x0A,

	/** @todo */
	VST_HOST_OPCODE_0B = 0x0B,

	/** @todo */
	VST_HOST_OPCODE_0C = 0x0C,

	/** Notify the host that numInputs/numOutputs/delay/numParams has changed.
	 * Only supported if the host replies @ref VST_STATUS_TRUE to @ref VST_HOST_OPCODE_SUPPORTS query for
	 * @ref vst_host_supports_t.acceptIOChanges.
	 *
	 * @note In VST 2.3 and earlier calling this outside of @ref VST_EFFECT_OPCODE_IDLE may result in a crash.
	 * @note In VST 2.3 and later this may only be called while between @ref VST_EFFECT_OPCODE_PROCESS_END and
	 *       @ref VST_EFFECT_OPCODE_PROCESS_BEGIN.
	 *
	 * @return @ref VST_STATUS_TRUE if supported and handled otherwise @ref VST_STATUS_FALSE.
	 */
	VST_HOST_OPCODE_0D = 0x0D,
	/** @sa VST_HOST_OPCODE_0D */
	VST_HOST_OPCODE_IO_MODIFIED = 0x0D,

	/** @todo */
	VST_HOST_OPCODE_0E = 0x0E,
	VST_HOST_OPCODE_IO_NEED_IDLE = 0x0D,

	/** Request that the host changes the size of the containing window.
	 *
	 * @note (VST 2.x) Available from VST 2.0 onwards.
	 * @sa vst_host_supports_t.sizeWindow
	 *
	 * @param p_int1 Width (in pixels) that we'd like to have.
	 * @param p_int2 Height (in pixels) that we'd like to have.
	 * @param p_ptr Must be zero'd.
	 * @param p_float Must be zero'd.
	 * @return @ref VST_STATUS_TRUE if change was accepted, anything else if not. Do not rely on the return code being 0.
	 */
	VST_HOST_OPCODE_0F = 0x0F,
	/** @sa VST_HOST_OPCODE_0F */
	VST_HOST_OPCODE_EDITOR_RESIZE = 0x0F,

	/** Get the current sample rate the effect should be running at.
	 *
	 * @note (VST 2.x) Available from VST 2.0 onwards.
	 * @sa VST_EFFECT_OPCODE_SET_SAMPLE_RATE
	 *
	 * @return The current sample rate in Hertz.
	 */
	VST_HOST_OPCODE_10 = 0x10,
	/** @sa VST_HOST_OPCODE_10 */
	VST_HOST_OPCODE_GET_SAMPLE_RATE = 0x10,

	/** Get the current block size for the effect.
	 *
	 * @note (VST 2.x) Available from VST 2.0 onwards.
	 * @sa VST_EFFECT_OPCODE_SET_BLOCK_SIZE
	 *
	 * @return The current block size in samples.
	 */
	VST_HOST_OPCODE_11 = 0x11,
	/** @sa VST_HOST_OPCODE_11 */
	VST_HOST_OPCODE_GET_BLOCK_SIZE = 0x11,

	/** Current input latency.
	 * Appears to only work with ASIO input/output devices.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return Current input audio latency in samples.
	 */
	VST_HOST_OPCODE_12 = 0x12,
	/** @sa VST_HOST_OPCODE_12 */
	VST_HOST_OPCODE_INPUT_LATENCY = 0x12,

	/** Current output latency.
	 * Appears to only work with ASIO input/output devices.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return Current output audio latency in samples.
	 */
	VST_HOST_OPCODE_13 = 0x13,
	/** @sa VST_HOST_OPCODE_13 */
	VST_HOST_OPCODE_OUTPUT_LATENCY = 0x13,

	/** Get which effect is attached to the indexed input stream.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4+) Non-functional from VST 2.4 onwards and unimplemented in many earlier hosts.
	 * @param p_int1 Which input stream should be queried?
	 * @return Pointer to a valid @ref vst_effect_t structure or 0.
	 */
	VST_HOST_OPCODE_14 = 0x14,
	/** @sa VST_HOST_OPCODE_14 */
	VST_HOST_OPCODE_INPUT_GET_ATTACHED_EFFECT = 0x14,
	/** @sa VST_HOST_OPCODE_14 */
	VST_HOST_OPCODE_INPUT_STREAM_GET_ATTACHED_EFFECT = 0x14,

	/** Get which effect is attached to the indexed output stream.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4+) Non-functional from VST 2.4 onwards and unimplemented in many earlier hosts.
	 * @param p_int1 Which output stream should be queried?
	 * @return Pointer to a valid @ref vst_effect_t structure or 0.
	 */
	VST_HOST_OPCODE_15 = 0x15,
	/** @sa VST_HOST_OPCODE_15 */
	VST_HOST_OPCODE_OUTPUT_GET_ATTACHED_EFFECT = 0x15,
	/** @sa VST_HOST_OPCODE_15 */
	VST_HOST_OPCODE_OUTPUT_STREAM_GET_ATTACHED_EFFECT = 0x15,

	/** @todo */
	VST_HOST_OPCODE_16 = 0x16,
	VST_HOST_OPCODE_GET_REPLACE_OR_ACCUMULATE = 0x16,

	/** Which thread is the host currently processing this call from?
	 * Useful for memory and thread safety since we can guarantee code paths don't intersect between threads in
	 * compatible hosts. Not so useful in incompatible hosts.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return Any of @ref VST_HOST_ACTIVE_THREAD or 0 if unsupported.
	 */
	VST_HOST_OPCODE_17 = 0x17,
	/** @sa VST_HOST_OPCODE_17 */
	VST_HOST_OPCODE_GET_ACTIVE_THREAD = 0x17,

	/** @todo */
	VST_HOST_OPCODE_18 = 0x18,
	VST_HOST_OPCODE_GET_AUTOMATION_STATE = 0x18,

	/** @todo */
	VST_HOST_OPCODE_19 = 0x19,
	VST_HOST_OPCODE_OFFLINE_START = 0x19,

	/** @todo */
	VST_HOST_OPCODE_1A = 0x1A,
	VST_HOST_OPCODE_OFFLINE_READ = 0x1A,

	/** @todo */
	VST_HOST_OPCODE_1B = 0x1B,
	VST_HOST_OPCODE_OFFLINE_WRITE = 0x1B,

	/** @todo */
	VST_HOST_OPCODE_1C = 0x1C,
	VST_HOST_OPCODE_OFFLINE_GET_CURRENT_PASS = 0x1C,

	/** @todo */
	VST_HOST_OPCODE_1D = 0x1D,
	VST_HOST_OPCODE_OFFLINE_GET_CURRENT_META_PASS = 0x1D,

	/** @todo */
	VST_HOST_OPCODE_1E = 0x1E,
	VST_HOST_OPCODE_OUTPUT_SET_SAMPLE_RATE = 0x1E,

	/** Retrieve the hosts output speaker arrangement.
	 * Seems to always reply with the data provided in @ref VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT p_ptr.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @deprecated (VST 2.4+) Deprecated from VST 2.4 onwards.
	 * @sa vst_speaker_arrangement_t
	 * @sa VST_EFFECT_OPCODE_SET_SPEAKER_ARRANGEMENT
	 * @sa VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 * @sa VST_HOST_OPCODE_GET_INPUT_SPEAKER_ARRANGEMENT
	 * @return Seems to be a valid pointer to @ref vst_speaker_arrangement_t if supported.
	 */
	VST_HOST_OPCODE_1F = 0x1F,
	/** @sa VST_HOST_OPCODE_1F */
	VST_HOST_OPCODE_GET_OUTPUT_SPEAKER_ARRANGEMENT = 0x1F,
	/** @sa VST_HOST_OPCODE_1F */
	VST_HOST_OPCODE_OUTPUT_GET_SPEAKER_ARRANGEMENT = 0x1F,

	/** Retrieve the vendor name into the ptr buffer.
	 *
	 * @param p_ptr A zero terminated char buffer of size @ref VST_BUFFER_SIZE_VENDOR_NAME.
	 */
	VST_HOST_OPCODE_20 = 0x20,
	/** @sa VST_HOST_OPCODE_20 */
	VST_HOST_OPCODE_VENDOR_NAME = 0x20,

	/** Retrieve the product name into the ptr buffer.
	 *
	 * @param p_ptr A zero terminated char buffer of size @ref VST_BUFFER_SIZE_PRODUCT_NAME.
	 */
	VST_HOST_OPCODE_21 = 0x21,
	/** @sa VST_HOST_OPCODE_21 */
	VST_HOST_OPCODE_PRODUCT_NAME = 0x21,

	/** Retrieve the vendor version in return value.
	 *
	 * @return Version.
	 */
	VST_HOST_OPCODE_22 = 0x22,
	/** @sa VST_HOST_OPCODE_22 */
	VST_HOST_OPCODE_VENDOR_VERSION = 0x22,

	/** User defined OP Code, for custom interaction.
	 *
	 */
	VST_HOST_OPCODE_23 = 0x23,
	/** @sa VST_HOST_OPCODE_23 */
	VST_HOST_OPCODE_CUSTOM = 0x23,

	/** @todo */
	VST_HOST_OPCODE_24 = 0x24,

	/** Check if the host supports a certain feature.
	 *
	 * @param p_ptr `char[...]` Zero terminated string for which feature we want to support.
	 * @return @ref VST_STATUS_TRUE if the feature is supported otherwise @ref VST_STATUS_FALSE.
	 */
	VST_HOST_OPCODE_25 = 0x25,
	/** @sa VST_HOST_OPCODE_25 */
	VST_HOST_OPCODE_SUPPORTS = 0x25,

	/** What language is the host in?
	 *
	 * @return 1 if english, 2 if german. more possible?
	 */
	VST_HOST_OPCODE_26 = 0x26,
	/** @sa VST_HOST_OPCODE_26 */
	VST_HOST_OPCODE_LANGUAGE = 0x26,

	/** Crash the host if p_ptr isn't nullptr.
	 * @todo
	 */
	VST_HOST_OPCODE_27 = 0x27,

	/** Crash the host if p_ptr isn't nullptr.
	 * @todo
	 */
	VST_HOST_OPCODE_28 = 0x28,

	/** Retrieve the directory of the effect that emitted this.
	 * The returned value seems to be unchanged for container plug-ins.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return (Windows) A zero-terminated char buffer of unknown size.
	 * @return (MacOS) A valid FSSpec structure.
	 */
	VST_HOST_OPCODE_29 = 0x29,
	/** @sa VST_HOST_OPCODE_29 */
	VST_HOST_OPCODE_GET_EFFECT_DIRECTORY = 0x29,

	/** Refresh everything related to the effect that called this.
	 * This includes things like parameters, programs, banks, windows, files, meters, streams, sample rate, block size,
	 * and a lot more. Anything that has to do with the effect should be refreshed when the effect calls this.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	VST_HOST_OPCODE_2A = 0x2A,
	/** @sa VST_HOST_OPCODE_2A */
	VST_HOST_OPCODE_EDITOR_UPDATE = 0x2A,
	/** @sa VST_HOST_OPCODE_2A */
	VST_HOST_OPCODE_REFRESH = 0x2A,

	/*--------------------------------------------------------------------------------*/
	/* VST 2.1 */
	/*--------------------------------------------------------------------------------*/

	/** Notify host that a parameter is being edited.
	 * "Locks" the parameter from being edited in compatible hosts.
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @param p_int1 Parameter index.
	 */
	VST_HOST_OPCODE_2B = 0x2B,
	/** @sa VST_HOST_OPCODE_2B */
	VST_HOST_OPCODE_PARAM_START_EDIT = 0x2B,
	/** @sa VST_HOST_OPCODE_2B */
	VST_HOST_OPCODE_PARAM_LOCK = 0x2B,

	/** Notify host that parameter is no longer being edited.
	 * "Unlocks" the parameter for further editing in compatible hosts. Remember to call the @ref VST_HOST_OPCODE_PARAM_UPDATE
	 * op-code afterwards so that the host knows it needs to update its automation data.
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @sa VST_HOST_OPCODE_PARAM_UPDATE
	 * @param p_int1 Parameter index.
	 */
	VST_HOST_OPCODE_2C = 0x2C,
	/** @sa VST_HOST_OPCODE_2C */
	VST_HOST_OPCODE_PARAM_STOP_EDIT = 0x2C,
	/** @sa VST_HOST_OPCODE_2C */
	VST_HOST_OPCODE_PARAM_UNLOCK = 0x2C,

	/** Crash the host depending on what p_ptr is pointing at.
	 * @todo
	 */
	VST_HOST_OPCODE_2D = 0x2D,

	/*--------------------------------------------------------------------------------*/
	/* VST 2.2 */
	/*--------------------------------------------------------------------------------*/

	/** Crash the host depending on what p_ptr is pointing at.
	 * @todo
	 */
	VST_HOST_OPCODE_2E = 0x2E,

	/** Crash the host depending on what p_ptr is pointing at.
	 * @todo
	 */
	VST_HOST_OPCODE_2F = 0x2F,

	/**
	 * When queried by the plug-in shortly after @ref VST_EFFECT_OPCODE_PROGRAM_LOAD it often crashes compatible hosts
	 * with a memory access exception. This exception can be controlled with p_ptr but it's unclear what that is
	 * pointing at so far. In the event that it doesn't crash the memory address we pointed at changes to a path.
	 *
	 * @todo Figure out what p_ptr is.
	 * @note (VST 2.2+) Available from VST 2.2 onwards.
	 * @deprecated (VST 2.4+) Deprecated from VST 2.4 onwards.
	 * @param p_ptr A pointer to something
	 * @todo
	 */
	VST_HOST_OPCODE_30 = 0x30,

	/*--------------------------------------------------------------------------------*/
	/* VST 2.3 */
	/*--------------------------------------------------------------------------------*/

	/** Retrieve the hosts input speaker arrangement.
	 * Seems to always reply with the data provided in @ref VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT p_int2.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @deprecated (VST 2.4+) Deprecated from VST 2.4 onwards.
	 * @sa vst_speaker_arrangement_t
	 * @sa VST_EFFECT_OPCODE_SET_SPEAKER_ARRANGEMENT
	 * @sa VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 * @sa VST_HOST_OPCODE_GET_OUTPUT_SPEAKER_ARRANGEMENT
	 * @return Seems to be a valid pointer to @ref vst_speaker_arrangement_t if supported.
	 */
	VST_HOST_OPCODE_31 = 0x31,
	/** @sa VST_HOST_OPCODE_31 */
	VST_HOST_OPCODE_GET_INPUT_SPEAKER_ARRANGEMENT = 0x31,
	/** @sa VST_HOST_OPCODE_31 */
	VST_HOST_OPCODE_INPUT_GET_SPEAKER_ARRANGEMENT = 0x31,

	/** @private Highest known OPCODE. */
	VST_HOST_OPCODE_MAX,

	/** @private Force as 32-bit unsigned integer in compatible compilers. */
	_VST_HOST_OPCODE_PAD = (-1l)
};

#if (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 202002L)
/** Plug-in to Host support checks
 *
 * Provided as `char* p_ptr` in the VST_EFFECT_OPCODE_SUPPORTS op code.
 *
 * Harvested via strings command and just checking what hosts actually responded to.
 */
struct vst_host_supports_t {
	/** Does the host support modifying input/output/params/delay when programs, banks or parameters are changed?
	 * This only means that the host supports this inside of @ref VST_EFFECT_OPCODE_IDLE (VST 2.3 or earlier) or outside
	 * of a @ref VST_EFFECT_OPCODE_PROCESS_BEGIN and @ref VST_EFFECT_OPCODE_PROCESS_END group.
	 *
	 * Signals that the host supports the following:
	 * - @ref VST_HOST_OPCODE_IO_MODIFIED
	 *
	 * @return @ref VST_STATUS_TRUE if it supports it.
	 */
	const char* acceptIOChanges;

	/** Is the host using process begin/end instead of idle?
	 * The host may opt to emit @ref VST_EFFECT_OPCODE_IDLE or @ref VST_EFFECT_OPCODE_PROCESS_BEGIN and
	 * @ref VST_EFFECT_OPCODE_PROCESS_END when running in VST 2.3 compatibility mode.
	 *
	 * @sa VST_EFFECT_OPCODE_PROCESS_BEGIN
	 * @sa VST_EFFECT_OPCODE_PROCESS_END
	 * @sa VST_EFFECT_OPCODE_IDLE
	 * @note (VST 2.3) Available from VST 2.3 onwards.
	 * @deprecated (VST 2.4) This behavior is the default in VST 2.4 and later.
	 * @return @ref VST_STATUS_TRUE if it supports it.
	 */
	const char* startStopProcess;

	/** Does the host support container plug-ins?
	 *
	 * @note Is shell a reference to Windows shell menus?
	 * @sa VST_HOST_OPCODE_CURRENT_EFFECT_ID
	 * @sa VST_EFFECT_OPCODE_CONTAINER_NEXT_EFFECT_ID
	 * @return @ref VST_STATUS_TRUE if the host supports it _and_ the current plug-in is a container plug-in.
	 */
	const char* shellCategory;

	/** Can we request that the host changes the editor window size?
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @sa VST_HOST_OPCODE_EDITOR_RESIZE
	 */
	const char* sizeWindow;

	/** Host can send events to plug-in.
	 *
	 * @sa vst_effect_supports_t.receiveVstEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* sendVstEvents;

	/** Host can receive events from plug-in.
	 *
	 * @sa vst_effect_supports_t.sendVstEvents
	 * @sa VST_HOST_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* receiveVstEvents;

	/** Host can send MIDI events to plug-in.
	 *
	 * @sa vst_effect_supports_t.receiveVstMidiEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @sa vst_effect_midi_t
	 * @sa vst_effect_midi_sysex_t
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* sendVstMidiEvent;

	/** Host can receive MIDI events from plug-in.
	 *
	 * @sa vst_effect_supports_t.sendVstMidiEvents
	 * @sa VST_HOST_OPCODE_EVENT
	 * @sa vst_effect_midi_t
	 * @sa vst_effect_midi_sysex_t
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* receiveVstMidiEvent;

	/** Host can send real time (live) MIDI events to plug-in.
	 *
	 * @sa vst_host_supports_t.sendVstMidiEvent
	 * @sa vst_effect_supports_t.receiveVstMidiEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @sa vst_effect_midi_t
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* sendVstMidiEventFlagIsRealtime;

	const char* sendVstTimeInfo;
	const char* reportConnectionChanges; // Seems related to speakers?

	const char* offline;

	const char* editFile;
	const char* openFileSelector;
	const char* closeFileSelector;
} /** @private */ vst_host_supports = {
	.acceptIOChanges = "acceptIOChanges",
	.startStopProcess = "startStopProcess",
	.shellCategory = "shellCategory",
	.sizeWindow = "sizeWindow",
	.sendVstEvents = "sendVstEvents",
	.receiveVstEvents = "receiveVstEvents",
	.sendVstMidiEvent = "sendVstMidiEvent",
	.receiveVstMidiEvent = "receiveVstMidiEvent",
	.sendVstMidiEventFlagIsRealtime = "sendVstMidiEventFlagIsRealtime",
	.sendVstTimeInfo = "sendVstTimeInfo",
	.reportConnectionChanges = "reportConnectionChanges",
	.offline = "offline",
	.editFile = "editFile",
	.openFileSelector = "openFileSelector",
	.closeFileSelector = "closeFileSelector",
};
#endif

/** Plug-in to Host callback
 *
 * The plug-in may call this to attempt to change things on the host side. The host side is free to ignore all requests, annoyingly enough.
 *
 * @param opcode See VST_HOST_OPCODE
 * @param p_str Zero terminated string or null on call.
 * @return ?
 */
typedef intptr_t (VST_FUNCTION_INTERFACE *vst_host_callback_t)(struct vst_effect_t* plugin, int32_t opcode, int32_t p_int1, int64_t p_int2, const char* p_str, float p_float);

//------------------------------------------------------------------------------------------------------------------------
// VST Plug-in/Effect related Things
//------------------------------------------------------------------------------------------------------------------------

/** Magic Number identifying a VST 2.x plug-in structure
 *
 * @sa vst_effect_t.magic_numer
 */
#define VST_MAGICNUMBER VST_FOURCC('V', 's', 't', 'P')

/** Default VST 2.x Sample Rate
 * All VST 2.x hosts expect you to initialize your plug-in to these default values.
 *
 * @sa VST_EFFECT_OPCODE_SET_SAMPLE_RATE
 */
#define VST_DEFAULT_SAMPLE_RATE 44100.0f

/** Default VST 2.x Block Size
 * All VST 2.x hosts expect you to initialize your plug-in to these default values.
 *
 * @sa VST_EFFECT_OPCODE_SET_BLOCK_SIZE
 */
#define VST_DEFAULT_BLOCK_SIZE 1024

/** Plug-in Categories
 * Pre-defined category grouping that also affect host behavior when handling the plug-in. This is not just a UI/UX
 * thing, it actually affects what plug-ins can do, so place your plug-in into the correct category.
 *
 */
enum VST_EFFECT_CATEGORY {
	VST_EFFECT_CATEGORY_UNCATEGORIZED = 0x00,

	/** Generic Effects
	 * Examples: Distortion, Pitch Shift, ...
	 *
	 * Supports: Delay (Optional), Tail Samples, MIDI
	 */
	VST_EFFECT_CATEGORY_01            = 0x01,
	/** @sa VST_EFFECT_CATEGORY_01 */
	VST_EFFECT_CATEGORY_EFFECT        = 0x01,

	/** Instruments
	 * Examples: Instruments, Synths, Samplers, ...
	 *
	 * Supports: Delay (Optional), Tail Samples, MIDI
	 */
	VST_EFFECT_CATEGORY_02            = 0x02,
	/** @sa VST_EFFECT_CATEGORY_02 */
	VST_EFFECT_CATEGORY_INSTRUMENT    = 0x02,

	/** Metering
	 * Examples: Loudness Meters, Volume Analysis, ...
	 *
	 * Supports: Tail Samples, MIDI
	 * @note Delay causes crashes in some hosts. Fun.
	 */
	VST_EFFECT_CATEGORY_03            = 0x03,
	/** @sa VST_EFFECT_CATEGORY_03 */
	VST_EFFECT_CATEGORY_METERING      = 0x03,

	/** Mastering
	 * Examples: Compressors, Limiters, ...
	 *
	 * Supports: Delay, Tail Samples (optional), MIDI
	 */
	VST_EFFECT_CATEGORY_04            = 0x04,
	/** @sa VST_EFFECT_CATEGORY_04 */
	VST_EFFECT_CATEGORY_MASTERING     = 0x04,

	/** Spatializers
	 * Examples: Channel Panning, Expanders, ...
	 *
	 * Supports: Tail Samples (optional), MIDI
	 */
	VST_EFFECT_CATEGORY_05            = 0x05,
	/** @sa VST_EFFECT_CATEGORY_05 */
	VST_EFFECT_CATEGORY_SPATIAL       = 0x05,

	/** Delay/Echo
	 * Examples: Echo, Reverb, Room Simulation, Delay, ...
	 *
	 * Supports: Delay, Tail Samples, MIDI
	 */
	VST_EFFECT_CATEGORY_06            = 0x06,
	/** @sa VST_EFFECT_CATEGORY_06 */
	VST_EFFECT_CATEGORY_DELAY_OR_ECHO = 0x06,

	/** External Processing.
	 * This VST effect is an interface to an external device and requires special handling.
	 *
	 * @todo What does this actually support? Is it even still supported?
	 */
	VST_EFFECT_CATEGORY_07            = 0x07,
	/** @sa VST_EFFECT_CATEGORY_07 */
	VST_EFFECT_CATEGORY_EXTERNAL = 0x07,

	/** Restoration
	 * Examples: Noise Filtering, Upsamplers, ...
	 *
	 * Supports: Delay, Tail Samples, MIDI
	 * @note Some DAWs allocate additional processing time to these.
	 */
	VST_EFFECT_CATEGORY_08            = 0x08,
	/** @sa VST_EFFECT_CATEGORY_08 */
	VST_EFFECT_CATEGORY_RESTORATION   = 0x08,

	/** Offline Processing
	 * Examples: Nothing
	 * Supports: Nothing
	 */
	VST_EFFECT_CATEGORY_09            = 0x09,
	/** @sa VST_EFFECT_CATEGORY_09 */
	VST_EFFECT_CATEGORY_OFFLINE       = 0x09, // Offline Processing VST? Seems to receive all audio data prior to playback.

	/** Container Plug-in
	 * This plug-in contains multiple effects in one and requires special handling on both sides.
	 *
	 * Host handling:
	 * @code{.c}
	 * uint32_t current_select_id;
	 *
	 * // ... in intptr_t vst_host_callback(vst_effect_t* plugin, VST_HOST_OPCODE opcode, ...)
	 *     case VST_HOST_OPCODE_SUPPORTS: {
	 *       char* text = (char*)p_ptr;
	 *       // The plug-in may ask the host if it even supports containers at all and changes behavior if we don't.
	 *       if (text && strcmp(text, vst_host_supports.shellCategory) == 0) {
	 *         return VST_STATUS_TRUE;
	 *       }
	 *     }
	 *     case VST_HOST_OPCODE_CURRENT_EFFECT_ID:
	 *       return current_selected_id;
	 * // ...
	 *
	 * // ... in whatever you use to load plug-ins ...
	 *   current_select_id;
	 *   vst_effect_t* plugin = plugin_main(&vst_host_callback);
	 *   int32_t plugin_category = plugin->control(plugin, VST_EFFECT_OPCODE_CATEGORY, 0, 0, 0, 0)
	 *   if (plugin_category == VST_EFFECT_CATEGORY_CONTAINER) {
	 *     char effect_name[VST_BUFFER_SIZE_EFFECT_NAME] effect_name;
	 *     int32_t effect_id;
	 *     // Iterate over all contained effects.
	 *     while ((effect_id = plugin->control(plugin, VST_EFFECT_OPCODE_CONTAINER_NEXT_EFFECT_ID, 0, 0, effect_name, 0)) != 0) {
	 *       // Contained effects must be named as far as I can tell.
	 *       if (effect_name[0] != 0) {
	 *          // Do some logic that does the necessary things to list these in the host.
	 *       }
	 *     }
	 *   } else {
	 *     // Do things to list only this plugin in the host.
	 *   }
	 * // ...
	 * @endcode
	 *
	 * Plug-in handling:
	 * @code{.c}
	 * // ... in vst_effect for the container
	 *   size_t current_effect_idx;
	 *   int32_t effect_list[] = {
	 *     // ... list of effect ids.
	 *   }
	 * // ... in control(...)
	 *     case VST_EFFECT_OPCODE_CONTAINER_NEXT_EFFECT_ID:
	 *       // Make sure current_effect_idx doesn't exceed the maximum.
	 *       if (current_effect_idx > ARRAYSIZEOF(effect_list)) {
	 *         current_effect_idx;
	 *         return 0;
	 *       }
	 *       // Some code that turns effect indices into names to store into p_ptr.
	 *       return effect_list[current_effect_idx++]; // Return the effect id.
	 * // ...
	 *
	 * VST_ENTRYPOINT {
	 *   // Ensure the host VST 2.x compatible.
	 *   int32_t vst_version = callback(nullptr, VST_HOST_OPCODE_VST_VERSION, 0, 0, 0, 0);
	 *   if (vst_version == 0) {
	 *     return 0; // It's not so we exit early.
	 *   }
	 *
	 *   // Check if the host wants
	 *   int32_t effect_id = callback(nullptr, VST_HOST_OPCODE_CURRENT_EFFECT_ID, 0, 0, 0);
	 *   if (effect_id == 0) {
	 *     // ... logic specific to making the container.
	 *     return new vst_container_effect();
	 *   } else {
	 *     // ... logic specific to make sub effects
	 *     return new vst_sub_effect();
	 *   }
	 * }
	 *
	 * // ...
	 * @endcode
	 */
	VST_EFFECT_CATEGORY_0A            = 0x0A,
	/** @sa VST_EFFECT_CATEGORY_0A */
	VST_EFFECT_CATEGORY_CONTAINER     = 0x0A,

	/** Waveform Generators
	 * Examples: Sine Wave Generator, ...
	 * Supports: Delay, Tail Samples
	 *
	 * I don't know why this exists, there's only one plug-in that has it and all it does is generate a 400hz sine wave.
	 *
	 * @sa VST_EFFECT_CATEGORY_INSTRUMENT
	 */
	VST_EFFECT_CATEGORY_0B            = 0x0B,
	/** @sa VST_EFFECT_CATEGORY_0B */
	VST_EFFECT_CATEGORY_WAVEGENERATOR = 0x0B,

	/** @private */
	VST_EFFECT_CATEGORY_MAX, // Not part of specification, marks maximum category.

	/** @private */
	_VST_EFFECT_CATEGORY_PAD = (-1l)
};

/** Effect Flags
 */
enum VST_EFFECT_FLAG {
	/** Effect provides a custom editor.
	 * The host will not provide a generic editor interface and expects @ref VST_EFFECT_OPCODE_EDITOR_OPEN and
	 * @ref VST_EFFECT_OPCODE_EDITOR_CLOSE to work as expected. We are in charge of notifying the host about various
	 * things like which parameter is in focus and stuff.
	 *
	 * @sa VST_EFFECT_OPCODE_EDITOR_GET_RECT
	 * @sa VST_EFFECT_OPCODE_EDITOR_OPEN
	 * @sa VST_EFFECT_OPCODE_EDITOR_CLOSE
	 * @sa VST_EFFECT_OPCODE_EDITOR_DRAW
	 * @sa VST_EFFECT_OPCODE_EDITOR_MOUSE
	 * @sa VST_EFFECT_OPCODE_EDITOR_KEYBOARD
	 * @sa VST_EFFECT_OPCODE_EDITOR_KEEP_ALIVE
	 * @sa VST_EFFECT_OPCODE_EDITOR_VKEY_DOWN
	 * @sa VST_EFFECT_OPCODE_EDITOR_VKEY_UP
	 * @sa VST_HOST_OPCODE_EDITOR_UPDATE
	 * @sa VST_HOST_OPCODE_PARAM_START_EDIT
	 * @sa VST_HOST_OPCODE_PARAM_STOP_EDIT
	 * @sa VST_HOST_OPCODE_PARAM_UPDATE
	 */
	VST_EFFECT_FLAG_1ls0 = 1 << 0,
	/** @sa VST_EFFECT_FLAG_1ls0 */
	VST_EFFECT_FLAG_EDITOR = 1 << 0,

	//1 << 1,
	//1 << 2, // Only seen when the plug-in responds to VST_EFFECT_OPCODE_09. Seems to be ignored by hosts entirely.
	//1 << 3, // Only seen when the plug-in behaves differently in mono mode. Seems to be ignored by hosts entirely.

	/** Effect uses process_float.
	 *
	 * @sa vst_effect_t.process_float
	 * @sa vst_effect_process_float_t
	 * @deprecated (VST 2.4) Must be set in VST 2.4 and later or the host should fail to load the plug-in.
	 */
	VST_EFFECT_FLAG_1ls4 = 1 << 4,
	/** @sa VST_EFFECT_FLAG_1ls4 */
	VST_EFFECT_FLAG_SUPPORTS_FLOAT = 1 << 4,

	/** Effect supports saving/loading programs/banks from unformatted chunk data.
	 * When not set some sort of format is expected that I've yet to decipher.
	 *
	 * @sa VST_EFFECT_OPCODE_GET_CHUNK_DATA
	 * @sa VST_EFFECT_OPCODE_SET_CHUNK_DATA
	 */
	VST_EFFECT_FLAG_1ls5 = 1 << 5,
	/** @sa VST_EFFECT_FLAG_1ls5 */
	VST_EFFECT_FLAG_CHUNKS = 1 << 5,

	//1 << 6,
	//1 << 7,

	/** Effect is an Instrument/Generator
	 *
	 * This must be set in addition to @ref VST_EFFECT_CATEGORY_INSTRUMENT otherwise instruments don't work right.
	 * @note (VST 2.x) Flag is new to VST 2.x and later.
	 */
	VST_EFFECT_FLAG_1ls8 = 1 << 8,
	/** @sa VST_EFFECT_FLAG_1ls8 */
	VST_EFFECT_FLAG_INSTRUMENT = 1 << 8,

	/** Effect does not produce tail samples when the input is silent.
	 *
	 * Not to be confused with choosing to tell the host there is no tail.
	 * @sa VST_EFFECT_OPCODE_GET_TAIL_SAMPLES
	 * @note (VST 2.x) Flag is new to VST 2.x and later.
	 */
	VST_EFFECT_FLAG_1ls9 = 1 << 9,
	/** @sa VST_EFFECT_FLAG_1ls9 */
	VST_EFFECT_FLAG_SILENT_TAIL = 1 << 9,

	//1 << 10,
	//1 << 11,

	/** Effect supports process_double.
	 * The host can freely choose between process_float and process_double as required.
	 *
	 * @note (VST 2.4) Available in VST 2.4 and later only.
	 * @sa vst_effect_t.process_double
	 * @sa vst_effect_process_double_t
	 */
	VST_EFFECT_FLAG_1ls12 = 1 << 12,
	/** @sa VST_EFFECT_FLAG_1ls12 */
	VST_EFFECT_FLAG_SUPPORTS_DOUBLE = 1 << 12
};

/** Host to Plug-in Op-Codes
 * These Op-Codes are emitted by the host and we must either handle them or return 0 (false).
 */
enum VST_EFFECT_OPCODE {
	/** Create/Initialize the effect (if it has not been created already).
	 *
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_00         = 0x00,
	/** @sa VST_EFFECT_OPCODE_00 */
	VST_EFFECT_OPCODE_CREATE     = 0x00,
	/** @sa VST_EFFECT_OPCODE_00 */
	VST_EFFECT_OPCODE_INITIALIZE = 0x00,

	/** Destroy the effect (if there is any) and free its memory.
	 *
	 * This should destroy the actual object created by VST_ENTRYPOINT.
	 *
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_01      = 0x01,
	/** @sa VST_EFFECT_OPCODE_01 */
	VST_EFFECT_OPCODE_DESTROY = 0x01,

	/** Set which program number is currently select.
	 *
	 * @param p_int2 The program number to set. Can be negative for some reason.
	 */
	VST_EFFECT_OPCODE_02 = 0x02,
	/** @sa VST_EFFECT_OPCODE_02 */
	VST_EFFECT_OPCODE_SET_PROGRAM = 0x02,
	/** @sa VST_EFFECT_OPCODE_02 */
	VST_EFFECT_OPCODE_PROGRAM_SET = 0x02,

	/** Get currently selected program number.
	 *
	 * @return The currently set program number. Can be negative for some reason.
	 */
	VST_EFFECT_OPCODE_03 = 0x03,
	/** @sa VST_EFFECT_OPCODE_03 */
	VST_EFFECT_OPCODE_GET_PROGRAM = 0x03,
	/** @sa VST_EFFECT_OPCODE_03 */
	VST_EFFECT_OPCODE_PROGRAM_GET = 0x03,

	/** Set the name of the currently selected program.
	 *
	 * @param p_ptr `const char[VST_BUFFER_SIZE_PROGRAM_NAME]` Zero terminated string.
	 */
	VST_EFFECT_OPCODE_04 = 0x04,
	/** @sa VST_EFFECT_OPCODE_04 */
	VST_EFFECT_OPCODE_SET_PROGRAM_NAME = 0x04,
	/** @sa VST_EFFECT_OPCODE_04 */
	VST_EFFECT_OPCODE_PROGRAM_SET_NAME = 0x04,

	/** Get the name of the currently selected program.
	 *
	 * @param p_ptr `char[VST_BUFFER_SIZE_PROGRAM_NAME]` Zero terminated string.
	 */
	VST_EFFECT_OPCODE_05 = 0x05,
	/** @sa VST_EFFECT_OPCODE_05 */
	VST_EFFECT_OPCODE_GET_PROGRAM_NAME = 0x05,
	/** @sa VST_EFFECT_OPCODE_05 */
	VST_EFFECT_OPCODE_PROGRAM_GET_NAME = 0x05,

	/** Get the value? label for the parameter.
	 *
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[VST_BUFFER_SIZE_PARAM_LABEL]' Zero terminated string.
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_06             = 0x06,
	/** @sa VST_EFFECT_OPCODE_06 */
	VST_EFFECT_OPCODE_PARAM_GETLABEL = 0x06,
	/** @sa VST_EFFECT_OPCODE_06 */
	VST_EFFECT_OPCODE_PARAM_GET_LABEL = 0x06,
	/** @sa VST_EFFECT_OPCODE_06 */
	VST_EFFECT_OPCODE_PARAM_LABEL = 0x06,

	/** Get the string representing the value for the parameter.
	 *
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[VST_BUFFER_SIZE_PARAM_VALUE]' Zero terminated string.
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_07             = 0x07,
	/** @sa VST_EFFECT_OPCODE_07 */
	VST_EFFECT_OPCODE_PARAM_GETVALUE = 0x07,
	/** @sa VST_EFFECT_OPCODE_07 */
	VST_EFFECT_OPCODE_PARAM_GET_VALUE = 0x07,
	/** @sa VST_EFFECT_OPCODE_07 */
	VST_EFFECT_OPCODE_PARAM_VALUE = 0x07,
	/** @sa VST_EFFECT_OPCODE_07 */
	VST_EFFECT_OPCODE_PARAM_VALUE_TO_STRING = 0x07,

	/** Get the name for the parameter.
	 *
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[VST_BUFFER_SIZE_PARAM_NAME]' Zero terminated string.
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_08            = 0x08,
	/** @sa VST_EFFECT_OPCODE_08 */
	VST_EFFECT_OPCODE_PARAM_GETNAME = 0x08,
	/** @sa VST_EFFECT_OPCODE_08 */
	VST_EFFECT_OPCODE_PARAM_GET_NAME = 0x08,
	/** @sa VST_EFFECT_OPCODE_08 */
	VST_EFFECT_OPCODE_PARAM_NAME = 0x08,

	/**
	 *
	 * @deprecated: (VST 2.3+) Not used in VST 2.3 or later.
	 * @todo
	 */
	VST_EFFECT_OPCODE_09 = 0x09,

	/** Set the new sample rate for the plugin to use.
	 *
	 * @param p_float New sample rate as a float (double on 64-bit because register upgrades).
	 */
	VST_EFFECT_OPCODE_0A              = 0x0A,
	/** @sa VST_EFFECT_OPCODE_0A */
	VST_EFFECT_OPCODE_SETSAMPLERATE   = 0x0A,
	/** @sa VST_EFFECT_OPCODE_0A */
	VST_EFFECT_OPCODE_SET_SAMPLE_RATE = 0x0A,

	/** Sets the block size, which is the maximum number of samples passed into the effect via process calls.
	 *
	 * @param p_int2 The maximum number of samples to be passed in.
	 */
	VST_EFFECT_OPCODE_0B             = 0x0B,
	/** @sa VST_EFFECT_OPCODE_0B */
	VST_EFFECT_OPCODE_SETBLOCKSIZE   = 0x0B,
	/** @sa VST_EFFECT_OPCODE_0B */
	VST_EFFECT_OPCODE_SET_BLOCK_SIZE = 0x0B,

	/** Effect processing should be suspended/paused or resumed/unpaused.
	 *
	 * Unclear if this is should result in a flush of buffers. In VST 2.3+ this is quite clear as we get process
	 * begin/end.
	 *
	 * @param p_int2 @ref VST_STATUS_FALSE if the effect should suspend processing, @ref VST_STATUS_TRUE if it should
	 *               resume.
	 */
	VST_EFFECT_OPCODE_0C      = 0x0C,
	/** @sa VST_EFFECT_OPCODE_0C */
	VST_EFFECT_OPCODE_PAUSE_UNPAUSE = 0x0C,
	/** @sa VST_EFFECT_OPCODE_0C */
	VST_EFFECT_OPCODE_SUSPEND_RESUME = 0x0C,
	/** @sa VST_EFFECT_OPCODE_0C */
	VST_EFFECT_OPCODE_SUSPEND = 0x0C,

	/** Retrieve the client rect size of the plugins window.
	 * If no window has been created, returns the default rect.
	 *
	 * @param p_ptr Pointer of type 'struct vst_rect_t*'.
	 * @return On success, returns 1 and updates p_ptr to the rect. On failure, returns 0.
	 */
	VST_EFFECT_OPCODE_0D             = 0x0D,
	/** @sa VST_EFFECT_OPCODE_0D */
	VST_EFFECT_OPCODE_WINDOW_GETRECT = 0x0D,
	/** @sa VST_EFFECT_OPCODE_0D */
	VST_EFFECT_OPCODE_EDITOR_RECT = 0x0D,
	/** @sa VST_EFFECT_OPCODE_0D */
	VST_EFFECT_OPCODE_EDITOR_GET_RECT = 0x0D,

	/** Create the window for the plugin.
	 *
	 * @param p_ptr HWND of the parent window.
	 * @return 0 on failure, or HWND on success.
	 */
	VST_EFFECT_OPCODE_0E            = 0x0E,
	/** @sa VST_EFFECT_OPCODE_0E */
	VST_EFFECT_OPCODE_WINDOW_CREATE = 0x0E,
	/** @sa VST_EFFECT_OPCODE_0E */
	VST_EFFECT_OPCODE_EDITOR_OPEN = 0x0E,

	/** Destroy the plugins window.
	 *
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_0F             = 0x0F,
	/** @sa VST_EFFECT_OPCODE_0F */
	VST_EFFECT_OPCODE_WINDOW_DESTROY = 0x0F,
	/** @sa VST_EFFECT_OPCODE_0F */
	VST_EFFECT_OPCODE_EDITOR_CLOSE = 0x0F,

	/** Window Draw Event?
	 *
	 * Ocasionally called simultaneously as WM_DRAW on windows.
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 *
	 * @note Appears to be Mac OS exclusive.
	 *
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_10 = 0x10,
	/** @sa VST_EFFECT_OPCODE_10 */
	VST_EFFECT_OPCODE_WINDOW_DRAW = 0x10,
	/** @sa VST_EFFECT_OPCODE_10 */
	VST_EFFECT_OPCODE_EDITOR_DRAW = 0x10,

	/** Window Mouse Event?
	 *
	 * Called at the same time mouse events happen.
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 *
	 * @note Appears to be Mac OS exclusive.
	 *
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_11 = 0x11,
	/** @sa VST_EFFECT_OPCODE_11 */
	VST_EFFECT_OPCODE_WINDOW_MOUSE = 0x11,
	/** @sa VST_EFFECT_OPCODE_11 */
	VST_EFFECT_OPCODE_EDITOR_MOUSE = 0x11,

	/** Window Keyboard Event?
	 *
	 * Called at the same time keyboard events happen.
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 *
	 * @note Appears to be Mac OS exclusive.
	 *
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_12 = 0x12,
	/** @sa VST_EFFECT_OPCODE_12 */
	VST_EFFECT_OPCODE_WINDOW_KEYBOARD = 0x12,
	/** @sa VST_EFFECT_OPCODE_12 */
	VST_EFFECT_OPCODE_EDITOR_KEYBOARD = 0x12,

	/** Window/Editor Idle/Keep-Alive Callback?
	 *
	 * Does not receive any parameters. Randomly called when nothing happens? Idle/Keep-Alive callback?
	 */
	VST_EFFECT_OPCODE_13 = 0x13,
	/** @sa VST_EFFECT_OPCODE_13 */
	VST_EFFECT_OPCODE_EDITOR_KEEP_ALIVE = 0x13,

	/** Window Focus Event?
	 *
	 * Sometimes called when the editor window goes back into focus.
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 * @note Appears to be Mac OS exclusive.
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_14 = 0x14,

	/** Window Unfocus Event?
	 *
	 * Sometimes called when the editor window goes out of focus.
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 * @note Appears to be Mac OS exclusive.
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_15 = 0x15,

	/**
	 *
	 * @note Present in some VST 2.1 or earlier plugins.
	 * @important Almost all plug-ins return the @ref VST_FOURCC 'NvEf' (0x4E764566) here.
	 * @deprecated (VST 2.4+) Likely deprecated in VST 2.4 and later.
	 */
	VST_EFFECT_OPCODE_16 = 0x16,
	/** @sa VST_EFFECT_OPCODE_16 */
	VST_EFFECT_OPCODE_FOURCC = 0x16,

	/** Get Chunk Data
	 *
	 * Save current program or bank state to a buffer.
	 * Behavior is different based on the @ref VST_EFFECT_FLAG_CHUNKS flag.
	 *
	 * @sa VST_EFFECT_FLAG_CHUNKS
	 * @param p_int1	0 means Bank, 1 means Program, nothing else used?
	 * @param p_ptr		`void**` Pointer to a potential pointer containing your own chunk data.
	 * @return 			Size of the Chunk Data in bytes.
	 */
	VST_EFFECT_OPCODE_17 = 0x17,
	/** @sa VST_EFFECT_OPCODE_17 */
	VST_EFFECT_OPCODE_GET_CHUNK_DATA = 0x17,

	/** Set Chunk Data
	 *
	 * Restore current program or bank state from a buffer.
	 * Behavior is different based on the @ref VST_EFFECT_FLAG_CHUNKS flag.
	 *
	 * @sa VST_EFFECT_FLAG_CHUNKS
	 * @param p_int1	0 means Bank, 1 means Program, nothing else used?
	 * @param p_int2	Size of the Chunk Data in bytes.
	 * @param p_ptr		`void*` Pointer to a buffer containing chunk data.
	 */
	VST_EFFECT_OPCODE_18 = 0x18,
	/** @sa VST_EFFECT_OPCODE_18 */
	VST_EFFECT_OPCODE_SET_CHUNK_DATA = 0x18,

	//--------------------------------------------------------------------------------
	// VST 2.x starts here.
	//--------------------------------------------------------------------------------

	/** Send events from host to plug-in.
	 * The plug-in must support receiving events (see @ref vst_effect_supports_t.receiveVstEvents) while the host may
	 * optionally signal to the plugin that it wants to send events to the host (see @ref
	 * vst_host_supports_t.sendVstEvents).
	 *
	 * @sa vst_event_t
	 * @sa vst_events_t
	 * @sa vst_host_supports_t.sendVstEvents
	 * @sa vst_effect_supports_t.receiveVstEvents
	 * @sa vst_host_supports_t.sendVstMidiEvents
	 * @sa vst_effect_supports_t.receiveVstMidiEvents
	 * @sa VST_HOST_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_ptr A valid pointer to a @ref vst_events_t structure.
	 */
	VST_EFFECT_OPCODE_19 = 0x19,
	/** @sa VST_EFFECT_OPCODE_19 */
	VST_EFFECT_OPCODE_EVENT = 0x19,

	/** Can the parameter be automated?
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int1 Index of the parameter.
	 * @return 1 if the parameter can be automated, otherwise 0.
	 */
	VST_EFFECT_OPCODE_1A                   = 0x1A,
	/** @sa VST_EFFECT_OPCODE_1A */
	VST_EFFECT_OPCODE_PARAM_ISAUTOMATABLE  = 0x1A,
	/** @sa VST_EFFECT_OPCODE_1A */
	VST_EFFECT_OPCODE_PARAM_IS_AUTOMATABLE = 0x1A,
	/** @sa VST_EFFECT_OPCODE_1A */
	VST_EFFECT_OPCODE_PARAM_AUTOMATABLE = 0x1A,

	/** Set Parameter value from string representation.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int1 Index of the parameter.
	 * @param p_ptr `const char*` Zero terminated string representation of the value to set.
	 * @return 1 if it worked, otherwise 0.
	 */
	VST_EFFECT_OPCODE_1B = 0x1B,
	/** @sa VST_EFFECT_OPCODE_1B */
	VST_EFFECT_OPCODE_PARAM_SET_VALUE = 0x1B,
	/** @sa VST_EFFECT_OPCODE_1B */
	VST_EFFECT_OPCODE_PARAM_VALUE_FROM_STRING = 0x1B,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_1C = 0x1C,

	/**
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @sa VST_EFFECT_OPCODE_05
	 * @todo
	 */
	VST_EFFECT_OPCODE_1D = 0x1D,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_1E = 0x1E,

	/** Input connected.
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_1F = 0x1F,

	/** Input disconnected.
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_20 = 0x20,

	/** Retrieve properties for the given input index.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int1 Index of the input to get the properties for.
	 * @param p_ptr Pointer to @ref vst_stream_properties_t for the selected input provided by the host.
	 * @return @ref VST_STATUS_TRUE if p_ptr is updated, @ref VST_STATUS_FALSE otherwise.
	 */
	VST_EFFECT_OPCODE_21                   = 0x21,
	/** @sa VST_EFFECT_OPCODE_21 */
	VST_EFFECT_OPCODE_INPUT_GET_PROPERTIES = 0x21,
	/** @sa VST_EFFECT_OPCODE_21 */
	VST_EFFECT_OPCODE_INPUT_STREAM_GET_PROPERTIES = 0x21,

	/** Retrieve properties for the given output index.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int1 Index of the output to get the properties for.
	 * @param p_ptr Pointer to @ref vst_stream_properties_t for the selected output provided by the host.
	 * @return @ref VST_STATUS_TRUE if p_ptr is updated, @ref VST_STATUS_FALSE otherwise.
	 */
	VST_EFFECT_OPCODE_22                    = 0x22,
	/** @sa VST_EFFECT_OPCODE_22 */
	VST_EFFECT_OPCODE_OUTPUT_GET_PROPERTIES = 0x22,
	/** @sa VST_EFFECT_OPCODE_22 */
	VST_EFFECT_OPCODE_OUTPUT_STREAM_GET_PROPERTIES = 0x22,

	/** Retrieve category of this effect.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return The category that this effect is in, see @ref VST_EFFECT_CATEGORY.
	 */
	VST_EFFECT_OPCODE_23              = 0x23,
	/** @sa VST_EFFECT_OPCODE_23 */
	VST_EFFECT_OPCODE_EFFECT_CATEGORY = 0x23,
	/** @sa VST_EFFECT_OPCODE_23 */
	VST_EFFECT_OPCODE_CATEGORY = 0x23,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_24 = 0x24,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_25 = 0x25,

	/**
	 *
	 * Seen in plug-ins with @ref VST_EFFECT_CATEGORY_OFFLINE.
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_26 = 0x26,

	/**
	 *
	 * Seen in plug-ins with @ref VST_EFFECT_CATEGORY_OFFLINE.
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_27 = 0x27,

	/**
	 *
	 * Seen in plug-ins with @ref VST_EFFECT_CATEGORY_OFFLINE.
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_28 = 0x28,

	/**
	 *
	 * Seen in plug-ins with @ref VST_EFFECT_CATEGORY_OFFLINE.
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_29 = 0x29,

	/** Host wants to change the speaker arrangement.
	 *
	 * @sa vst_effect_t.num_inputs
	 * @sa vst_effect_t.num_outputs
	 * @sa VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int2 Pointer to a @ref vst_speaker_arrangement_t structure.
	 * @param p_ptr Pointer to a @ref vst_speaker_arrangement_t structure.
	 * @return @ref VST_STATUS_TRUE if we accept the new arrangement, @ref VST_STATUS_FALSE if we don't in which case
	 *         the host is required to ask for the speaker arrangement via @ref VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 *         and may retry this op-code with different values.
	 */
	VST_EFFECT_OPCODE_2A                      = 0x2A,
	/** @sa VST_EFFECT_OPCODE_2A */
	VST_EFFECT_OPCODE_SET_SPEAKER_ARRANGEMENT = 0x2A,

	/**
	 * @todo
	 */
	VST_EFFECT_OPCODE_2B = 0x2B,

	/** Enable/Disable bypassing the effect.
	 *
	 * See @ref VST_EFFECT_OPCODE_SUPPORTS with @ref vst_effect_supports_t.bypass for more information.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int2 Zero if bypassing the effect is disabled, otherwise 1.
	 */
	VST_EFFECT_OPCODE_2C     = 0x2C,
	/** @sa VST_EFFECT_OPCODE_2C */
	VST_EFFECT_OPCODE_BYPASS = 0x2C,

	/** Retrieve the effect name into the ptr buffer.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @bug Various hosts only provide a buffer that is 32 bytes long.
	 * @param p_ptr A zero terminated char buffer of size @ref VST_BUFFER_SIZE_EFFECT_NAME.
	 * @return Always 0, even on failure.
	 */
	VST_EFFECT_OPCODE_2D          = 0x2D,
	/** @sa VST_EFFECT_OPCODE_2D */
	VST_EFFECT_OPCODE_GETNAME     = 0x2D,
	/** @sa VST_EFFECT_OPCODE_2D */
	VST_EFFECT_OPCODE_EFFECT_NAME = 0x2D,
	/** @sa VST_EFFECT_OPCODE_2D */
	VST_EFFECT_OPCODE_NAME = 0x2D,

	/** Translate an error code to a string.
	 *
	 * @bug Some hosts provide unexpected data in p_ptr.
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4+) Fairly sure this is deprecated in VST 2.4 and later.
	 * @param p_ptr A zero terminated char buffer with undefined size.
	 * @return @ref VST_STATUS_TRUE if we could translate the error, @ref VST_STATUS_FALSE if not.
	 */
	VST_EFFECT_OPCODE_2E              = 0x2E,
	/** @sa VST_EFFECT_OPCODE_2E */
	VST_EFFECT_OPCODE_TRANSLATE_ERROR = 0x2E,

	/** Retrieve the vendor name into the ptr buffer.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_ptr A zero terminated char buffer of size @ref VST_BUFFER_SIZE_VENDOR_NAME.
	 */
	VST_EFFECT_OPCODE_2F          = 0x2F,
	/** @sa VST_EFFECT_OPCODE_2F */
	VST_EFFECT_OPCODE_GETVENDOR   = 0x2F,
	/** @sa VST_EFFECT_OPCODE_2F */
	VST_EFFECT_OPCODE_VENDOR_NAME = 0x2F,

	/** Retrieve the product name into the ptr buffer.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_ptr A zero terminated char buffer of size @ref VST_BUFFER_SIZE_PRODUCT_NAME.
	 */
	VST_EFFECT_OPCODE_30           = 0x30,
	/** @sa VST_EFFECT_OPCODE_30 */
	VST_EFFECT_OPCODE_GETNAME2     = 0x30,
	/** @sa VST_EFFECT_OPCODE_30 */
	VST_EFFECT_OPCODE_PRODUCT_NAME = 0x30,

	/** Retrieve the vendor version in return value.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return Version.
	 */
	VST_EFFECT_OPCODE_31               = 0x31,
	/** @sa VST_EFFECT_OPCODE_31 */
	VST_EFFECT_OPCODE_GETVENDORVERSION = 0x31,
	/** @sa VST_EFFECT_OPCODE_31 */
	VST_EFFECT_OPCODE_VENDOR_VERSION   = 0x31,

	/** User-defined Op-Code for VST extensions.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * All parameters are undefined by the standard and left up to the host/plug-in. Use @ref VST_EFFECT_OPCODE_SUPPORTS
	 * and @ref VST_EFFECT_OPCODE_VENDOR_NAME + @ref VST_EFFECT_OPCODE_VENDOR_VERSION to check if the plug-in is
	 * compatible with your expected format.
	 */
	VST_EFFECT_OPCODE_32     = 0x32,
	/** @sa VST_EFFECT_OPCODE_32 */
	VST_EFFECT_OPCODE_CUSTOM = 0x32,

	/** Test for support of a specific named feature.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_ptr A zero terminated char buffer of undefined size containing the feature name.
	 * @return @ref VST_STATUS_YES if the feature is supported, @ref VST_STATUS_NO if the feature is not supported,
	 *         @ref VST_STATUS_UNKNOWN in all other cases.
	 */
	VST_EFFECT_OPCODE_33       = 0x33,
	/** @sa VST_EFFECT_OPCODE_33 */
	VST_EFFECT_OPCODE_SUPPORTS = 0x33,

	/** Number of samples that are at the tail at the end of playback.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @return @ref VST_STATUS_UNKNOWN for automatic tail size, @ref VST_STATUS_TRUE for no tail, any other number above
	 *         1 for the number of samples the tail has.
	 */
	VST_EFFECT_OPCODE_34             = 0x34,
	/** @sa VST_EFFECT_OPCODE_34 */
	VST_EFFECT_OPCODE_GETTAILSAMPLES = 0x34,
	/** @sa VST_EFFECT_OPCODE_34 */
	VST_EFFECT_OPCODE_TAIL_SAMPLES   = 0x34,

	/** Notify effect that it is idle?
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4+) As of VST 2.4 the default behavior is @ref VST_EFFECT_OPCODE_PROCESS_BEGIN and
	 *             @ref VST_EFFECT_OPCODE_PROCESS_END which allows cleaner control flows.
	 * @sa vst_host_supports.startStopProcess
	 */
	VST_EFFECT_OPCODE_35 = 0x35,
	/** @sa VST_EFFECT_OPCODE_35 */
	VST_EFFECT_OPCODE_IDLE = 0x35,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4) Invalid in all VST 2.4 and later hosts.
	 * @todo
	 */
	VST_EFFECT_OPCODE_36 = 0x36,

	/**
	 *
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4) Invalid in all VST 2.4 and later hosts.
	 * @todo
	 */
	VST_EFFECT_OPCODE_37 = 0x37,

	/** Parameter Properties
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @param p_int1 Parameter index to get properties for.
	 * @param p_ptr Pointer to @ref vst_parameter_properties_t for the given parameter.
	 * @return @ref VST_STATUS_YES if supported, otherwise @ref VST_STATUS_NO.
	 */
	VST_EFFECT_OPCODE_38 = 0x38,
	/** @sa VST_EFFECT_OPCODE_38 */
	VST_EFFECT_OPCODE_GET_PARAMETER_PROPERTIES = 0x38,
	/** @sa VST_EFFECT_OPCODE_38 */
	VST_EFFECT_OPCODE_PARAM_PROPERTIES = 0x38,

	/**
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @deprecated (VST 2.4) Invalid in all VST 2.4 and later hosts.
	 * @todo
	 */
	VST_EFFECT_OPCODE_39                       = 0x39,

	/** Retrieve the VST Version supported.
	 *
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 * @sa VST_VERSION
	 * @return One of the valid enums in @ref VST_VERSION
	 */
	VST_EFFECT_OPCODE_3A          = 0x3A,
	/** @sa VST_EFFECT_OPCODE_3A */
	VST_EFFECT_OPCODE_VST_VERSION = 0x3A,

	//--------------------------------------------------------------------------------
	// VST 2.1
	//--------------------------------------------------------------------------------

	/** Editor Virtual Key Down Input
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @param p_int1 ASCII character that represents the virtual key code.
	 * @param p_int2 See @ref VST_VKEY for the full list.
	 * @param p_float A bitfield with any of @ref VST_VKEY_MODIFIER.
	 * @return @ref VST_STATUS_TRUE if we used the input, otherwise @ref VST_STATUS_FALSE
	 */
	VST_EFFECT_OPCODE_3B = 0x3B,
	/** @sa VST_EFFECT_OPCODE_3B */
	VST_EFFECT_OPCODE_EDITOR_VKEY_DOWN = 0x3B,

	/** Editor Virtual Key Up Event
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @param p_int1 ASCII character that represents the virtual key code.
	 * @param p_int2 See @ref VST_VKEY for the full list.
	 * @param p_float A bitfield with any of @ref VST_VKEY_MODIFIER.
	 * @return @ref VST_STATUS_TRUE if we used the input, otherwise @ref VST_STATUS_FALSE
	 */
	VST_EFFECT_OPCODE_3C = 0x3C,
	/** @sa VST_EFFECT_OPCODE_3C */
	VST_EFFECT_OPCODE_EDITOR_VKEY_UP = 0x3C,

	/**
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @param p_int2 A value between 0 and 2.
	 * @todo
	 */
	VST_EFFECT_OPCODE_3D = 0x3D,

	/**
	 *
	 * Midi related
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_3E = 0x3E,

	/**
	 *
	 * Midi related
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_3F = 0x3F,

	/**
	 *
	 * Midi related
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_40 = 0x40,

	/**
	 *
	 * Midi related
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_41 = 0x41,

	/**
	 *
	 * Midi related
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_42 = 0x42,

	/** Host is starting to set up a program.
	 * Emitted prior to the host loading a program.
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @return @ref VST_STATUS_TRUE if we understood the notification, or @ref VST_STATUS_FALSE if not.
	 */
	VST_EFFECT_OPCODE_43 = 0x43,
	/** @sa VST_EFFECT_OPCODE_43 */
	VST_EFFECT_OPCODE_PROGRAM_SET_BEGIN = 0x43,

	/** Host is done setting up a program.
	 * Emitted after the host finished loading a program.
	 *
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @return @ref VST_STATUS_TRUE if we understood the notification, or @ref VST_STATUS_FALSE if not.
	 */
	VST_EFFECT_OPCODE_44 = 0x44,
	/** @sa VST_EFFECT_OPCODE_44 */
	VST_EFFECT_OPCODE_PROGRAM_SET_END = 0x44,

	//--------------------------------------------------------------------------------
	// VST 2.3
	//--------------------------------------------------------------------------------

	/** Host wants to know the current speaker arrangement.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @param p_int2 Pointer to a @ref vst_speaker_arrangement_t pointer.
	 * @param p_ptr Pointer to a @ref vst_speaker_arrangement_t pointer.
	 * @return @ref VST_STATUS_TRUE if we were successful, otherwise @ref VST_STATUS_FALSE.
	 */
	VST_EFFECT_OPCODE_45                      = 0x45,
	/** @sa VST_EFFECT_OPCODE_45 */
	VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT = 0x45,

	/** Get the next effect contained in this effect.
	 * This returns the next effect based on an effect internal counter, the host does not provide any index.
	 *
	 * Used in combination with @ref VST_EFFECT_CATEGORY_CONTAINER.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @param p_ptr Pointer to a char buffer of size @ref VST_BUFFER_SIZE_EFFECT_NAME to store the name of the next effect.
	 * @return Next effects unique_id
	 */
	VST_EFFECT_OPCODE_46 = 0x46,
	/** @sa VST_EFFECT_OPCODE_46 */
	VST_EFFECT_OPCODE_CONTAINER_NEXT_EFFECT_ID = 0x46,

	/** Begin processing of audio.
	 *
	 * Host is requesting that we prepare for a new section of audio separate from the previous section.
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 */
	VST_EFFECT_OPCODE_47 = 0x47,
	/** @sa VST_EFFECT_OPCODE_47 */
	VST_EFFECT_OPCODE_PROCESS_BEGIN = 0x47,

	/** End processing of audio.
	 *
	 * Host is requesting that we stop processing audio and go into idle instead.
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 */
	VST_EFFECT_OPCODE_48 = 0x48,
	/** @sa VST_EFFECT_OPCODE_48 */
	VST_EFFECT_OPCODE_PROCESS_END = 0x48,

	/**
	 *
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_49 = 0x49,

	/**
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @sa VST_EFFECT_CATEGORY_SPATIAL
	 * @param p_int2 Unknown meaning.
	 * @param p_float Unknown meaning, usually 1.0
	 * @todo
	 */
	VST_EFFECT_OPCODE_4A = 0x4A,

	/** Host wants to know if we can load the provided bank data.
	 * Should be emitted prior to @ref VST_EFFECT_OPCODE_SET_CHUNK_DATA by the host.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @param p_ptr Unknown structured data.
	 * @return @ref VST_STATUS_NO if we can't load the data, @ref VST_STATUS_YES if we can load the data,
	 *         @ref VST_STATUS_UNKNOWN if this isn't supported.
	 * @todo
	 */
	VST_EFFECT_OPCODE_4B = 0x4B,
	/** @sa VST_EFFECT_OPCODE_4B */
	VST_EFFECT_OPCODE_BANK_LOAD = 0x4B,

	/** Host wants to know if we can load the provided program data.
	 * Should be emitted prior to @ref VST_EFFECT_OPCODE_PROGRAM_SET_BEGIN by the host.
	 *
	 * @note (VST 2.3+) Available from VST 2.3 onwards.
	 * @param p_ptr Unknown structured data.
	 * @return @ref VST_STATUS_NO if we can't load the data, @ref VST_STATUS_YES if we can load the data,
	 *         @ref VST_STATUS_UNKNOWN if this isn't supported.
	 * @todo
	 */
	VST_EFFECT_OPCODE_4C = 0x4C,
	/** @sa VST_EFFECT_OPCODE_4C */
	VST_EFFECT_OPCODE_PROGRAM_LOAD = 0x4C,

	//--------------------------------------------------------------------------------
	// VST 2.4
	//--------------------------------------------------------------------------------

	/**
	 *
	 *
	 * @note (VST 2.4+) Available from VST 2.4 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_4D = 0x4D,

	/**
	 *
	 *
	 * @note (VST 2.4+) Available from VST 2.4 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_4E = 0x4E,

	/**
	 *
	 *
	 * @note (VST 2.4+) Available from VST 2.4 onwards.
	 * @todo
	 */
	VST_EFFECT_OPCODE_4F = 0x4F,

	/** @private Highest known OPCODE. */
	VST_EFFECT_OPCODE_MAX,

	/** @private Force as 32-bit unsigned integer in compatible compilers. */
	_VST_EFFECT_OPCODE_PAD = (-1l)
};

#if (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 202002L)
/** Host to Plug-in support checks
 *
 * Provided as `char* p_ptr` in the VST_EFFECT_OPCODE_SUPPORTS op code.
 *
 * Harvested via strings command and just checking what plug-ins actually responded to.
 *
 * @important These are only available with a C99 or a C++20 or newer compiler.
 */
struct vst_effect_supports_t {
	/** Effect supports alternative bypass.
	 * The alternative bypass still has the host call process/process_float/process_double and expects us to compensate
	 * for our delay/latency, copy inputs to outputs, and do minimal work. If we don't support it the host will not call
	 * process/process_float/process_double at all while bypass is enabled.
	 *
	 * @note VST 2.3 or later only.
	 * @return VST_STATUS_TRUE if we support this, otherwise VST_STATUS_FALSE.
	 */
	const char* bypass;

	/** Plug-in can send events to host.
	 *
	 * @sa vst_host_supports_t.receiveVstEvents
	 * @sa VST_HOST_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* sendVstEvents;

	/** Plug-in can receive events from host.
	 *
	 * @sa vst_host_supports_t.sendVstEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* receiveVstEvents;

	/** Host can send MIDI events to plug-in.
	 *
	 * @sa vst_effect_supports_t.receiveVstMidiEvents
	 * @sa VST_EFFECT_OPCODE_EVENT
	 * @sa vst_effect_midi_t
	 * @sa vst_effect_midi_sysex_t
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* sendVstMidiEvent;

	/** Plug-in can receive MIDI events from host.
	 *
	 * @sa vst_host_supports_t.sendVstMidiEvents
	 * @sa VST_HOST_OPCODE_EVENT
	 * @sa vst_effect_midi_t
	 * @sa vst_effect_midi_sysex_t
	 * @note (VST 2.0+) Available from VST 2.0 onwards.
	 */
	const char* receiveVstMidiEvent;

	/** Plug-in wants to use @ref VST_HOST_OPCODE_EDITOR_RESIZE.
	 * Only necessary for legacy host compatibility.
	 *
	 * @sa vst_host_supports_t.sizeWindow
	 * @note (VST 2.1+) Available from VST 2.1 onwards.
	 * @deprecated (VST 2.4+) Deprecated from VST 2.4 onwards as the same check already exists on the host side.
	 * @return @ref VST_STATUS_TRUE if you want to use @ref VST_HOST_OPCODE_EDITOR_RESIZE, otherwise @ref VST_STATUS_FALSE.
	 */
	const char* conformsToWindowRules;

	const char* midiProgramNames; // VST 2.1 or later.
	const char* receiveVstTimeInfo;
	const char* offline;
	// The following were only found in VST 2.3 plug-ins
	const char* plugAsChannelInsert;
	const char* plugAsSend;
	const char* mixDryWet;
	const char* noRealTime;
	const char* multipass;
	const char* metapass;
	const char* _1in1out;
	const char* _1in2out;
	const char* _2in1out;
	const char* _2in2out;
	const char* _2in4out;
	const char* _4in2out;
	const char* _4in4out;
	const char* _4in8out;
	const char* _8in4out;
	const char* _8in8out;
} /** @private */ vst_effect_supports = {
	.bypass = "bypass",
	.sendVstEvents = "sendVstEvents",
	.receiveVstEvents = "receiveVstEvents",
	.sendVstMidiEvent = "sendVstMidiEvent",
	.receiveVstMidiEvent = "receiveVstMidiEvent",
	.conformsToWindowRules = "conformsToWindowRules",
	.midiProgramNames = "midiProgramNames",
	.receiveVstTimeInfo = "receiveVstTimeInfo",
	.offline = "offline",
	.plugAsChannelInsert = "plugAsChannelInsert",
	.plugAsSend = "plugAsSend",
	.mixDryWet = "mixDryWet",
	.noRealTime = "noRealTime",
	.multipass = "multipass",
	.metapass = "metapass",
	._1in1out = "1in1out",
	._1in2out = "1in2out",
	._2in1out = "2in1out",
	._2in2out = "2in2out",
	._2in4out = "2in4out",
	._4in2out = "4in2out",
	._4in4out = "4in4out",
	._4in8out = "4in8out",
	._8in4out = "8in4out",
	._8in8out = "8in8out",
};
#endif

/** Control the VST through an opcode and up to four parameters.
 *
 * @sa VST_EFFECT_OPCODE
 *
 * @param self Pointer to the effect itself.
 * @param opcode The opcode to run, see @ref VST_EFFECT_OPCODE.
 * @param p_int1 Parameter, see @ref VST_EFFECT_OPCODE.
 * @param p_int2 Parameter, see @ref VST_EFFECT_OPCODE.
 * @param p_ptr Parameter, see @ref VST_EFFECT_OPCODE.
 * @param p_float Parameter, see @ref VST_EFFECT_OPCODE.
 */
typedef intptr_t (VST_FUNCTION_INTERFACE* vst_effect_control_t)(struct vst_effect_t* self, int32_t opcode, int32_t p_int1, intptr_t p_int2, void* p_ptr, float p_float);

/** Process the given number of samples in inputs and outputs.
 *
 * Used to handle input data and provides output data. We seem to be the ones that provide the output buffer?
 *
 * @param self Pointer to the effect itself.
 * @param inputs Pointer to an array of 'const float[samples]' with size @ref vst_effect_t.num_inputs.
 * @param outputs Pointer to an array of 'float[samples]' with size @ref vst_effect_t.num_outputs.
 * @param samples Number of samples per channel in inputs and outputs.
 */
typedef void (VST_FUNCTION_INTERFACE* vst_effect_process_t) (struct vst_effect_t* self, const float* const* inputs, float** outputs, int32_t samples);

/** Updates the value for the parameter at the given index, or does nothing if out of bounds.
 *
 * @param self Pointer to the effect itself.
 * @param index Parameter index.
 * @param value New value for the parameter.
 */
typedef void(VST_FUNCTION_INTERFACE* vst_effect_set_parameter_t)(struct vst_effect_t* self, uint32_t index, float value);

/** Retrieve the current value of the parameter at the given index, or do nothing if out of bounds.
 *
 * @param self Pointer to the effect itself.
 * @param index Parameter index.
 * @return Current value of the parameter.
 */
typedef float(VST_FUNCTION_INTERFACE* vst_effect_get_parameter_t)(struct vst_effect_t* self, uint32_t index);

/** Process the given number of single samples in inputs and outputs.
 *
 * Process input and overwrite the output in place. Host provides output buffers.
 *
 * @important Not thread-safe on MacOS for some reason or another.
 *
 * @param self Pointer to the effect itself.
 * @param inputs Pointer to an array of 'const float[samples]' with size numInputs.
 * @param outputs Pointer to an array of 'float[samples]' with size numOutputs.
 * @param samples Number of samples per channel in inputs.
 */
typedef void(VST_FUNCTION_INTERFACE* vst_effect_process_float_t)(struct vst_effect_t* self, const float* const* inputs, float** outputs, int32_t samples);

/** Process the given number of double samples in inputs and outputs.
 *
 * Process input and overwrite the output in place. Host provides output buffers.
 *
 * @note (VST 2.4+) Available from VST 2.4 and later.
 *
 * @param self Pointer to the effect itself.
 * @param inputs Pointer to an array of 'const double[samples]' with size numInputs.
 * @param outputs Pointer to an array of 'double[samples]' with size numOutputs.
 * @param samples Number of samples per channel in inputs.
 */
typedef void (VST_FUNCTION_INTERFACE* vst_effect_process_double_t)(struct vst_effect_t* self, const double* const* inputs, double** outputs, int32_t samples);


class vst_time_info
{
public:
   // 00
   double samplePos;
   // 08
   double sampleRate;
   // 10
   double nanoSeconds;
   // 18
   double ppqPos;
   // 20?
   double tempo;
   // 28
   double barStartPos;
   // 30?
   double cycleStartPos;
   // 38?
   double cycleEndPos;
   // 40?
   int timeSigNumerator;
   // 44?
   int timeSigDenominator;
   // unconfirmed 48 4c 50
   char empty3[4 + 4 + 4];
   // 54
   int flags;

} ;
/** Plug-in Effect definition
 */
struct vst_effect_t {
	/** VST Magic Number
	 *
	 * Should always be VST_FOURCC('VstP')
	 *
	 * @sa VST_MAGICNUMBER
	 */
	int32_t magic_number;

	/** Control Function
	 * @sa vst_effect_control_t
	 * @sa VST_EFFECT_OPCODE
	 */
	vst_effect_control_t control;

	/** Process Function
	 * @sa vst_effect_process_t
	 * @deprecated (VST 2.4+) Deprecated and practically unsupported in all VST 2.4 compatible hosts and may treat it
	 *             as just another @ref vst_effect_t.process_float.
	 */
	vst_effect_process_t process;

	/** Set Parameter Function
	 * @sa vst_effect_set_parameter_t
	 */
	vst_effect_set_parameter_t set_parameter;

	/** Get Parameter Function
	 * @sa vst_effect_get_parameter_t
	 */
	vst_effect_get_parameter_t get_parameter;

	/** Number of available pre-defined programs.
	 *
	 * @sa VST_EFFECT_OPCODE_PROGRAM_LOAD
	 * @sa VST_EFFECT_OPCODE_PROGRAM_SET_BEGIN
	 * @sa VST_EFFECT_OPCODE_PROGRAM_SET
	 * @sa VST_EFFECT_OPCODE_PROGRAM_SET_NAME
	 * @sa VST_EFFECT_OPCODE_PROGRAM_SET_END
	 * @sa VST_EFFECT_OPCODE_PROGRAM_GET
	 * @sa VST_EFFECT_OPCODE_PROGRAM_GET_NAME
	 * @sa VST_EFFECT_FLAG_CHUNKS
	 * @sa VST_EFFECT_OPCODE_SET_CHUNK_DATA
	 * @sa VST_EFFECT_OPCODE_GET_CHUNK_DATA
	 */
	int32_t num_programs;

	/** Number of available parameters.
	 * All programs must have at least this many parameters.
	 *
	 * @sa VST_HOST_OPCODE_IO_MODIFIED
	 */
	int32_t num_params;

	/** Number of available input streams.
	 *
	 *
	 * @sa VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 * @sa VST_EFFECT_OPCODE_INPUT_GET_PROPERTIES
	 * @sa VST_HOST_OPCODE_IO_MODIFIED
	 */
	int32_t num_inputs;

	/** Number of available output streams.
	 *
	 * @sa VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT
	 * @sa VST_EFFECT_OPCODE_OUTPUT_GET_PROPERTIES
	 * @sa VST_HOST_OPCODE_IO_MODIFIED
	 */
	int32_t num_outputs;

	/** Effect Flags
	 *
	 * @sa VST_EFFECT_FLAGS
	 */
	int32_t flags;

	/** @todo */
	void* _unknown_00; // Must be zero when created. Reserved for host?

	/** @todo */
	void* _unknown_01; // Must be zero when created. Reserved for host?

	/** Initial delay before processing of samples can actually begin in Samples.
	 *
	 * @note The host can modify this at runtime so it is not safe.
	 * @note Should be reinitialized when the effect is resumed.
	 *
	 * @sa VST_HOST_OPCODE_IO_MODIFIED
	 */
	int32_t delay;

	/** @todo */
	int32_t _unknown_02; // Unknown int32_t values.

	/** @todo */
	int32_t _unknown_03;

	/** Ratio of Input to Output production
	 * Defines how much output data is produced relative to input data when using 'process' instead of 'processFloat'.
	 * Example: A ratio of 2.0 means we produce twice as much output as we receive input.
	 *
	 * Range: >0.0 to Infinity
	 * Default: 1.0
	 * @note Ignored in VST 2.4 or with VST_EFFECT_FLAG_SUPPORTS_FLOAT.
	 */
	float input_output_ratio;

	/** Effect Internal Pointer
	 *
	 * You can freely set this to point at some sort of class or similar for use in your own effect. The host must
	 * never modify this or the data available through this.
	 */
	void* effect_internal;

	/** Host Internal Pointer
	 *
	 * The host may set this to point at data related to your effect instance that the host needs. The effect must
	 * never modify this or the data available through this.
	 */
	void* host_internal; // Pointer to Host internal data.

	/** Id of the plugin.
	 *
	 * Due to this not being enough for uniqueness, it should not be used alone for indexing.
	 * Ideally you want to index like this:
	 *     [unique_id][module_name][version][flags]
	 * If any of the checks after unique_id fail, you default to the first possible choice.
	 *
	 * Used in combination with @ref VST_EFFECT_CATEGORY_CONTAINER.
	 *
	 * BUG: Some broken hosts rely on this alone to save information about VST plug-ins.
	 */
	int32_t unique_id;

	/** Plugin version
	 *
	 * Unrelated to the minimum VST Version, but often the same.
	 */
	int32_t version;

	//--------------------------------------------------------------------------------
	// VST 2.x starts here.
	//--------------------------------------------------------------------------------

	/** Process function for in-place single (32-bit float) processiong.
	 * @sa vst_effect_process_single_t
	 * @note (VST 2.0+) Available from VST 2.0 and later.
	 */
	vst_effect_process_float_t process_float;

	//--------------------------------------------------------------------------------
	// VST 2.4 starts here.
	//--------------------------------------------------------------------------------

	/** Process function for in-place double (64-bit float) processiong.
	 * @sa vst_effect_process_double_t
	 * @note (VST 2.4+) Available from VST 2.4 and later.
	 */
	vst_effect_process_double_t process_double;

	// Everything after this is unknown and was present in reacomp-standalone.dll.
	uint8_t _unknown[56]; // 56-bytes of something. Could also just be 52-bytes.
};

/** VST 2.x Entry Point for all platforms
 *
 * Must be present in VST 2.x plug-ins but must not be present in VST 1.x plug-ins.
 *
 * @return A new instance of the VST 2.x effect.
 */
#define VST_ENTRYPOINT \
	vst_effect_t* VSTPluginMain(vst_host_callback_t callback)

/** [DEPRECATED] VST 1.x Entry Point for Windows
 *
 * Do not implement in VST 2.1 or later plug-ins!
 *
 * @return A new instance of the VST 1.x effect.
 */
#define VST_ENTRYPOINT_WINDOWS \
	vst_effect_t* MAIN(vst_host_callback_t callback) { return VSTPluginMain(callback); }

/** [DEPRECATED] VST 1.x Entry Point for MacOS
 *
 * Do not implement in VST 2.1 or later plug-ins!
 *
 * @return A new instance of the VST 1.x effect.
 */
#define VST_ENTRYPOINT_MACOS \
	vst_effect_t* main_macho(vst_host_callback_t callback) { return VSTPluginMain(callback); }

/** [DEPRECATED] VST 2.3 Entry Point for PowerPC
 *
 * Present in some VST 2.3 and earlier compatible plug-ins that support MacOS.
 *
 * @return A new instance of the VST 2.x effect.
 */
#define VST_ENTRYPOINT_MACOS_POWERPC \
	vst_effect_t* main(vst_host_callback_t callback) { return VSTPluginMain(callback); }

#ifdef __cplusplus
}
#endif
#pragma pack(pop)
#endif