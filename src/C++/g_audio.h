/*
G-Audio - An audio library for LabVIEW.

v0.2.0

This code implements a wrapper library around the following libraries:
dr_flac and dr_wav - https://github.com/mackron/dr_libs
miniaudio - https://github.com/mackron/miniaudio
stb_vorbis - https://github.com/nothings/stb
minimp3 - https://github.com/lieff/minimp3

Also includes the following libraries:
thread.h - https://github.com/mattiasgustavsson/libs

Author - Dataflow_G
GitHub - https://github.com/dataflowg/g-audio
Twitter - https://twitter.com/Dataflow_G

/////////////
// HISTORY //
/////////////
v0.2.0
- Audio device playback and capture via miniaudio
- Update dr_flac, dr_wav, minimp3, stb_vorbis libraries

v0.1.0
- Initial release

/////////////
// LICENSE //
/////////////
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#ifndef _G_AUDIO_H_
#define _G_AUDIO_H_

#include <stdint.h>
#if defined(_WIN32)
#include <stringapiset.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include <vector>

// Single header decoder definitions. These must appear before including decoders.
#define DR_FLAC_IMPLEMENTATION
#define MINIMP3_IMPLEMENTATION
//#define MINIMP3_FLOAT_OUTPUT
#define DR_WAV_IMPLEMENTATION

#include "thread_safety.h"
#include "dr_flac.h"
#include "minimp3_ex.h"
#include "stb_vorbis.h"
#include "dr_wav.h"

// Don't define miniaudio's encoders and decoders, as we're using those libraries separately.
#define MA_NO_DECODING
#define MA_NO_ENCODING
// Not using miniaudio to generate audio signals
#define MA_NO_GENERATION
// No null backend
#define MA_NO_NULL
// #define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#if defined(_WIN32)
#define LV_DLL_IMPORT  __declspec(dllimport)
#define LV_DLL_EXPORT  __declspec(dllexport)
#define LV_DLL_PRIVATE static
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#define LV_DLL_IMPORT  __attribute__((visibility("default")))
#define LV_DLL_EXPORT  __attribute__((visibility("default")))
#define LV_DLL_PRIVATE __attribute__((visibility("hidden")))
#else
#define LV_DLL_IMPORT
#define LV_DLL_EXPORT
#define LV_DLL_PRIVATE static
#endif
#endif

typedef int32_t GA_RESULT;
#define GA_SUCCESS				0
// ERRORS
#define GA_E_GENERIC			-1		// Generic error
#define GA_E_MEMORY				-2		// Memory allocation / deallocation error
#define GA_E_UNSUPPORTED		-3		// Unsupported codec
#define GA_E_FILE				-4		// Generic file IO error
#define GA_E_DECODER			-5		// Internal decoder error
#define GA_E_REFNUM				-6		// Invalid refnum
#define GA_E_READ_MODE			-7		// File was opened in read mode, but tried to write to it
#define GA_E_WRITE_MODE			-8		// File was opened in write mode, but tried to read from it
#define GA_E_INVALID_TYPE		-9		// Specified audio data type is invalid or unsupported by the codec
#define GA_E_REFNUM_LIMIT		-10		// Refnum allocation exhausted
#define GA_E_CONTEXT_BACKEND	-11		// The requested backend for a context differs from the active context
#define GA_E_DEVICE_STOPPED		-12		// The audio device is stopped when it should be running
#define GA_E_BUFFER_SIZE		-13		// Attempt to playback / capture more data than buffer can hold
#define GA_E_PLAYBACK_MODE		-14		// Device type is playback, but tried capture operation
#define GA_E_CAPTURE_MODE		-15		// Device type is capture, but tried playback operation
#define MA_ERROR_OFFSET			-1000	// Add this to miniaudio error codes for return to LabVIEW.
// WARNINGS
#define GA_W_BUFFER_SIZE		1		// The specified buffer size is smaller than the period, may cause glitches

#define CODEC_NAME_LEN 8
#define MAX_DEVICE_COUNT 32

typedef enum
{
	ga_codec_flac = 0,
	ga_codec_mp3,
	ga_codec_vorbis,
	ga_codec_wav,
	ga_codec_unsupported
} ga_codec;

typedef enum
{
	ga_file_mode_closed = 0,
	ga_file_mode_read,
	ga_file_mode_write
} ga_file_mode;

// Data types passed to and from LabVIEW.
// Note: Most audio codecs are I16 only, and none are truly 64-bit float. Only include DBL as pretty much all LabVIEW waveform processing is DBL.
// Similarly audio devices aren't generally 64-bit.
typedef enum
{
	ga_data_type_u8 = 0,
	ga_data_type_i16,
	ga_data_type_i32,
	ga_data_type_float,
	ga_data_type_double
} ga_data_type;

// Structure to hold infomration about the current file
typedef struct
{
	int32_t refnum;
	thread_mutex_t mutex;
	ga_codec codec;
	ga_file_mode file_mode;
	void* decoder;
	void* encoder;
	uint64_t read_offset;
	GA_RESULT (*open)(const char* file_name, void** decoder);
	GA_RESULT (*open_write)(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, void* codec_specific, void** encoder);
	GA_RESULT (*get_basic_info)(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
	GA_RESULT (*seek)(void* decoder, uint64_t offset, uint64_t* new_offset);
	GA_RESULT (*read)(void* decoder, uint64_t frames_to_read, ga_data_type data_type, uint64_t* frames_read, void* output_buffer);
	GA_RESULT (*write)(void* encoder, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written);
	GA_RESULT (*close)(void* decoder);
} audio_file_codec;

// Structure to hold information about the audio device
typedef struct
{
	ma_device device;
	ma_pcm_rb buffer;
	ma_int32 buffer_size;
} audio_device;


////////////////////////////
// LabVIEW Audio File API //
////////////////////////////

// Get detailed audio file information. Requires decoding some audio files (mp3).
extern "C" LV_DLL_EXPORT GA_RESULT get_audio_file_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample, ga_codec* codec);
// Load an entire audio file and return data in interleaved 16-bit integer format. Data returned by this function must be freed with free_sample_data().
extern "C" LV_DLL_EXPORT int16_t* load_audio_file_s16(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_codec* codec, GA_RESULT* result);
// Frees the memory allocated during a file load operation.
extern "C" LV_DLL_EXPORT void free_sample_data(int16_t* buffer);
// Opens an audio file in read mode. Call close_audio_file() to free memory related to the refnum.
extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file(const char* file_name, int32_t* refnum);
// Opens an audio file in write mode. Call close_audio_file() to free memory related to the refnum.
extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, ga_codec codec, int32_t has_specific_info, void* codec_specific, int32_t* refnum);
// Get basic audio file information. More detailed info can be accessed using get_audio_file_info().
extern "C" LV_DLL_EXPORT GA_RESULT get_basic_audio_file_info(int32_t refnum, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
// Updates the file offset to the specified offset. Subsequent reads will be at the new offset.
extern "C" LV_DLL_EXPORT GA_RESULT seek_audio_file(int32_t refnum, uint64_t offset, uint64_t* new_offset);
// Read a chunk of audio data from the file and update the file position ready for the next read.
// The output_buffer variable needs to be allocated prior to calling this function, and should be channels x samples_to_read x sizeof(type)
extern "C" LV_DLL_EXPORT GA_RESULT read_audio_file(int32_t refnum, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer);
// Write a chunk of audio data to the file and update the file position ready for the next write.
extern "C" LV_DLL_EXPORT GA_RESULT write_audio_file(int32_t refnum, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written);
// Close the audio file and release any resources allocated in the refnum.
extern "C" LV_DLL_EXPORT GA_RESULT close_audio_file(int32_t refnum);

// Determine the codec of the audio file
GA_RESULT get_audio_file_codec(const char* file_name, ga_codec* codec);

//////////////////////////////
// LabVIEW Audio Device API //
//////////////////////////////
// Query the available backends (WASAPI, DirectSound, WinMM, etc).
extern "C" LV_DLL_EXPORT GA_RESULT query_audio_backends(uint16_t* backends, uint16_t* num_backends);
// Query available devices for a given backend. Set backend greater than ma_backend_null to query the default backend.
extern "C" LV_DLL_EXPORT GA_RESULT query_audio_devices(uint16_t backend, uint8_t* playback_device_ids, int32_t* num_playback_devices, uint8_t* capture_device_ids, int32_t* num_capture_devices);
// Get audio device info for a given backend. Set backend greater than ma_backend_null to query the default backend.
extern "C" LV_DLL_EXPORT GA_RESULT get_audio_device_info(uint16_t backend, const uint8_t* device_id, uint16_t device_type, char* device_name);
// Configure an audio device ready for playback. Will setup the context, device, audio buffers, and callbacks.
extern "C" LV_DLL_EXPORT GA_RESULT configure_audio_device(uint16_t backend, const uint8_t* device_id, uint16_t device_type, uint32_t channels, uint32_t sample_rate, uint16_t format, uint8_t exclusive_mode, int32_t buffer_size, int32_t* refnum);
// Get info on the configured audio device, primarily for allocating memory in LabVIEW
extern "C" LV_DLL_EXPORT GA_RESULT get_configured_audio_device_info(int32_t refnum, uint32_t * sample_rate, uint32_t * channels, uint16_t * format, uint8_t * exclusive_mode);
// Start the audio device playback or capture.
extern "C" LV_DLL_EXPORT GA_RESULT start_audio_device(int32_t refnum);
// Write audio data to the device's buffer for playback. Will block if the audio buffer is full.
extern "C" LV_DLL_EXPORT GA_RESULT playback_audio(int32_t refnum, void* buffer, int32_t num_frames, ga_data_type audio_type);
// Read audio data from the device's buffer. Will block until the specified number of frames has been captured.
extern "C" LV_DLL_EXPORT GA_RESULT capture_audio(int32_t refnum, void* buffer, int32_t* num_frames, ga_data_type audio_type);
// Wait until the buffer has been emptied by the playback_callback routine.
extern "C" LV_DLL_EXPORT GA_RESULT playback_wait(int32_t refnum);
// Stop the audio device from playing. Doesn't clear the buffer.
extern "C" LV_DLL_EXPORT GA_RESULT stop_audio_device(int32_t refnum);
// Uninitializes the audio device. Will also uninitialize the backend context if no other audio devices are active.
extern "C" LV_DLL_EXPORT GA_RESULT clear_audio_device(int32_t refnum);
// Uninitializes the backend context. Will also uninitialize all audio devices.
extern "C" LV_DLL_EXPORT GA_RESULT clear_audio_backend();

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void stop_callback(ma_device* pDevice);
inline ma_bool32 device_is_started(ma_device* pDevice);
inline GA_RESULT check_and_start_audio_device(ma_device* pDevice);

/////////////////////////
// FLAC codec wrappers //
/////////////////////////

inline GA_RESULT convert_flac_result(int32_t result);
GA_RESULT get_flac_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample);
int16_t* load_flac(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result);
void free_flac(int16_t* buffer);
GA_RESULT open_flac_file(const char* file_name, void** decoder);
GA_RESULT get_basic_flac_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
GA_RESULT seek_flac_file(void* decoder, uint64_t offset, uint64_t* new_offset);
GA_RESULT read_flac_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer);
GA_RESULT close_flac_file(void* decoder);


////////////////////////
// MP3 codec wrappers //
////////////////////////

inline GA_RESULT convert_mp3_result(int32_t result);
GA_RESULT get_mp3_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample);
int16_t* load_mp3(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result);
void free_mp3(int16_t* buffer);
GA_RESULT open_mp3_file(const char* file_name, void** decoder);
GA_RESULT get_basic_mp3_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
GA_RESULT seek_mp3_file(void* decoder, uint64_t offset, uint64_t* new_offset);
GA_RESULT read_mp3_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer);
GA_RESULT close_mp3_file(void* decoder);


///////////////////////////
// Vorbis codec wrappers //
///////////////////////////

inline GA_RESULT convert_vorbis_result(int32_t result);
GA_RESULT get_vorbis_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample);
int16_t* load_vorbis(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result);
void free_vorbis(int16_t* buffer);
GA_RESULT open_vorbis_file(const char* file_name, void** decoder);
GA_RESULT get_basic_vorbis_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
GA_RESULT seek_vorbis_file(void* decoder, uint64_t offset, uint64_t* new_offset);
GA_RESULT read_vorbis_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer);
GA_RESULT close_vorbis_file(void* decoder);


////////////////////////
// WAV codec wrappers //
////////////////////////

typedef struct
{
	uint32_t container_format;
	uint32_t data_format;
} wav_specific;

inline GA_RESULT convert_wav_result(int32_t result);
GA_RESULT get_wav_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample);
int16_t* load_wav(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result);
void free_wav(int16_t* sample_data);
GA_RESULT open_wav_file(const char* file_name, void** decoder);
GA_RESULT open_wav_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, void* codec_specific, void** encoder);
GA_RESULT get_basic_wav_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset);
GA_RESULT seek_wav_file(void* decoder, uint64_t offset, uint64_t* new_offset);
GA_RESULT read_wav_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer);
GA_RESULT write_wav_file(void* encoder, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written);
GA_RESULT close_wav_file(void* decoder);


/////////////////////////
// Ancillary functions //
/////////////////////////

#if defined(_WIN32)
// https://gist.github.com/xebecnan/6d070c93fb69f40c3673
// Convert an ASCII / UTF8 string to a UTF-16 LE wide char representation.
// Use for interfacing with wide Win32 file APIs.
// Assumes strings are null terminated (which they should be, coming from LabVIEW)
wchar_t* widen(const char* utf8_string)
{
	if (utf8_string == NULL)
		return NULL;

	UINT codepage = CP_UTF8; // GetACP();
	int utf8_length = strlen(utf8_string);
	int wide_length = MultiByteToWideChar(codepage, 0, utf8_string, utf8_length, 0, 0);
	wchar_t* wide_string = (wchar_t*)malloc((wide_length + 1) * sizeof(wchar_t));

	if (wide_string != NULL)
	{
		MultiByteToWideChar(codepage, 0, utf8_string, utf8_length, wide_string, wide_length);
		wide_string[wide_length] = L'\0';
	}

	return wide_string;
}
#endif


//////////////////////////
// Conversion functions //
//////////////////////////
void u8_to_s16(int16_t* buffer_out, const uint8_t* buffer_in, size_t num_samples)
{
	int32_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		int32_t x = buffer_in[i];
		r = x << 8;
		r = r - 32768;
		buffer_out[i] = (int16_t)r;
	}
}

void u8_to_s32(int32_t* buffer_out, const uint8_t* buffer_in, size_t num_samples)
{
	int64_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		int64_t x = buffer_in[i];
		r = x << 24;
		r = r - 2147483648;
		buffer_out[i] = (int32_t)r;
	}
}

void u8_to_f32(float* buffer_out, const uint8_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		float x = buffer_in[i];
		x = x * 0.00784313725490196078f;    /* 0..255 to 0..2 */
		x = x - 1;                          /* 0..2 to -1..1 */
		buffer_out[i] = x;
	}
}

void u8_to_f64(double* buffer_out, const uint8_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		double x = buffer_in[i];
		x = x * 0.00784313725490196078;    /* 0..255 to 0..2 */
		x = x - 1;                          /* 0..2 to -1..1 */
		buffer_out[i] = x;
	}
}

void s16_to_u8(uint8_t* buffer_out, const int16_t* buffer_in, size_t num_samples)
{
	int32_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		int32_t x = buffer_in[i];
		r = x + 32768;
		r = r >> 8;
		buffer_out[i] = (uint8_t)r;
	}
}

void s16_to_s32(int32_t* buffer_out, const int16_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = buffer_in[i] << 16;
	}
}

void s16_to_f32(float* buffer_out, const int16_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = buffer_in[i] * 0.000030517578125f;
	}
}

void s16_to_f64(double* buffer_out, const int16_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; ++i)
	{
		buffer_out[i] = buffer_in[i] * 0.000030517578125;
	}
}

void s32_to_u8(uint8_t* buffer_out, const int32_t* buffer_in, size_t num_samples)
{
	int64_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		int64_t x = buffer_in[i];
		r = x + 2147483648;
		r = r >> 24;
		buffer_out[i] = (uint8_t)r;
	}
}

void s32_to_s16(int16_t* buffer_out, const int32_t* buffer_in, size_t num_samples)
{
	int32_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		int32_t x = buffer_in[i];
		r = x >> 16;
		buffer_out[i] = (int16_t)r;
	}
}

void s32_to_f32(float* buffer_out, const int32_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = buffer_in[i] / 2147483648.0f;
	}
}

void s32_to_f64(double* buffer_out, const int32_t* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = buffer_in[i] / 2147483648.0;
	}
}

void f32_to_u8(uint8_t* buffer_out, const float* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		float x = buffer_in[i];
		float c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		buffer_out[i] = (uint8_t)(c * 127.5f);
	}
}

void f32_to_s16(int16_t* buffer_out, const float* buffer_in, size_t num_samples)
{
	int32_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		float x = buffer_in[i];
		float c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int32_t)(c * 32767.5f);
		r = r - 32768;
		buffer_out[i] = (int16_t)r;
	}
}

void f32_to_s32(int32_t* buffer_out, const float* buffer_in, size_t num_samples)
{
	int64_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		float x = buffer_in[i];
		float c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int64_t)(c * 2147483647.5f);
		r = r - 2147483648;
		buffer_out[i] = (int32_t)r;
	}
}

void f32_to_f64(double* buffer_out, const float* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = (double)buffer_in[i];
	}
}

void f64_to_u8(uint8_t* buffer_out, const double* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		double x = buffer_in[i];
		double c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		buffer_out[i] = (uint8_t)(c * 127.5);
	}
}

void f64_to_s16(int16_t* buffer_out, const double* buffer_in, size_t num_samples)
{
	int32_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		double x = buffer_in[i];
		double c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int32_t)(c * 32767.5);
		r = r - 32768;
		buffer_out[i] = (int16_t)r;
	}
}

void f64_to_s32(int32_t* buffer_out, const double* buffer_in, size_t num_samples)
{
	int64_t r;
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		double x = buffer_in[i];
		double c;
		c = ((x < -1) ? -1 : ((x > 1) ? 1 : x));
		c = c + 1;
		r = (int64_t)(c * 2147483647.5);
		r = r - 2147483648;
		buffer_out[i] = (int32_t)r;
	}
}

void f64_to_f32(float* buffer_out, const double* buffer_in, size_t num_samples)
{
	size_t i;

	if (buffer_out == NULL || buffer_in == NULL)
	{
		return;
	}

	for (i = 0; i < num_samples; i++)
	{
		buffer_out[i] = (float)buffer_in[i];
	}
}

//////////////////////////////////////////////////////////////////////
// Modified functions from dr_flac, dr_wav, minimp3, and stb_vorbis //
// Primarily adds wchar versions of win32 API functions             //
//////////////////////////////////////////////////////////////////////

// No decode versions of MP3 functions, used for determining accurate sample length.
// Performs all the frame parsing etc, but skips the CPU intensive decode step.
// See https://github.com/lieff/minimp3/issues/54
int mp3dec_decode_frame_no_decode(mp3dec_t *dec, const uint8_t *mp3, int mp3_bytes, mp3d_sample_t *pcm, mp3dec_frame_info_t *info)
{
	int i = 0, igr, frame_size = 0, success = 1;
	const uint8_t *hdr;
	bs_t bs_frame[1];
	mp3dec_scratch_t scratch;

	if (mp3_bytes > 4 && dec->header[0] == 0xff && hdr_compare(dec->header, mp3))
	{
		frame_size = hdr_frame_bytes(mp3, dec->free_format_bytes) + hdr_padding(mp3);
		if (frame_size != mp3_bytes && (frame_size + HDR_SIZE > mp3_bytes || !hdr_compare(mp3, mp3 + frame_size)))
		{
			frame_size = 0;
		}
	}
	if (!frame_size)
	{
		memset(dec, 0, sizeof(mp3dec_t));
		i = mp3d_find_frame(mp3, mp3_bytes, &dec->free_format_bytes, &frame_size);
		if (!frame_size || i + frame_size > mp3_bytes)
		{
			info->frame_bytes = i;
			return 0;
		}
	}

	hdr = mp3 + i;
	memcpy(dec->header, hdr, HDR_SIZE);
	info->frame_bytes = i + frame_size;
	info->frame_offset = i;
	info->channels = HDR_IS_MONO(hdr) ? 1 : 2;
	info->hz = hdr_sample_rate_hz(hdr);
	info->layer = 4 - HDR_GET_LAYER(hdr);
	info->bitrate_kbps = hdr_bitrate_kbps(hdr);

	if (!pcm)
	{
		return hdr_frame_samples(hdr);
	}

	bs_init(bs_frame, hdr + HDR_SIZE, frame_size - HDR_SIZE);
	if (HDR_IS_CRC(hdr))
	{
		get_bits(bs_frame, 16);
	}

	if (info->layer == 3)
	{
		int main_data_begin = L3_read_side_info(bs_frame, scratch.gr_info, hdr);
		if (main_data_begin < 0 || bs_frame->pos > bs_frame->limit)
		{
			mp3dec_init(dec);
			return 0;
		}
		success = L3_restore_reservoir(dec, bs_frame, &scratch, main_data_begin);
		if (success)
		{
			/* for (igr = 0; igr < (HDR_TEST_MPEG1(hdr) ? 2 : 1); igr++, pcm += 576 * info->channels)
			{
				memset(scratch.grbuf[0], 0, 576 * 2 * sizeof(float));
				L3_decode(dec, &scratch, scratch.gr_info + igr*info->channels, info->channels);
				mp3d_synth_granule(dec->qmf_state, scratch.grbuf[0], 18, info->channels, pcm, scratch.syn[0]);
			} */
		}
		L3_save_reservoir(dec, &scratch);
	}
	else
	{
#ifdef MINIMP3_ONLY_MP3
		return 0;
#else /* MINIMP3_ONLY_MP3 */
		L12_scale_info sci[1];
		L12_read_scale_info(hdr, bs_frame, sci);

		memset(scratch.grbuf[0], 0, 576 * 2 * sizeof(float));
		for (i = 0, igr = 0; igr < 3; igr++)
		{
			if (12 == (i += L12_dequantize_granule(scratch.grbuf[0] + i, bs_frame, sci, info->layer | 1)))
			{
				i = 0;
				/* L12_apply_scf_384(sci, sci->scf + igr, scratch.grbuf[0]);
				mp3d_synth_granule(dec->qmf_state, scratch.grbuf[0], 12, info->channels, pcm, scratch.syn[0]);
				memset(scratch.grbuf[0], 0, 576 * 2 * sizeof(float));
				pcm += 384 * info->channels; */
			}
			if (bs_frame->pos > bs_frame->limit)
			{
				mp3dec_init(dec);
				return 0;
			}
		}
#endif /* MINIMP3_ONLY_MP3 */
	}
	return success*hdr_frame_samples(dec->header);
}

int mp3dec_load_cb_no_decode(mp3dec_t *dec, mp3dec_io_t *io, uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
	if (!dec || !buf || !info || (size_t)-1 == buf_size || (io && buf_size < MINIMP3_BUF_SIZE))
		return MP3D_E_PARAM;
	uint64_t detected_samples = 0;
	size_t orig_buf_size = buf_size;
	int to_skip = 0;
	mp3dec_frame_info_t frame_info;
	memset(info, 0, sizeof(*info));
	memset(&frame_info, 0, sizeof(frame_info));

	/* skip id3 */
	size_t filled = 0, consumed = 0;
	int eof = 0, ret = 0;
	if (io)
	{
		if (io->seek(0, io->seek_data))
			return MP3D_E_IOERROR;
		filled = io->read(buf, MINIMP3_ID3_DETECT_SIZE, io->read_data);
		if (filled > MINIMP3_ID3_DETECT_SIZE)
			return MP3D_E_IOERROR;
		if (MINIMP3_ID3_DETECT_SIZE != filled)
			return 0;
		size_t id3v2size = mp3dec_skip_id3v2(buf, filled);
		if (id3v2size)
		{
			if (io->seek(id3v2size, io->seek_data))
				return MP3D_E_IOERROR;
			filled = io->read(buf, buf_size, io->read_data);
			if (filled > buf_size)
				return MP3D_E_IOERROR;
		}
		else
		{
			size_t readed = io->read(buf + MINIMP3_ID3_DETECT_SIZE, buf_size - MINIMP3_ID3_DETECT_SIZE, io->read_data);
			if (readed > (buf_size - MINIMP3_ID3_DETECT_SIZE))
				return MP3D_E_IOERROR;
			filled += readed;
		}
		if (filled < MINIMP3_BUF_SIZE)
			mp3dec_skip_id3v1(buf, &filled);
	}
	else
	{
		mp3dec_skip_id3((const uint8_t **)&buf, &buf_size);
		if (!buf_size)
			return 0;
	}
	/* try to make allocation size assumption by first frame or vbr tag */
	mp3dec_init(dec);
	int samples;
	do
	{
		uint32_t frames;
		int i, delay, padding, free_format_bytes = 0, frame_size = 0;
		const uint8_t *hdr;
		if (io)
		{
			if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
			{   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
				memmove(buf, buf + consumed, filled - consumed);
				filled -= consumed;
				consumed = 0;
				size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
				if (readed >(buf_size - filled))
					return MP3D_E_IOERROR;
				if (readed != (buf_size - filled))
					eof = 1;
				filled += readed;
				if (eof)
					mp3dec_skip_id3v1(buf, &filled);
			}
			i = mp3d_find_frame(buf + consumed, filled - consumed, &free_format_bytes, &frame_size);
			consumed += i;
			hdr = buf + consumed;
		}
		else
		{
			i = mp3d_find_frame(buf, buf_size, &free_format_bytes, &frame_size);
			buf += i;
			buf_size -= i;
			hdr = buf;
		}
		if (i && !frame_size)
			continue;
		if (!frame_size)
			return 0;
		frame_info.channels = HDR_IS_MONO(hdr) ? 1 : 2;
		frame_info.hz = hdr_sample_rate_hz(hdr);
		frame_info.layer = 4 - HDR_GET_LAYER(hdr);
		frame_info.bitrate_kbps = hdr_bitrate_kbps(hdr);
		frame_info.frame_bytes = frame_size;
		samples = hdr_frame_samples(hdr)*frame_info.channels;
		if (3 != frame_info.layer)
			break;
		int ret = mp3dec_check_vbrtag(hdr, frame_size, &frames, &delay, &padding);
		if (ret > 0)
		{
			padding *= frame_info.channels;
			to_skip = delay*frame_info.channels;
			detected_samples = samples*(uint64_t)frames;
			if (detected_samples >= (uint64_t)to_skip)
				detected_samples -= to_skip;
			if (padding > 0 && detected_samples >= (uint64_t)padding)
				detected_samples -= padding;
			if (!detected_samples)
				return 0;
		}
		if (ret)
		{
			if (io)
			{
				consumed += frame_size;
			}
			else
			{
				buf += frame_size;
				buf_size -= frame_size;
			}
		}
		break;
	} while (1);
	size_t allocated = MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t);
	info->buffer = (mp3d_sample_t*)malloc(allocated);
	if (!info->buffer)
		return MP3D_E_MEMORY;
	/* save info */
	info->channels = frame_info.channels;
	info->hz = frame_info.hz;
	info->layer = frame_info.layer;
	/* decode all frames */
	size_t avg_bitrate_kbps = 0, frames = 0;
	do
	{
		if (io)
		{
			if (!eof && filled - consumed < MINIMP3_BUF_SIZE)
			{   /* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
				memmove(buf, buf + consumed, filled - consumed);
				filled -= consumed;
				consumed = 0;
				size_t readed = io->read(buf + filled, buf_size - filled, io->read_data);
				if (readed != (buf_size - filled))
					eof = 1;
				filled += readed;
				if (eof)
					mp3dec_skip_id3v1(buf, &filled);
			}
			samples = mp3dec_decode_frame_no_decode(dec, buf + consumed, filled - consumed, info->buffer, &frame_info);
			consumed += frame_info.frame_bytes;
		}
		else
		{
			samples = mp3dec_decode_frame_no_decode(dec, buf, MINIMP3_MIN(buf_size, (size_t)INT_MAX), info->buffer, &frame_info);
			buf += frame_info.frame_bytes;
			buf_size -= frame_info.frame_bytes;
		}
		if (samples)
		{
			if (info->hz != frame_info.hz || info->layer != frame_info.layer)
			{
				ret = MP3D_E_DECODE;
				break;
			}
			if (info->channels && info->channels != frame_info.channels)
			{
#ifdef MINIMP3_ALLOW_MONO_STEREO_TRANSITION
				info->channels = 0; /* mark file with mono-stereo transition */
#else
				ret = MP3D_E_DECODE;
				break;
#endif
			}
			samples *= frame_info.channels;
			if (to_skip)
			{
				size_t skip = MINIMP3_MIN(samples, to_skip);
				to_skip -= skip;
				samples -= skip;
			}
			info->samples += samples;
			avg_bitrate_kbps += frame_info.bitrate_kbps;
			frames++;
			if (progress_cb)
			{
				ret = progress_cb(user_data, orig_buf_size, orig_buf_size - buf_size, &frame_info);
				if (ret)
					break;
			}
		}
	} while (frame_info.frame_bytes);
	if (detected_samples && info->samples > detected_samples)
		info->samples = detected_samples; /* cut padding */
										  /* reallocate to normal buffer size */
	if (frames)
		info->avg_bitrate_kbps = avg_bitrate_kbps / frames;
	return ret;
}

int mp3dec_load_buf_no_decode(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
	return mp3dec_load_cb_no_decode(dec, 0, (uint8_t *)buf, buf_size, info, progress_cb, user_data);
}

static int mp3dec_load_mapinfo_no_decode(mp3dec_t *dec, mp3dec_map_info_t *map_info, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
	int ret = mp3dec_load_buf_no_decode(dec, map_info->buffer, map_info->size, info, progress_cb, user_data);
	mp3dec_close_file(map_info);
	return ret;
}

#if defined(_WIN32)
int mp3dec_load_w_no_decode(mp3dec_t *dec, const wchar_t *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
	int ret;
	mp3dec_map_info_t map_info;
	if ((ret = mp3dec_open_file_w(file_name, &map_info)))
		return ret;
	return mp3dec_load_mapinfo_no_decode(dec, &map_info, info, progress_cb, user_data);
}
#endif //_WIN32

int mp3dec_load_no_decode(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
	int ret;
	mp3dec_map_info_t map_info;
	if ((ret = mp3dec_open_file(file_name, &map_info)))
		return ret;
	return mp3dec_load_mapinfo_no_decode(dec, &map_info, info, progress_cb, user_data);
}

#ifndef DR_FLAC_NO_STDIO
#if defined(_WIN32)
DRFLAC_API drflac_int16* drflac_open_file_and_read_pcm_frames_s16_w(const wchar_t* filename, unsigned int* channels, unsigned int* sampleRate, drflac_uint64* totalPCMFrameCount, const drflac_allocation_callbacks* pAllocationCallbacks)
{
	drflac* pFlac;

	if (sampleRate) {
		*sampleRate = 0;
	}
	if (channels) {
		*channels = 0;
	}
	if (totalPCMFrameCount) {
		*totalPCMFrameCount = 0;
	}

	pFlac = drflac_open_file_w(filename, pAllocationCallbacks);
	if (pFlac == NULL) {
		return NULL;
	}

	return drflac__full_read_and_close_s16(pFlac, channels, sampleRate, totalPCMFrameCount);
}
#endif // _WIN32
#endif // DR_FLAC_NO_STDIO

#ifndef STB_VORBIS_NO_STDIO
#if defined(_WIN32)
stb_vorbis * stb_vorbis_open_filename_w(const wchar_t *filename, int *error, const stb_vorbis_alloc *alloc)
{
	FILE *f;
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
	if (0 != _wfopen_s(&f, filename, L"rb"))
		f = NULL;
#else
	f = _wfopen(filename, "rb");
#endif
	if (f)
		return stb_vorbis_open_file(f, TRUE, error, alloc);
	if (error) *error = VORBIS_file_open_failure;
	return NULL;
}

int stb_vorbis_decode_filename_w(const wchar_t *filename, int *channels, int *sample_rate, short **output)
{
	int data_len, offset, total, limit, error;
	short *data;
	stb_vorbis *v = stb_vorbis_open_filename_w(filename, &error, NULL);
	if (v == NULL) return -1;
	limit = v->channels * 4096;
	*channels = v->channels;
	if (sample_rate)
		*sample_rate = v->sample_rate;
	offset = data_len = 0;
	total = limit;
	data = (short *)malloc(total * sizeof(*data));
	if (data == NULL) {
		stb_vorbis_close(v);
		return -2;
	}
	for (;;) {
		int n = stb_vorbis_get_frame_short_interleaved(v, v->channels, data + offset, total - offset);
		if (n == 0) break;
		data_len += n;
		offset += n * v->channels;
		if (offset + limit > total) {
			short *data2;
			total *= 2;
			data2 = (short *)realloc(data, total * sizeof(*data));
			if (data2 == NULL) {
				free(data);
				stb_vorbis_close(v);
				return -2;
			}
			data = data2;
		}
	}
	*output = data;
	stb_vorbis_close(v);
	return data_len;
}
#endif // _WIN32
#endif // STB_VORBIS_NO_STDIO

// Get the ma_backend enum given a string. Performs reverse of ma_get_backend_name().
MA_API ma_backend ma_get_backend_enum(const char* backend_name)
{
	if (strcmp(backend_name, "WASAPI") == 0)      return ma_backend_wasapi;
	if (strcmp(backend_name, "DirectSound") == 0) return ma_backend_dsound;
	if (strcmp(backend_name, "WinMM") == 0)       return ma_backend_winmm;
	if (strcmp(backend_name, "Core Audio") == 0)  return ma_backend_coreaudio;
	if (strcmp(backend_name, "sndio") == 0)       return ma_backend_sndio;
	if (strcmp(backend_name, "audio(4)") == 0)    return ma_backend_audio4;
	if (strcmp(backend_name, "OSS") == 0)         return ma_backend_oss;
	if (strcmp(backend_name, "PulseAudio") == 0)  return ma_backend_pulseaudio;
	if (strcmp(backend_name, "ALSA") == 0)        return ma_backend_alsa;
	if (strcmp(backend_name, "JACK") == 0)        return ma_backend_jack;
	if (strcmp(backend_name, "AAudio") == 0)      return ma_backend_aaudio;
	if (strcmp(backend_name, "OpenSL|ES") == 0)   return ma_backend_opensl;
	if (strcmp(backend_name, "Web Audio") == 0)   return ma_backend_webaudio;
	if (strcmp(backend_name, "Custom") == 0)      return ma_backend_custom;
	
	return ma_backend_null;
}

#endif
