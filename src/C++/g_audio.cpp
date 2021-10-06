/*
G-Audio - An audio library for LabVIEW.

See g_audio.h for license details.
*/

#include "stdafx.h"
#include "g_audio.h"

//////////////////////
// Global variables //
//////////////////////
ma_context* global_context = NULL;

////////////////////////////
// LabVIEW Audio File API //
////////////////////////////

extern "C" LV_DLL_EXPORT GA_RESULT get_audio_file_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample, ga_codec* codec)
{
	GA_RESULT result;

	result = get_audio_file_codec(file_name, codec);

	if (result != GA_SUCCESS)
	{
		return result;
	}

	switch (*codec)
	{
		case ga_codec_flac: return get_flac_info(file_name, num_frames, channels, sample_rate, bits_per_sample); break;
		case ga_codec_mp3: return get_mp3_info(file_name, num_frames, channels, sample_rate, bits_per_sample); break;
		case ga_codec_vorbis: return get_vorbis_info(file_name, num_frames, channels, sample_rate, bits_per_sample); break;
		case ga_codec_wav: return get_wav_info(file_name, num_frames, channels, sample_rate, bits_per_sample); break;
		case ga_codec_unsupported: return GA_E_UNSUPPORTED;
		default: break;
	}

	return GA_E_GENERIC;
}

extern "C" LV_DLL_EXPORT int16_t* load_audio_file_s16(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_codec* codec, GA_RESULT* result)
{
	int16_t* sample_data = NULL;

	*result = get_audio_file_codec(file_name, codec);

	if (*result != GA_SUCCESS)
	{
		return NULL;
	}

	switch (*codec)
	{
		case ga_codec_flac: sample_data = load_flac(file_name, num_frames, channels, sample_rate, result); break;
		case ga_codec_mp3: sample_data = load_mp3(file_name, num_frames, channels, sample_rate, result); break;
		case ga_codec_vorbis: sample_data = load_vorbis(file_name, num_frames, channels, sample_rate, result); break;
		case ga_codec_wav: sample_data = load_wav(file_name, num_frames, channels, sample_rate, result); break;
		case ga_codec_unsupported: *result = GA_E_UNSUPPORTED; break;
		default: *result = GA_E_GENERIC; break;
	}

	if (*result != GA_SUCCESS)
	{
		free_sample_data(sample_data);
		sample_data = NULL;
	}

	return sample_data;
}

extern "C" LV_DLL_EXPORT void free_sample_data(int16_t* buffer)
{
	free(buffer);
}

extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file(const char* file_name, int32_t* refnum)
{
	GA_RESULT result;
	ga_codec codec;
	audio_file_codec* audio_file = (audio_file_codec*)malloc(sizeof(audio_file_codec));
	if (audio_file == NULL)
	{
		return GA_E_MEMORY;
	}

	audio_file->file_mode = ga_file_mode_read;

	result = get_audio_file_codec(file_name, &codec);

	if (result != GA_SUCCESS)
	{
		return result;
	}

	audio_file->codec = codec;
	audio_file->decoder = NULL;
	audio_file->encoder = NULL;
	audio_file->read_offset = 0;

	switch (audio_file->codec)
	{
		case ga_codec_flac:
			audio_file->open = open_flac_file;
			audio_file->get_basic_info = get_basic_flac_file_info;
			audio_file->seek = seek_flac_file;
			audio_file->read = read_flac_file;
			audio_file->close = close_flac_file;
			// These should never be called in read mode
			audio_file->open_write = NULL;
			audio_file->write = NULL;
			break;
		case ga_codec_mp3:
			audio_file->open = open_mp3_file;
			audio_file->get_basic_info = get_basic_mp3_file_info;
			audio_file->seek = seek_mp3_file;
			audio_file->read = read_mp3_file;
			audio_file->close = close_mp3_file;
			// These should never be called in read mode
			audio_file->open_write = NULL;
			audio_file->write = NULL;
			break;
		case ga_codec_vorbis:
			audio_file->open = open_vorbis_file;
			audio_file->get_basic_info = get_basic_vorbis_file_info;
			audio_file->seek = seek_vorbis_file;
			audio_file->read = read_vorbis_file;
			audio_file->close = close_vorbis_file;
			// These should never be called in read mode
			audio_file->open_write = NULL;
			audio_file->write = NULL;
			break;
		case ga_codec_wav:
			audio_file->open = open_wav_file;
			audio_file->get_basic_info = get_basic_wav_file_info;
			audio_file->seek = seek_wav_file;
			audio_file->read = read_wav_file;
			audio_file->close = close_wav_file;
			// These should never be called in read mode
			audio_file->open_write = NULL;
			audio_file->write = NULL;
			break;
		default:
			free(audio_file);
			audio_file = NULL;
			return GA_E_UNSUPPORTED;
			break;
	}

	if (audio_file->open == NULL)
	{
		free(audio_file);
		audio_file = NULL;
		return GA_E_GENERIC;
	}
	void* decoder;
	if (audio_file->open(file_name, &decoder) != GA_SUCCESS)
	{
		free(audio_file);
		audio_file = NULL;
		return GA_E_DECODER;
	}
	else
	{
		audio_file->decoder = decoder;
		thread_mutex_init(&(audio_file->mutex));
		*refnum = create_insert_refnum_data(ga_refnum_audio_file, (void*)audio_file);

		if (*refnum < 0)
		{
			thread_mutex_term(&(audio_file->mutex));
			free(audio_file);
			audio_file = NULL;
			return GA_E_REFNUM_LIMIT;
		}
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, ga_codec codec, int32_t has_specific_info, void* codec_specific, int32_t* refnum)
{
	audio_file_codec* audio_file = (audio_file_codec*)malloc(sizeof(audio_file_codec));
	if (audio_file == NULL)
	{
		return GA_E_MEMORY;
	}

	audio_file->file_mode = ga_file_mode_write;
	audio_file->codec = codec;
	audio_file->decoder = NULL;
	audio_file->encoder = NULL;
	audio_file->read_offset = 0;

	switch (audio_file->codec)
	{
		case ga_codec_wav:
			audio_file->open_write = open_wav_file_write;
			audio_file->write = write_wav_file;
			audio_file->close = close_wav_file;
			// These should never be called in write mode
			audio_file->open = NULL;
			audio_file->get_basic_info = NULL;
			audio_file->seek = NULL;
			audio_file->read = NULL;
			break;
		default:
			free(audio_file);
			audio_file = NULL;
			return GA_E_UNSUPPORTED;
			break;
	}

	if (audio_file->open_write == NULL)
	{
		free(audio_file);
		audio_file = NULL;
		return GA_E_GENERIC;
	}
	void* encoder;
	if (audio_file->open_write(file_name, channels, sample_rate, bits_per_sample, (has_specific_info != 0 ? codec_specific : NULL), &encoder) != GA_SUCCESS)
	{
		free(audio_file);
		audio_file = NULL;
		return GA_E_DECODER;
	}
	else
	{
		audio_file->encoder = encoder;
		thread_mutex_init(&(audio_file->mutex));
		*refnum = create_insert_refnum_data(ga_refnum_audio_file, (void*)audio_file);

		if (*refnum < 0)
		{
			thread_mutex_term(&(audio_file->mutex));
			free(audio_file);
			audio_file = NULL;
			return GA_E_REFNUM_LIMIT;
		}
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT get_basic_audio_file_info(int32_t refnum, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(ga_refnum_audio_file, refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != ga_file_mode_read)
	{
		return GA_E_READ_MODE;
	}

	if (audio_file->get_basic_info == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->get_basic_info(audio_file->decoder, channels, sample_rate, read_offset);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT seek_audio_file(int32_t refnum, uint64_t offset, uint64_t* new_offset)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(ga_refnum_audio_file, refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != ga_file_mode_read)
	{
		return GA_E_READ_MODE;
	}

	if (audio_file->seek == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->seek(audio_file->decoder, offset, new_offset);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT read_audio_file(int32_t refnum, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(ga_refnum_audio_file, refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != ga_file_mode_read)
	{
		return GA_E_READ_MODE;
	}

	if (audio_file->read == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->read(audio_file->decoder, frames_to_read, audio_type, frames_read, output_buffer);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT write_audio_file(int32_t refnum, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(ga_refnum_audio_file, refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != ga_file_mode_write)
	{
		return GA_E_WRITE_MODE;
	}

	if (audio_file->write == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->write(audio_file->encoder, frames_to_write, input_buffer, frames_written);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT close_audio_file(int32_t refnum)
{
	audio_file_codec* audio_file = (audio_file_codec*)remove_reference(ga_refnum_audio_file, refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}

	if (audio_file->close == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	if (audio_file->file_mode == ga_file_mode_read)
	{
		audio_file->close(audio_file->decoder);
	}
	else if (audio_file->file_mode == ga_file_mode_write)
	{
		audio_file->close(audio_file->encoder);
	}
	thread_mutex_unlock(&(audio_file->mutex));
	thread_mutex_term(&(audio_file->mutex));

	free(audio_file);
	audio_file = NULL;

	return GA_SUCCESS;
}

GA_RESULT get_audio_file_codec(const char* file_name, ga_codec* codec)
{
	FILE* pFile;

	*codec = ga_codec_unsupported;

#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
	if (0 != _wfopen_s(&pFile, widen(file_name), L"rb"))
		pFile = NULL;
#elif defined(_WIN32)
	pFile = _wfopen(widen(file_name), L"rb");
#else
	pFile = fopen(file_name, "rb");
#endif
	if (pFile == NULL)
	{
		return GA_E_FILE;
	}

	char signature[4];

	fseek(pFile, 0, SEEK_SET);
	fread(&signature, 1, sizeof(signature), pFile);
	fclose(pFile);

	// FLAC
	if (!strncmp(signature, "fLaC", 4))
	{
		*codec = ga_codec_flac;
	}
	// MP3 (ID3v2)
	else if (!strncmp(signature, "ID3", 3))
	{
		*codec = ga_codec_mp3;
	}
	// MP3 (no tag or ID3V1)
	else if ((uint8_t)signature[0] == 0xFF && ((uint8_t)signature[1] == 0xFB || (uint8_t)signature[1] == 0xF3 || (uint8_t)signature[1] == 0xF2))
	{
		*codec = ga_codec_mp3;
	}
	// Ogg Vorbis
	else if (!strncmp(signature, "OggS", 4))
	{
		*codec = ga_codec_vorbis;
	}
	// WAV (RIFF format)
	else if (!strncmp(signature, "RIFF", 4))
	{
		*codec = ga_codec_wav;
	}
	// WAV (Wave64 format)
	else if (!strncmp(signature, "riff", 4))
	{
		*codec = ga_codec_wav;
	}
	else
	{
		*codec = ga_codec_unsupported;
		return GA_E_UNSUPPORTED;
	}
	return GA_SUCCESS;
}

///////////////////////////
// FLAC decoding wrapper //
///////////////////////////

inline GA_RESULT convert_flac_result(int32_t result)
{
	switch (result)
	{
	case DRFLAC_SUCCESS:
		return GA_SUCCESS; break;
	default:
		return GA_E_GENERIC; break;
	}

	return GA_E_GENERIC;
}

GA_RESULT get_flac_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	drflac* pFlac = NULL;

#if defined(_WIN32)
	pFlac = drflac_open_file_w(widen(file_name), NULL);
#else
	pFlac = drflac_open_file(file_name, NULL);
#endif
	if (pFlac == NULL)
	{
		return GA_E_DECODER;
	}

	*num_frames = pFlac->totalPCMFrameCount;
	*channels = pFlac->channels;
	*sample_rate = pFlac->sampleRate;
	*bits_per_sample = pFlac->bitsPerSample;

	drflac_close(pFlac);

	return GA_SUCCESS;
}

int16_t* load_flac(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	drflac_uint64 flac_num_frames = 0;
	drflac_uint32 flac_channels = 0;
	drflac_uint32 flac_sample_rate = 0;
	int16_t* sample_data = NULL;

#if defined(_WIN32)
	sample_data = drflac_open_file_and_read_pcm_frames_s16_w(widen(file_name), &flac_channels, &flac_sample_rate, &flac_num_frames, NULL);
#else
	sample_data = drflac_open_file_and_read_pcm_frames_s16(file_name, &flac_channels, &flac_sample_rate, &flac_num_frames, NULL);
#endif

	if (sample_data == NULL)
	{
		// Failed to open and decode FLAC file.
		*result = GA_E_GENERIC;
	}

	*num_frames = flac_num_frames;
	*channels = flac_channels;
	*sample_rate = flac_sample_rate;
	*result = GA_SUCCESS;

	return sample_data;
}

void free_flac(int16_t* sample_data)
{
	drflac_free(sample_data, NULL);
}

GA_RESULT open_flac_file(const char* file_name, void** decoder)
{
#if defined(_WIN32)
	*decoder = (void*)drflac_open_file_w(widen(file_name), NULL);
#else
	*decoder = (void*)drflac_open_file(file_name, NULL);
#endif

	if (*decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	return GA_SUCCESS;
}

GA_RESULT get_basic_flac_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	*channels = ((drflac*)decoder)->channels;
	*sample_rate = ((drflac*)decoder)->sampleRate;
	*read_offset = ((drflac*)decoder)->currentPCMFrame;

	return GA_SUCCESS;
}

GA_RESULT seek_flac_file(void* decoder, uint64_t offset, uint64_t* new_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (drflac_seek_to_pcm_frame((drflac*)decoder, offset) == DRFLAC_FALSE)
	{
		return GA_E_DECODER;
	}
	
	*new_offset = offset;

	return GA_SUCCESS;
}

GA_RESULT read_flac_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (output_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	void* temp_buffer = NULL;

	switch (audio_type)
	{
	case ga_data_type_u8:
		temp_buffer = malloc(frames_to_read * ((drflac*)decoder)->channels * sizeof(drflac_int16));
		*frames_read = drflac_read_pcm_frames_s16((drflac*)decoder, frames_to_read, (drflac_int16*)temp_buffer);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((drflac*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_i16:
		*frames_read = drflac_read_pcm_frames_s16((drflac*)decoder, frames_to_read, (drflac_int16*)output_buffer);
		break;
	case ga_data_type_i32:
		*frames_read = drflac_read_pcm_frames_s32((drflac*)decoder, frames_to_read, (drflac_int32*)output_buffer);
		break;
	case ga_data_type_float:
		*frames_read = drflac_read_pcm_frames_f32((drflac*)decoder, frames_to_read, (float*)output_buffer);
		break;
	case ga_data_type_double:
		temp_buffer = malloc(frames_to_read * ((drflac*)decoder)->channels * sizeof(drflac_int32));
		*frames_read = drflac_read_pcm_frames_s32((drflac*)decoder, frames_to_read, (drflac_int32*)temp_buffer);
		s32_to_f64((double*)output_buffer, (int32_t*)temp_buffer, frames_to_read * ((drflac*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	return GA_SUCCESS;
}

GA_RESULT close_flac_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	drflac_close((drflac*)decoder);

	return GA_SUCCESS;
}


//////////////////////////
// MP3 decoding wrapper //
//////////////////////////

inline GA_RESULT convert_mp3_result(int32_t result)
{
	switch (result)
	{
	case 0:
		return GA_SUCCESS; break;
	case MP3D_E_PARAM:
		return GA_E_DECODER; break;
	case MP3D_E_MEMORY:
		return GA_E_MEMORY; break;
	case MP3D_E_IOERROR:
		return GA_E_FILE; break;
	case MP3D_E_USER:
		return GA_E_GENERIC; break;
	case MP3D_E_DECODE:
		return GA_E_DECODER; break;
	default:
		return GA_E_GENERIC; break;
	}

	return GA_E_GENERIC;
}

GA_RESULT get_mp3_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	GA_RESULT result = GA_SUCCESS;
	mp3dec_t mp3d;
	mp3dec_file_info_t info;

#if defined(_WIN32)
	result = convert_mp3_result(mp3dec_load_w_no_decode(&mp3d, widen(file_name), &info, NULL, NULL));
#else
	result = convert_mp3_result(mp3dec_load_no_decode(&mp3d, file_name, &info, NULL, NULL));
#endif

	if (result != GA_SUCCESS)
	{
		return result;
	}
	else if (info.channels <= 0)
	{
		return GA_E_DECODER;
	}

	*num_frames = info.samples / info.channels;
	*channels = info.channels;
	*sample_rate = info.hz;
	// Lossy codecs don't have a true "bits per sample", it's all floating point math. minimp3 decodes to 16-bit.
	*bits_per_sample = 16;

	free_mp3(info.buffer);

	return result;
}

int16_t* load_mp3(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	mp3dec_t mp3d;
	mp3dec_file_info_t info;

#if defined(_WIN32)
	*result = convert_mp3_result(mp3dec_load_w(&mp3d, widen(file_name), &info, NULL, NULL));
#else
	*result = convert_mp3_result(mp3dec_load(&mp3d, file_name, &info, NULL, NULL));
#endif

	if (*result != GA_SUCCESS)
	{
		return NULL;
	}
	else if (info.channels <= 0)
	{
		*result = GA_E_DECODER;
		return NULL;
	}

	// MP3 decoder samples (ie. samples * channels), and not frames like the other decoders or LabVIEW wrapper. Convert to frames.
	*num_frames = info.samples / info.channels;
	*channels = info.channels;
	*sample_rate = info.hz;

	return info.buffer;
}

void free_mp3(int16_t* sample_data)
{
	free(sample_data);
}

GA_RESULT open_mp3_file(const char* file_name, void** decoder)
{
	GA_RESULT result = GA_SUCCESS;

	mp3dec_ex_t* mp3_decoder = (mp3dec_ex_t*)malloc(sizeof(mp3dec_ex_t));

	if (mp3_decoder == NULL)
	{
		return GA_E_MEMORY;
	}

#if defined(_WIN32)
	result = convert_mp3_result(mp3dec_ex_open_w(mp3_decoder, widen(file_name), MP3D_SEEK_TO_SAMPLE));
#else
	result = convert_mp3_result(mp3dec_ex_open(mp3_decoder, file_name, MP3D_SEEK_TO_SAMPLE));
#endif

	if (result != GA_SUCCESS)
	{
		free(mp3_decoder);
		mp3_decoder = NULL;
		return result;
	}

	*decoder = (void*)mp3_decoder;

	return result;
}

GA_RESULT get_basic_mp3_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	*channels = ((mp3dec_ex_t*)decoder)->info.channels;
	*sample_rate = ((mp3dec_ex_t*)decoder)->info.hz;
	*read_offset = ((mp3dec_ex_t*)decoder)->cur_sample / ((mp3dec_ex_t*)decoder)->info.channels;

	return GA_SUCCESS;
}

GA_RESULT seek_mp3_file(void* decoder, uint64_t offset, uint64_t* new_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}
	
	// MP3 decoder offset values are in terms of samples, and not frames like the other decoders or LabVIEW wrapper.
	// Multiply by channels to get actual number of samples to read.
	return convert_mp3_result(mp3dec_ex_seek((mp3dec_ex_t*)decoder, offset * ((mp3dec_ex_t*)decoder)->info.channels));
}

GA_RESULT read_mp3_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	int32_t num_channels = ((mp3dec_ex_t*)decoder)->info.channels;

	if (num_channels <= 0)
	{
		return GA_E_DECODER;
	}

	if (output_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	void* temp_buffer = NULL;

	// MP3 decoder offset values are in terms of total samples (ie. samples * channels), and not samples/ch like the other decoders or LabVIEW wrapper.
	// Multiply by channels to get actual number of samples to read. Divide result by channels to get samples/ch.
	switch (audio_type)
	{
	case ga_data_type_u8:
		temp_buffer = malloc(frames_to_read * num_channels * sizeof(mp3d_sample_t));
		*frames_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, frames_to_read * num_channels) / num_channels;
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * num_channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_i16:
		*frames_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)output_buffer, frames_to_read * num_channels) / num_channels;
		break;
	case ga_data_type_i32:
		temp_buffer = malloc(frames_to_read * num_channels * sizeof(mp3d_sample_t));
		*frames_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, frames_to_read * num_channels) / num_channels;
		s16_to_s32((int32_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * num_channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_float:
		temp_buffer = malloc(frames_to_read * num_channels * sizeof(mp3d_sample_t));
		*frames_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, frames_to_read * num_channels) / num_channels;
		s16_to_f32((float*)output_buffer, (int16_t*)temp_buffer, frames_to_read * num_channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_double:
		temp_buffer = malloc(frames_to_read * num_channels * sizeof(mp3d_sample_t));
		*frames_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, frames_to_read * num_channels) / num_channels;
		s16_to_f64((double*)output_buffer, (int16_t*)temp_buffer, frames_to_read * num_channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	if (*frames_read < 0)
	{
		*frames_read = 0;
		return GA_E_GENERIC;
	}

	return GA_SUCCESS;
}

GA_RESULT close_mp3_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	mp3dec_ex_close((mp3dec_ex_t*)decoder);
	free(decoder);

	return GA_SUCCESS;
}


/////////////////////////////////
// Ogg Vorbis decoding wrapper //
/////////////////////////////////

inline GA_RESULT convert_vorbis_result(int32_t result)
{
	switch (result)
	{
	case VORBIS__no_error:
		return GA_SUCCESS; break;
	case VORBIS_outofmem:
		return GA_E_MEMORY; break;
	case VORBIS_file_open_failure:
	case VORBIS_unexpected_eof:
		return GA_E_FILE; break;
	case VORBIS_invalid_setup:
	case VORBIS_invalid_stream:
	case VORBIS_missing_capture_pattern:
	case VORBIS_invalid_stream_structure_version:
	case VORBIS_continued_packet_flag_invalid:
	case VORBIS_incorrect_stream_serial_number:
	case VORBIS_invalid_first_page:
	case VORBIS_bad_packet_type:
	case VORBIS_cant_find_last_page:
	case VORBIS_seek_failed:
	case VORBIS_ogg_skeleton_not_supported:
		return GA_E_DECODER; break;
	default:
		return GA_E_GENERIC; break;
	}

	return GA_E_GENERIC;
}

GA_RESULT get_vorbis_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	GA_RESULT result = GA_SUCCESS;
	int error = 0;
	stb_vorbis* info;

#if defined(_WIN32)
	info = stb_vorbis_open_filename_w(widen(file_name), &error, NULL);
#else
	info = stb_vorbis_open_filename(file_name, &error, NULL);
#endif

	result = convert_vorbis_result(error);

	if (info == NULL || result != GA_SUCCESS)
	{
		return result;
	}

	*num_frames = (uint64_t)stb_vorbis_stream_length_in_samples(info);
	*channels = (uint32_t)info->channels;
	*sample_rate = (uint32_t)info->sample_rate;
	// Lossy codecs don't have a true "bits per sample", it's all floating point math. stb_vorbis decodes to 16-bit.
	*bits_per_sample = 16;

	stb_vorbis_close(info);

	return result;
}

int16_t* load_vorbis(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	int ogg_num_frames = 0;
	int ogg_channels = 0;
	int ogg_sample_rate = 0;
	int ogg_error_code = 0;
	short* sample_data = NULL;

#if defined(_WIN32)
	ogg_num_frames = stb_vorbis_decode_filename_w(widen(file_name), &ogg_channels, &ogg_sample_rate, &sample_data);
#else
	ogg_num_frames = stb_vorbis_decode_filename(file_name, &ogg_channels, &ogg_sample_rate, &sample_data);
#endif

	if (ogg_num_frames < 0)
	{
		*result = GA_E_GENERIC;
		return NULL;
	}

	*num_frames = (uint64_t)ogg_num_frames;
	*channels = (uint32_t)ogg_channels;
	*sample_rate = (uint32_t)ogg_sample_rate;
	*result = GA_SUCCESS;

	return (int16*)sample_data;
}

void free_vorbis(int16_t* sample_data)
{
	free(sample_data);
}

GA_RESULT open_vorbis_file(const char* file_name, void** decoder)
{
	GA_RESULT result = GA_SUCCESS;
	int error = 0;
	stb_vorbis* vorbis_decoder;

#if defined(_WIN32)
	vorbis_decoder = stb_vorbis_open_filename_w(widen(file_name), &error, NULL);
#else
	vorbis_decoder = stb_vorbis_open_filename(file_name, &error, NULL);
#endif

	result = convert_vorbis_result(error);

	if (vorbis_decoder == NULL || result != GA_SUCCESS)
	{
		return result;
	}

	*decoder = (void*)vorbis_decoder;

	return result;
}

GA_RESULT get_basic_vorbis_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	*channels = ((stb_vorbis*)decoder)->channels;
	*sample_rate = ((stb_vorbis*)decoder)->sample_rate;
	*read_offset = stb_vorbis_get_sample_offset((stb_vorbis*)decoder);

	return GA_SUCCESS;
}

GA_RESULT seek_vorbis_file(void* decoder, uint64_t offset, uint64_t* new_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	// Clear any previous errors
	stb_vorbis_get_error((stb_vorbis*)decoder);

	stb_vorbis_seek((stb_vorbis*)decoder, offset);

	return convert_vorbis_result(stb_vorbis_get_error((stb_vorbis*)decoder));
}

GA_RESULT read_vorbis_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	// Clear any previous errors
	stb_vorbis_get_error((stb_vorbis*)decoder);

	void* temp_buffer = NULL;

	switch (audio_type)
	{
	case ga_data_type_u8:
		temp_buffer = malloc(frames_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_i16:
		*frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		break;
	case ga_data_type_i32:
		temp_buffer = malloc(frames_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_s32((int32_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_float:
		temp_buffer = malloc(frames_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_f32((float*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_double:
		temp_buffer = malloc(frames_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_f64((double*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	return convert_vorbis_result(stb_vorbis_get_error((stb_vorbis*)decoder));
}

GA_RESULT close_vorbis_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	stb_vorbis_close((stb_vorbis*)decoder);

	return GA_SUCCESS;
}


//////////////////////////
// WAV decoding wrapper //
//////////////////////////

inline GA_RESULT convert_wav_result(int32_t result)
{
	switch (result)
	{
	case DRWAV_SUCCESS:
		return GA_SUCCESS; break;
	default:
		return GA_E_GENERIC; break;
	}

	return GA_E_GENERIC;
}

GA_RESULT get_wav_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	drwav pWav;
	drwav_bool32 init_ok = DRWAV_FALSE;

#if defined(_WIN32)
	init_ok = drwav_init_file_w(&pWav, widen(file_name), NULL);
#else
	init_ok = drwav_init_file(&pWav, file_name, NULL);
#endif

	if (init_ok == DRWAV_FALSE)
	{
		return GA_E_DECODER;
	}

	*num_frames = pWav.totalPCMFrameCount;
	*channels = pWav.channels;
	*sample_rate = pWav.sampleRate;
	*bits_per_sample = pWav.bitsPerSample;

	drwav_uninit(&pWav);

	return GA_SUCCESS;
}

int16_t* load_wav(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	drwav_uint64 wav_num_frames = 0;
	drwav_uint32 wav_channels = 0;
	drwav_uint32 wav_sample_rate = 0;
	int16_t* sample_data = NULL;

#if defined(_WIN32)
	sample_data = drwav_open_file_and_read_pcm_frames_s16_w(widen(file_name), &wav_channels, &wav_sample_rate, &wav_num_frames, NULL);
#else
	sample_data = drwav_open_file_and_read_pcm_frames_s16(file_name, &wav_channels, &wav_sample_rate, &wav_num_frames, NULL);
#endif

	if (sample_data == NULL)
	{
		*result = GA_E_GENERIC;
	}

	*num_frames = wav_num_frames;
	*channels = wav_channels;
	*sample_rate = wav_sample_rate;
	*result = GA_SUCCESS;

	return sample_data;
}

void free_wav(int16_t* sample_data)
{
	drwav_free((void*)sample_data, NULL);
}

GA_RESULT open_wav_file(const char* file_name, void** decoder)
{
	drwav_bool32 result = DRWAV_FALSE;
	drwav* wav_decoder = (drwav*)malloc(sizeof(drwav));

	if (wav_decoder == NULL)
	{
		return GA_E_MEMORY;
	}

#if defined(_WIN32)
	result = drwav_init_file_w(wav_decoder, widen(file_name), NULL);
#else
	result = drwav_init_file(wav_decoder, file_name, NULL);
#endif

	if (result == DRWAV_FALSE)
	{
		free(wav_decoder);
		wav_decoder = NULL;
		return GA_E_GENERIC;
	}

	*decoder = (void*)wav_decoder;

	return GA_SUCCESS;
}

GA_RESULT open_wav_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, void* codec_specific, void** encoder)
{
	drwav_bool32 result = DRWAV_FALSE;
	drwav* wav_encoder = (drwav*)malloc(sizeof(drwav));

	if (wav_encoder == NULL)
	{
		return GA_E_MEMORY;
	}

	drwav_data_format format;
	format.channels = channels;
	format.sampleRate = sample_rate;
	format.bitsPerSample = bits_per_sample;

	if (codec_specific != NULL)
	{
		format.container = (drwav_container)((wav_specific*)codec_specific)->container_format;
		format.format = ((wav_specific*)codec_specific)->data_format;
	}
	else
	{
		format.container = drwav_container_riff;
		format.format = DR_WAVE_FORMAT_PCM;
	}

#if defined(_WIN32)
	result = drwav_init_file_write_w(wav_encoder, widen(file_name), &format, NULL);
#else
	result = drwav_init_file_write(wav_encoder, file_name, &format, NULL);
#endif

	if (result == DRWAV_FALSE)
	{
		free(wav_encoder);
		wav_encoder = NULL;
		return GA_E_GENERIC;
	}

	*encoder = (void*)wav_encoder;

	return GA_SUCCESS;
}

GA_RESULT get_basic_wav_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	*channels = ((drwav*)decoder)->channels;
	*sample_rate = ((drwav*)decoder)->sampleRate;
	*read_offset = ((drwav*)decoder)->readCursorInPCMFrames;

	return GA_SUCCESS;
}

GA_RESULT seek_wav_file(void* decoder, uint64_t offset, uint64_t* new_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (drwav_seek_to_pcm_frame((drwav*)decoder, offset) == DRWAV_FALSE)
	{
		return GA_E_DECODER;
	}

	return GA_SUCCESS;
}

GA_RESULT read_wav_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (output_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	void* temp_buffer = NULL;

	switch (audio_type)
	{
		case ga_data_type_u8:
		temp_buffer = malloc(frames_to_read * ((drwav*)decoder)->channels * sizeof(drwav_int16));
		*frames_read = drwav_read_pcm_frames_s16((drwav*)decoder, frames_to_read, (drwav_int16*)temp_buffer);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, frames_to_read * ((drwav*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	case ga_data_type_i16:
		*frames_read = drwav_read_pcm_frames_s16((drwav*)decoder, frames_to_read, (drwav_int16*)output_buffer);
		break;
	case ga_data_type_i32:
		*frames_read = drwav_read_pcm_frames_s32((drwav*)decoder, frames_to_read, (drwav_int32*)output_buffer);
		break;
	case ga_data_type_float:
		*frames_read = drwav_read_pcm_frames_f32((drwav*)decoder, frames_to_read, (float*)output_buffer);
		break;
	case ga_data_type_double:
		temp_buffer = malloc(frames_to_read * ((drwav*)decoder)->channels * sizeof(float));
		*frames_read = drwav_read_pcm_frames_f32((drwav*)decoder, frames_to_read, (float*)temp_buffer);
		f32_to_f64((double*)output_buffer, (float*)temp_buffer, frames_to_read * ((drwav*)decoder)->channels);
		free(temp_buffer);
		temp_buffer = NULL;
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	return GA_SUCCESS;
}

GA_RESULT write_wav_file(void* encoder, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written)
{
	if (encoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (input_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	// TODO
	// If wav configured as PCM and input is float, convert to int and scale range
	// If wav configured as IEEE Float and input is integer, convert to float and scale range
	// Or simply throw error?

	*frames_written = drwav_write_pcm_frames((drwav*)encoder, frames_to_write, input_buffer);

	return GA_SUCCESS;
}

GA_RESULT close_wav_file(void* decoder)
{
	drwav_result result = DRWAV_SUCCESS;

	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	result = drwav_uninit((drwav*)decoder);
	free(decoder);

	if (result != DRWAV_SUCCESS)
	{
		return GA_E_DECODER;
	}

	return GA_SUCCESS;
}





//////////////////////////////
// LabVIEW Audio Device API //
//////////////////////////////

extern "C" LV_DLL_EXPORT GA_RESULT query_audio_backends(uint16_t* backends, uint16_t* num_backends)
{
	ma_context context;
	*num_backends = 0;

	// Thread safety - ma_context_init, ma_context_uninit are unsafe
	lock_ga_mutex(ga_mutex_context);
	for (int i = 0; i < ma_backend_null; i++)
	{
		if (ma_context_init((ma_backend*)&i, 1, NULL, &context) != MA_SUCCESS)
		{
			// Couldn't init a context for the backend, try the next backend
			continue;
		}

		backends[*num_backends] = i;
		(*num_backends)++;

		ma_context_uninit(&context);
	}
	unlock_ga_mutex(ga_mutex_context);

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT query_audio_devices(uint16_t backend_in, uint8_t* playback_device_ids, int32_t* num_playback_devices, uint8_t* capture_device_ids, int32_t* num_capture_devices)
{
	ma_context context;
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	ma_result result;
	uint32_t iDevice;
	ma_backend backend = (ma_backend)backend_in;

	// Thread safety - ma_context_init, ma_context_get_devices, ma_context_uninit are unsafe
	lock_ga_mutex(ga_mutex_context);
	// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
	// The backends enum in LabVIEW adds Default after Null
	result = ma_context_init((backend_in > ma_backend_null ? NULL : &backend), 1, NULL, &context);
	if (result != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return result + MA_ERROR_OFFSET;
	}

	result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return result + MA_ERROR_OFFSET;
	}

	// playback_device_ids and capture_device_ids are 1D arrays, allocated in LabVIEW as MAX_DEVICE_COUNT * 256 bytes, where the 256 bytes holds an ma_device_id union.
	// Each device ID is stored in this array at 256 byte offsets. A 2D array of [MAX_DEVICE_COUNT][256] was considered, but they're a pain to use in LabVIEW with lots
	// of memory copies and cognitive overhead. Reshaping a 1D array to 2D is much easier.
	if (playbackDeviceCount > MAX_DEVICE_COUNT)
	{
		playbackDeviceCount = MAX_DEVICE_COUNT;
	}
	if (captureDeviceCount > MAX_DEVICE_COUNT)
	{
		captureDeviceCount = MAX_DEVICE_COUNT;
	}

	for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice)
	{
		memcpy(&playback_device_ids[iDevice * sizeof(ma_device_id)], &pPlaybackDeviceInfos[iDevice].id, sizeof(ma_device_id));
	}

	for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice)
	{
		memcpy(&capture_device_ids[iDevice * sizeof(ma_device_id)], &pCaptureDeviceInfos[iDevice].id, sizeof(ma_device_id));
	}

	*num_playback_devices = playbackDeviceCount;
	*num_capture_devices = captureDeviceCount;

	ma_context_uninit(&context);

	unlock_ga_mutex(ga_mutex_context);

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT get_audio_device_info(uint16_t backend_in, const uint8_t* device_id, uint16_t device_type, char* device_name)
{
	ma_context context;
	ma_device_id deviceId;
	ma_device_info deviceInfo;
	ma_result result;
	ma_backend backend = (ma_backend)backend_in;

	memcpy(&deviceId, device_id, sizeof(ma_device_id));

	// Thread safety - ma_context_init, ma_context_uninit are unsafe
	lock_ga_mutex(ga_mutex_context);
	// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
	// The backends enum in LabVIEW adds Default after Null
	result = ma_context_init((backend_in > ma_backend_null ? NULL : &backend), 1, NULL, &context);
	if (result != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return result + MA_ERROR_OFFSET;
	}

	result = ma_context_get_device_info(&context, (ma_device_type)device_type, &deviceId, ma_share_mode_shared, &deviceInfo);
	if (result != MA_SUCCESS)
	{
		ma_context_uninit(&context);
		unlock_ga_mutex(ga_mutex_context);
		return result + MA_ERROR_OFFSET;
	}

#if defined(_WIN32)
	strcpy_s(device_name, sizeof(deviceInfo.name), deviceInfo.name);
#else
	strncpy(device_name, deviceInfo.name, sizeof(deviceInfo.name));
	device_name[sizeof(deviceInfo.name)-1] = '\0';
#endif

	ma_context_uninit(&context);

	unlock_ga_mutex(ga_mutex_context);

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT configure_audio_device(uint16_t backend_in, const uint8_t* device_id, uint16_t device_type, uint32_t channels, uint32_t sample_rate, uint16_t format, uint8_t exclusive_mode, int32_t buffer_size, int32_t* refnum)
{
	ma_result result;
	ma_device_id deviceId;
	ma_format device_format_init;
	ma_uint32 device_channels_init;
	ma_uint32 device_internal_buffer_size = 0;
	ma_backend backend = (ma_backend)backend_in;

	uint8_t blank_device_id[sizeof(ma_device_id)] = { 0 };

	audio_device* pDevice = NULL;

	memcpy(&deviceId, device_id, sizeof(ma_device_id));

	lock_ga_mutex(ga_mutex_context);
	//// START CRITICAL SECTION ////
	// Create a context if it doesn't exist.
	if (global_context == NULL)
	{
		global_context = (ma_context*)malloc(sizeof(ma_context));
		if (global_context == NULL)
		{
			unlock_ga_mutex(ga_mutex_context);
			return GA_E_MEMORY;
		}

		ma_context_config context_config = ma_context_config_init();
		// TODO: Make this configurable. Don't set it for now.
		//context_config.threadPriority = ma_thread_priority_realtime;

		// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
		// The backends enum in LabVIEW adds Default after Null
		result = ma_context_init((backend_in > ma_backend_null ? NULL : &backend), 1, &context_config, global_context);
		if (result != MA_SUCCESS)
		{
			free(global_context);
			global_context = NULL;
			unlock_ga_mutex(ga_mutex_context);
			return result + MA_ERROR_OFFSET;
		}
	}
	else if ((backend_in <= ma_backend_null) && (global_context->backend != backend))
	{
		unlock_ga_mutex(ga_mutex_context);
		return GA_E_CONTEXT_BACKEND;
	}
	//// END CRITICAL SECTION ////
	unlock_ga_mutex(ga_mutex_context);

	ma_device_config device_config = ma_device_config_init((ma_device_type)device_type);
	switch (device_config.deviceType)
	{
	case ma_device_type_capture:
	case ma_device_type_loopback:
		device_config.capture.format = (ma_format)format;   // Set to ma_format_unknown to use the device's native format.
		device_config.capture.channels = channels;               // Set to 0 to use the device's native channel count.
		device_config.capture.pDeviceID = memcmp(&deviceId, &blank_device_id, sizeof(ma_device_id)) != 0 ? &deviceId : NULL;
		device_config.sampleRate = sample_rate;           // Set to 0 to use the device's native sample rate.
		device_config.dataCallback = capture_callback;
		device_config.stopCallback = stop_callback;
		device_config.capture.channelMixMode = ma_channel_mix_mode_simple;
		//config.wasapi.noAutoConvertSRC = true; // Enable low latency shared mode
		device_config.capture.shareMode = exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
		break;
	default:
		device_config.playback.format = (ma_format)format;   // Set to ma_format_unknown to use the device's native format.
		device_config.playback.channels = channels;               // Set to 0 to use the device's native channel count.
		device_config.playback.pDeviceID = memcmp(&deviceId, &blank_device_id, sizeof(ma_device_id)) != 0 ? &deviceId : NULL;
		device_config.sampleRate = sample_rate;           // Set to 0 to use the device's native sample rate.
		device_config.dataCallback = playback_callback;
		device_config.stopCallback = stop_callback;
		device_config.playback.channelMixMode = ma_channel_mix_mode_simple;
		//config.wasapi.noAutoConvertSRC = true; // Enable low latency shared mode
		device_config.playback.shareMode = exclusive_mode ? ma_share_mode_exclusive : ma_share_mode_shared;
		break;
	}

	pDevice = (audio_device*)malloc(sizeof(audio_device));
	if (pDevice == NULL)
	{
		return GA_E_MEMORY;
	}

	result = ma_device_init(global_context, &device_config, &pDevice->device);
	if (result != MA_SUCCESS)
	{
		free(pDevice);
		pDevice = NULL;
		return result + MA_ERROR_OFFSET;
	}

	pDevice->buffer_size = buffer_size;

	switch (device_config.deviceType)
	{
	case ma_device_type_capture:
	case ma_device_type_loopback:
		device_format_init = pDevice->device.capture.format;
		device_channels_init = pDevice->device.capture.channels;
		device_internal_buffer_size = pDevice->device.capture.internalPeriodSizeInFrames;
		break;
	default:
		device_format_init = pDevice->device.playback.format;
		device_channels_init = pDevice->device.playback.channels;
		device_internal_buffer_size = pDevice->device.playback.internalPeriodSizeInFrames;
		break;
	}

	result = ma_pcm_rb_init(device_format_init, device_channels_init, buffer_size, NULL, NULL, &pDevice->buffer);
	if (result != MA_SUCCESS)
	{
		ma_device_uninit(&pDevice->device);
		free(pDevice);
		pDevice = NULL;
		return result + MA_ERROR_OFFSET;
	}

	// Store the ring buffer with the device, so the callback routine can access it
	pDevice->device.pUserData = &pDevice->buffer;
	*refnum = create_insert_refnum_data(ga_refnum_audio_device, pDevice);

	if (*refnum < 0)
	{
		ma_pcm_rb_uninit(&pDevice->buffer);
		ma_device_uninit(&pDevice->device);
		free(pDevice);
		pDevice = NULL;

		return GA_E_REFNUM_LIMIT;
	}

	if (buffer_size < device_internal_buffer_size)
	{
		return GA_W_BUFFER_SIZE;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT get_configured_audio_device_info(int32_t refnum, uint32_t* sample_rate, uint32_t* channels, uint16_t* format, uint8_t* exclusive_mode)
{
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	switch (pDevice->device.type)
	{
	case ma_device_type_capture:
	case ma_device_type_loopback:
		*format = pDevice->device.capture.format;
		*channels = pDevice->device.capture.channels;
		*sample_rate = pDevice->device.sampleRate;
		*exclusive_mode = pDevice->device.capture.shareMode;
		break;
	default:
		*format = pDevice->device.playback.format;
		*channels = pDevice->device.playback.channels;
		*sample_rate = pDevice->device.sampleRate;
		*exclusive_mode = pDevice->device.capture.shareMode;
		break;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT start_audio_device(int32_t refnum)
{
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	return check_and_start_audio_device(&pDevice->device);
}

extern "C" LV_DLL_EXPORT GA_RESULT playback_audio(int32_t refnum, void* buffer, int32_t num_frames, ga_data_type audio_type)
{
	ma_result result;
	ma_uint32 framesToWrite = num_frames;
	ma_uint32 pcmFramesProcessed = 0;
	ma_uint32 bufferOffset = 0;
	ma_uint32 bytesToWrite;
	void* pWriteBuffer;
	int i = 0;
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	if (pDevice->device.type != ma_device_type_playback)
	{
		return GA_E_PLAYBACK_MODE;
	}

	if (num_frames > pDevice->buffer_size)
	{
		return GA_E_BUFFER_SIZE;
	}

	void* conversion_buffer = NULL;
	uint64_t num_channel_samples = num_frames * pDevice->device.playback.channels;

	switch (audio_type)
	{
	case ga_data_type_u8:
		conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
		ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, buffer, ma_format_u8, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_i16:
		conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
		ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, buffer, ma_format_s16, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_i32:
		conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
		ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, buffer, ma_format_s32, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_float:
		conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
		ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, buffer, ma_format_f32, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_double:
		switch (pDevice->device.playback.format)
		{
		case ma_format_u8:
			conversion_buffer = malloc(num_channel_samples * sizeof(uint8_t));
			f64_to_u8((uint8_t*)conversion_buffer, (double*)buffer, num_channel_samples);
			break;
		case ma_format_s16:
			conversion_buffer = malloc(num_channel_samples * sizeof(int16_t));
			f64_to_s16((int16_t*)conversion_buffer, (double*)buffer, num_channel_samples);
			break;
		case ma_format_s32:
			conversion_buffer = malloc(num_channel_samples * sizeof(int32_t));
			f64_to_s32((int32_t*)conversion_buffer, (double*)buffer, num_channel_samples);
			break;
		case ma_format_f32:
			conversion_buffer = malloc(num_channel_samples * sizeof(float));
			f64_to_f32((float*)conversion_buffer, (double*)buffer, num_channel_samples);
			break;
		default:
			return GA_E_INVALID_TYPE;
			break;
		}
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	if (conversion_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	result = check_and_start_audio_device(&pDevice->device);
	if (result != GA_SUCCESS)
	{
		return result;
	}

	while ((ma_pcm_rb_available_write(&pDevice->buffer) < num_frames) && ma_device_is_started(&pDevice->device))
	{
		Sleep(1);
	}

	do
	{
		if (!ma_device_is_started(&pDevice->device))
		{
			return GA_E_DEVICE_STOPPED;
		}

		framesToWrite = num_frames - pcmFramesProcessed;

		result = ma_pcm_rb_acquire_write(&pDevice->buffer, &framesToWrite, &pWriteBuffer);
		if (result != MA_SUCCESS)
		{
			return result + MA_ERROR_OFFSET;
		}
		{
			bytesToWrite = framesToWrite * ma_get_bytes_per_frame(pDevice->device.playback.format, pDevice->device.playback.channels);
			memcpy(pWriteBuffer, (ma_uint8*)conversion_buffer + bufferOffset, bytesToWrite);
		}
		result = ma_pcm_rb_commit_write(&pDevice->buffer, framesToWrite, pWriteBuffer);
		if (!((result == MA_SUCCESS) || (result == MA_AT_END)))
		{
			return result + MA_ERROR_OFFSET;
		}
		pcmFramesProcessed += framesToWrite;
		bufferOffset += bytesToWrite;
	} while (pcmFramesProcessed < num_frames);

	if (conversion_buffer != NULL)
	{
		free(conversion_buffer);
		conversion_buffer = NULL;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT capture_audio(int32_t refnum, void* buffer, int32_t* num_frames, ga_data_type audio_type)
{
	ma_result result;
	ma_uint32 framesToRead;
	ma_uint32 numFrames;
	ma_uint32 pcmFramesProcessed = 0;
	ma_uint32 bufferOffset = 0;
	ma_uint32 bytesToRead;
	void* pReadBuffer;
	int i = 0;
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	if (!(pDevice->device.type == ma_device_type_capture || pDevice->device.type == ma_device_type_loopback))
	{
		return GA_E_CAPTURE_MODE;
	}

	if (*num_frames > pDevice->buffer_size)
	{
		return GA_E_BUFFER_SIZE;
	}

	numFrames = *num_frames > 0 ? *num_frames : pDevice->buffer_size;
	uint64_t num_channel_samples = numFrames * pDevice->device.capture.channels;
	void* conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.capture.format));
	if (conversion_buffer == NULL)
	{
		return GA_E_MEMORY;
	}

	result = check_and_start_audio_device(&pDevice->device);
	if (result != GA_SUCCESS)
	{
		return result;
	}

	while ((ma_pcm_rb_available_read(&pDevice->buffer) < numFrames) && ma_device_is_started(&pDevice->device))
	{
		Sleep(1);
	}

	do
	{
		if (!ma_device_is_started(&pDevice->device))
		{
			return GA_E_DEVICE_STOPPED;
		}

		framesToRead = numFrames - pcmFramesProcessed;

		result = ma_pcm_rb_acquire_read(&pDevice->buffer, &framesToRead, &pReadBuffer);
		if (result != MA_SUCCESS)
		{
			return result + MA_ERROR_OFFSET;
		}
		{
			bytesToRead = framesToRead * ma_get_bytes_per_frame(pDevice->device.capture.format, pDevice->device.capture.channels);
			memcpy((ma_uint8*)conversion_buffer + bufferOffset, pReadBuffer, bytesToRead);
		}
		result = ma_pcm_rb_commit_read(&pDevice->buffer, framesToRead, pReadBuffer);
		if (!((result == MA_SUCCESS) || (result == MA_AT_END)))
		{
			return result + MA_ERROR_OFFSET;
		}
		pcmFramesProcessed += framesToRead;
		bufferOffset += bytesToRead;
	} while (pcmFramesProcessed < numFrames);

	switch (audio_type)
	{
	case ga_data_type_u8:
		ma_pcm_convert(buffer, ma_format_u8, conversion_buffer, pDevice->device.capture.format, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_i16:
		ma_pcm_convert(buffer, ma_format_s16, conversion_buffer, pDevice->device.capture.format, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_i32:
		ma_pcm_convert(buffer, ma_format_s32, conversion_buffer, pDevice->device.capture.format, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_float:
		ma_pcm_convert(buffer, ma_format_f32, conversion_buffer, pDevice->device.capture.format, num_channel_samples, ma_dither_mode_none);
		break;
	case ga_data_type_double:
		switch (pDevice->device.capture.format)
		{
		case ma_format_u8:
			u8_to_f64((double*)buffer, (uint8_t*)conversion_buffer, num_channel_samples);
			break;
		case ma_format_s16:
			s16_to_f64((double*)buffer, (int16_t*)conversion_buffer, num_channel_samples);
			break;
		case ma_format_s32:
			s32_to_f64((double*)buffer, (int32_t*)conversion_buffer, num_channel_samples);
			break;
		case ma_format_f32:
			f32_to_f64((double*)buffer, (float*)conversion_buffer, num_channel_samples);
			break;
		default:
			return GA_E_INVALID_TYPE;
			break;
		}
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	if (conversion_buffer != NULL)
	{
		free(conversion_buffer);
		conversion_buffer = NULL;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT playback_wait(int32_t refnum)
{
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	if (pDevice->device.type != ma_device_type_playback)
	{
		return GA_E_PLAYBACK_MODE;
	}

	while ((ma_pcm_rb_available_read(&pDevice->buffer) > 0) && ma_device_is_started(&pDevice->device))
	{
		Sleep(1);
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT stop_audio_device(int32_t refnum)
{
	ma_result result;

	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	switch (ma_device_get_state(&pDevice->device))
	{
		case MA_STATE_STOPPING:
		case MA_STATE_STOPPED:
			return GA_SUCCESS;
			break;
		default:
			break;
	}

	result = ma_device_stop(&pDevice->device);
	if (result != MA_SUCCESS)
	{
		return result + MA_ERROR_OFFSET;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT clear_audio_device(int32_t refnum)
{
	audio_device* pDevice = (audio_device*)remove_reference(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	ma_device_uninit(&pDevice->device);
	ma_pcm_rb_uninit(&pDevice->buffer);
	free(pDevice);
	pDevice = NULL;

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT clear_audio_backend()
{
	ma_result result;

	std::vector<int32_t> refnums = get_all_references(ga_refnum_audio_device);

	for (int i = 0; i < refnums.size(); i++)
	{
		clear_audio_device(refnums[i]);
	}

	lock_ga_mutex(ga_mutex_context);
	if (global_context != NULL)
	{
		result = ma_context_uninit(global_context);
		free(global_context);
		global_context = NULL;
	}
	unlock_ga_mutex(ga_mutex_context);

	if (result != GA_SUCCESS)
	{
		return result + MA_ERROR_OFFSET;
	}

	return GA_SUCCESS;
}

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_uint32 pcmFramesAvailableInRB;
	ma_uint32 pcmFramesProcessed = 0;
	ma_uint8* pRunningOutput = (ma_uint8*)pOutput;

	// Keep going until we've filled the output buffer.
	// The output buffer will be filled from the ring buffer, or zero padded if the ring buffer has insufficient samples.
	do
	{
		ma_uint32 framesRemaining = frameCount - pcmFramesProcessed;

		pcmFramesAvailableInRB = ma_pcm_rb_available_read((ma_pcm_rb*)pDevice->pUserData);
		if (pcmFramesAvailableInRB > 0)
		{
			ma_uint32 framesToRead = (framesRemaining < pcmFramesAvailableInRB) ? framesRemaining : pcmFramesAvailableInRB;
			void* pReadBuffer;

			ma_pcm_rb_acquire_read((ma_pcm_rb*)pDevice->pUserData, &framesToRead, &pReadBuffer);
			{
				memcpy(pRunningOutput, pReadBuffer, framesToRead * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
			}
			ma_pcm_rb_commit_read((ma_pcm_rb*)pDevice->pUserData, framesToRead, pReadBuffer);

			pRunningOutput += framesToRead * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
			pcmFramesProcessed += framesToRead;
		}
		// The ring buffer is empty. Check the device is still started.
		else if (!ma_device_is_started(pDevice))
		{
			break;
		}
		// Take a quick nap while we wait for more audio in the ring buffer.
		else
		{
			Sleep(1);
		}
	} while (pcmFramesProcessed < frameCount);

	(void)pInput;
}

void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_uint32 pcmFramesFreeInRB;
	ma_uint32 pcmFramesProcessed = 0;
	ma_uint8* pRunningInput = (ma_uint8*)pInput;

	// Keep going until we've written everything to the ring buffer buffer.
	// The input buffer will be filled from the ring buffer, or zero padded if the ring buffer has insufficient samples.
	do
	{
		ma_uint32 framesRemaining = frameCount - pcmFramesProcessed;

		pcmFramesFreeInRB = ma_pcm_rb_available_write((ma_pcm_rb*)pDevice->pUserData);
		if (pcmFramesFreeInRB > 0)
		{
			ma_uint32 framesToWrite = (framesRemaining < pcmFramesFreeInRB) ? framesRemaining : pcmFramesFreeInRB;
			void* pWriteBuffer;

			ma_pcm_rb_acquire_write((ma_pcm_rb*)pDevice->pUserData, &framesToWrite, &pWriteBuffer);
			{
				memcpy(pWriteBuffer, pRunningInput, framesToWrite * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
			}
			ma_pcm_rb_commit_write((ma_pcm_rb*)pDevice->pUserData, framesToWrite, pWriteBuffer);

			pRunningInput += framesToWrite * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
			pcmFramesProcessed += framesToWrite;
		}
		// The ring buffer is full. Check the device is still started.
		else if (!ma_device_is_started(pDevice))
		{
			break;
		}
		// Take a quick nap while we wait for room in the ring buffer.
		else
		{
			Sleep(1);
		}
	} while (pcmFramesProcessed < frameCount);

	(void)pOutput;
}

void stop_callback(ma_device* pDevice)
{
	// ma_pcm_rb_reset((ma_pcm_rb*)pDevice->pUserData);
}

inline ma_bool32 device_is_started(ma_device* pDevice)
{
	ma_uint32 state;
	state = ma_device_get_state(pDevice);
	return (state == MA_STATE_STARTED) || (state == MA_STATE_STARTING);
}

inline GA_RESULT check_and_start_audio_device(ma_device* pDevice)
{
	ma_result result;

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	if (!device_is_started(pDevice))
	{
		result = ma_device_start(pDevice);
		if (result != MA_SUCCESS)
		{
			return result + MA_ERROR_OFFSET;
		}
	}

	return GA_SUCCESS;
}
