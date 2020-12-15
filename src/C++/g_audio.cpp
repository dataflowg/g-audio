/*
G-Audio - An audio library for LabVIEW.

See g_audio.h for license details.
*/

#include "stdafx.h"
// Single header decoder definitions. These must appear before including g_audio.h.
#define DR_FLAC_IMPLEMENTATION
#define MINIMP3_IMPLEMENTATION
//#define MINIMP3_FLOAT_OUTPUT
#define DR_WAV_IMPLEMENTATION
#include "g_audio.h"


///////////////////////////
// LabVIEW API functions //
///////////////////////////

extern "C" LV_DLL_EXPORT GA_RESULT get_audio_file_info(const char* codec, const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	if (!strcmp(codec, "flac"))
	{
		return get_flac_info(file_name, num_samples, channels, sample_rate, bits_per_sample);
	}
	else if (!strcmp(codec, "mp3"))
	{
		return get_mp3_info(file_name, num_samples, channels, sample_rate, bits_per_sample);
	}
	else if (!strcmp(codec, "vorbis"))
	{
		return get_vorbis_info(file_name, num_samples, channels, sample_rate, bits_per_sample);
	}
	else if (!strcmp(codec, "wav"))
	{
		return get_wav_info(file_name, num_samples, channels, sample_rate, bits_per_sample);
	}
	else
	{
		return GA_E_UNSUPPORTED;
	}

	return GA_E_GENERIC;
}

extern "C" LV_DLL_EXPORT int16_t* load_audio_file_s16(const char* codec, const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	int16_t* sample_data = NULL;

	if (!strcmp(codec, "flac"))
	{
		sample_data = load_flac(file_name, num_samples, channels, sample_rate, result);
	}
	else if (!strcmp(codec, "mp3"))
	{
		sample_data = load_mp3(file_name, num_samples, channels, sample_rate, result);
	}
	else if (!strcmp(codec, "vorbis"))
	{
		sample_data = load_vorbis(file_name, num_samples, channels, sample_rate, result);
	}
	else if (!strcmp(codec, "wav"))
	{
		sample_data = load_wav(file_name, num_samples, channels, sample_rate, result);
	}
	else
	{
		// Unsupported codec type
		*result = GA_E_UNSUPPORTED;
		return NULL;
	}

	if (*result != GA_SUCCESS)
	{
		sample_data = NULL;
	}

	return sample_data;
}

extern "C" LV_DLL_EXPORT void free_sample_data(int16_t* sample_data)
{
	free(sample_data);
}

extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file(const char* codec, const char* file_name, int32_t* refnum)
{
	audio_file_codec* audio_file = (audio_file_codec*)malloc(sizeof(audio_file_codec));
	if (audio_file == NULL)
	{
		return GA_E_MEMORY;
	}

	audio_file->file_mode = read_mode;
#if defined(_WIN32)
	strcpy_s(audio_file->codec_name, codec);
#else
	strncpy(audio_file->codec_name, codec, CODEC_NAME_LEN-1);
	audio_file->codec_name[CODEC_NAME_LEN-1] = '\0';
#endif
	audio_file->decoder = NULL;
	audio_file->encoder = NULL;
	audio_file->read_offset = 0;

	if (!strcmp(codec, "flac"))
	{
		audio_file->open = open_flac_file;
		audio_file->get_basic_info = get_basic_flac_file_info;
		audio_file->seek = seek_flac_file;
		audio_file->read = read_flac_file;
		audio_file->close = close_flac_file;
		// These should never be called in read mode
		audio_file->open_write = NULL;
		audio_file->write = NULL;
	}
	else if (!strcmp(codec, "mp3"))
	{
		audio_file->open = open_mp3_file;
		audio_file->get_basic_info = get_basic_mp3_file_info;
		audio_file->seek = seek_mp3_file;
		audio_file->read = read_mp3_file;
		audio_file->close = close_mp3_file;
		// These should never be called in read mode
		audio_file->open_write = NULL;
		audio_file->write = NULL;
	}
	else if (!strcmp(codec, "vorbis"))
	{
		audio_file->open = open_vorbis_file;
		audio_file->get_basic_info = get_basic_vorbis_file_info;
		audio_file->seek = seek_vorbis_file;
		audio_file->read = read_vorbis_file;
		audio_file->close = close_vorbis_file;
		// These should never be called in read mode
		audio_file->open_write = NULL;
		audio_file->write = NULL;
	}
	else if (!strcmp(codec, "wav"))
	{
		audio_file->open = open_wav_file;
		audio_file->get_basic_info = get_basic_wav_file_info;
		audio_file->seek = seek_wav_file;
		audio_file->read = read_wav_file;
		audio_file->close = close_wav_file;
		// These should never be called in read mode
		audio_file->open_write = NULL;
		audio_file->write = NULL;
	}
	else
	{
		free(audio_file);
		return GA_E_UNSUPPORTED;
	}

	if (audio_file->open == NULL)
	{
		return GA_E_GENERIC;
	}
	void* decoder;
	if (audio_file->open(file_name, &decoder) != GA_SUCCESS)
	{
		free(audio_file);
		return GA_E_DECODER;
	}
	else
	{
		audio_file->decoder = decoder;
		thread_mutex_init(&(audio_file->mutex));
		*refnum = create_insert_refnum_data((void*)audio_file);
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT open_audio_file_write(const char* codec, const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, int32_t has_specific_info, void* codec_specific, int32_t* refnum)
{
	audio_file_codec* audio_file = (audio_file_codec*)malloc(sizeof(audio_file_codec));
	if (audio_file == NULL)
	{
		return GA_E_MEMORY;
	}

	audio_file->file_mode = write_mode;
#if defined(_WIN32)
	strcpy_s(audio_file->codec_name, codec);
#else
	strncpy(audio_file->codec_name, codec, CODEC_NAME_LEN - 1);
	audio_file->codec_name[CODEC_NAME_LEN - 1] = '\0';
#endif
	audio_file->decoder = NULL;
	audio_file->encoder = NULL;
	audio_file->read_offset = 0;

	if (!strcmp(codec, "wav"))
	{
		audio_file->open_write = open_wav_file_write;
		audio_file->write = write_wav_file;
		audio_file->close = close_wav_file;
		// These should never be called in write mode
		audio_file->open = NULL;
		audio_file->get_basic_info = NULL;
		audio_file->seek = NULL;
		audio_file->read = NULL;
	}
	else
	{
		free(audio_file);
		return GA_E_UNSUPPORTED;
	}

	if (audio_file->open_write == NULL)
	{
		return GA_E_GENERIC;
	}
	void* encoder;
	if (audio_file->open_write(file_name, channels, sample_rate, bits_per_sample, (has_specific_info != 0 ? codec_specific : NULL), &encoder) != GA_SUCCESS)
	{
		free(audio_file);
		return GA_E_DECODER;
	}
	else
	{
		audio_file->encoder = encoder;
		thread_mutex_init(&(audio_file->mutex));
		*refnum = create_insert_refnum_data((void*)audio_file);
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT GA_RESULT get_basic_audio_file_info(int32_t refnum, uint32_t* channels, uint32_t* sample_rate, uint64_t * read_offset)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != read_mode)
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

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != read_mode)
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

extern "C" LV_DLL_EXPORT GA_RESULT read_audio_file(int32_t refnum, uint64_t samples_to_read, audio_return_type_t audio_type, uint64_t* samples_read, void* output_buffer)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != read_mode)
	{
		return GA_E_READ_MODE;
	}

	if (audio_file->read == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->read(audio_file->decoder, samples_to_read, audio_type, samples_read, output_buffer);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT write_audio_file(int32_t refnum, uint64_t samples_to_write, void* sample_data, uint64_t* samples_written)
{
	GA_RESULT result = GA_SUCCESS;

	audio_file_codec* audio_file = (audio_file_codec*)get_reference_data(refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}
	else if (audio_file->file_mode != write_mode)
	{
		return GA_E_WRITE_MODE;
	}

	if (audio_file->write == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	result = audio_file->write(audio_file->encoder, samples_to_write, sample_data, samples_written);
	thread_mutex_unlock(&(audio_file->mutex));

	return result;
}

extern "C" LV_DLL_EXPORT GA_RESULT close_audio_file(int32_t refnum)
{
	audio_file_codec* audio_file = (audio_file_codec*)remove_reference(refnum);

	if (audio_file == NULL)
	{
		return GA_E_REFNUM;
	}

	if (audio_file->close == NULL)
	{
		return GA_E_GENERIC;
	}
	thread_mutex_lock(&(audio_file->mutex));
	if (audio_file->file_mode == read_mode)
	{
		audio_file->close(audio_file->decoder);
	}
	else if (audio_file->file_mode == write_mode)
	{
		audio_file->close(audio_file->encoder);
	}
	thread_mutex_unlock(&(audio_file->mutex));
	thread_mutex_term(&(audio_file->mutex));

	free(audio_file);

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

GA_RESULT get_flac_info(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

	*num_samples = pFlac->totalPCMFrameCount;
	*channels = pFlac->channels;
	*sample_rate = pFlac->sampleRate;
	*bits_per_sample = pFlac->bitsPerSample;

	drflac_close(pFlac);

	return GA_SUCCESS;
}

int16_t* load_flac(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	drflac_uint64 flac_num_samples = 0;
	drflac_uint32 flac_channels = 0;
	drflac_uint32 flac_sample_rate = 0;
	int16_t* sample_data = NULL;

#if defined(_WIN32)
	sample_data = drflac_open_file_and_read_pcm_frames_s16_w(widen(file_name), &flac_channels, &flac_sample_rate, &flac_num_samples, NULL);
#else
	sample_data = drflac_open_file_and_read_pcm_frames_s16(file_name, &flac_channels, &flac_sample_rate, &flac_num_samples, NULL);
#endif

	if (sample_data == NULL)
	{
		// Failed to open and decode FLAC file.
		*result = GA_E_GENERIC;
	}

	*num_samples = flac_num_samples;
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

GA_RESULT read_flac_file(void* decoder, uint64_t samples_to_read, audio_return_type_t audio_type, uint64_t* samples_read, void* output_buffer)
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
	case as_U8:
		temp_buffer = malloc(samples_to_read * ((drflac*)decoder)->channels * sizeof(drflac_int16));
		*samples_read = drflac_read_pcm_frames_s16((drflac*)decoder, samples_to_read, (drflac_int16*)temp_buffer);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((drflac*)decoder)->channels);
		free(temp_buffer);
		break;
	case as_I16:
		*samples_read = drflac_read_pcm_frames_s16((drflac*)decoder, samples_to_read, (drflac_int16*)output_buffer);
		break;
	case as_I32:
		*samples_read = drflac_read_pcm_frames_s32((drflac*)decoder, samples_to_read, (drflac_int32*)output_buffer);
		break;
	case as_SGL:
		*samples_read = drflac_read_pcm_frames_f32((drflac*)decoder, samples_to_read, (float*)output_buffer);
		break;
	case as_DBL:
		temp_buffer = malloc(samples_to_read * ((drflac*)decoder)->channels * sizeof(drflac_int32));
		*samples_read = drflac_read_pcm_frames_s32((drflac*)decoder, samples_to_read, (drflac_int32*)temp_buffer);
		s32_to_f64((double*)output_buffer, (int32_t*)temp_buffer, samples_to_read * ((drflac*)decoder)->channels);
		free(temp_buffer);
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

GA_RESULT get_mp3_info(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

	*num_samples = info.samples / info.channels;
	*channels = info.channels;
	*sample_rate = info.hz;
	// Lossy codecs don't have a true "bits per sample", it's all floating point math. minimp3 decodes to 16-bit.
	*bits_per_sample = 16;

	free_mp3(info.buffer);

	return result;
}

int16_t* load_mp3(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
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

	// MP3 decoder samples are in terms of total samples (ie. samples * channels), and not samples/ch like the other decoders or LabVIEW wrapper.
	// Divide result by channels to get samples/ch.
	*num_samples = info.samples / info.channels;
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
	
	// MP3 decoder offset values are in terms of total samples (ie. samples * channels), and not samples/ch like the other decoders or LabVIEW wrapper.
	// Multiply by channels to get actual number of samples to read.
	return convert_mp3_result(mp3dec_ex_seek((mp3dec_ex_t*)decoder, offset * ((mp3dec_ex_t*)decoder)->info.channels));
}

GA_RESULT read_mp3_file(void* decoder, uint64_t samples_to_read, audio_return_type_t audio_type, uint64_t* samples_read, void* output_buffer)
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
	case as_U8:
		temp_buffer = malloc(samples_to_read * num_channels * sizeof(mp3d_sample_t));
		*samples_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, samples_to_read * num_channels) / num_channels;
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * num_channels);
		free(temp_buffer);
		break;
	case as_I16:
		*samples_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)output_buffer, samples_to_read * num_channels) / num_channels;
		break;
	case as_I32:
		temp_buffer = malloc(samples_to_read * num_channels * sizeof(mp3d_sample_t));
		*samples_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, samples_to_read * num_channels) / num_channels;
		s16_to_s32((int32_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * num_channels);
		free(temp_buffer);
		break;
	case as_SGL:
		temp_buffer = malloc(samples_to_read * num_channels * sizeof(mp3d_sample_t));
		*samples_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, samples_to_read * num_channels) / num_channels;
		s16_to_f32((float*)output_buffer, (int16_t*)temp_buffer, samples_to_read * num_channels);
		free(temp_buffer);
		break;
	case as_DBL:
		temp_buffer = malloc(samples_to_read * num_channels * sizeof(mp3d_sample_t));
		*samples_read = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)temp_buffer, samples_to_read * num_channels) / num_channels;
		s16_to_f64((double*)output_buffer, (int16_t*)temp_buffer, samples_to_read * num_channels);
		free(temp_buffer);
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	if (*samples_read < 0)
	{
		*samples_read = 0;
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

GA_RESULT get_vorbis_info(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

	*num_samples = (uint64_t)stb_vorbis_stream_length_in_samples(info);
	*channels = (uint32_t)info->channels;
	*sample_rate = (uint32_t)info->sample_rate;
	// Lossy codecs don't have a true "bits per sample", it's all floating point math. stb_vorbis decodes to 16-bit.
	*bits_per_sample = 16;

	stb_vorbis_close(info);

	return result;
}

int16_t* load_vorbis(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	int ogg_num_samples = 0;
	int ogg_channels = 0;
	int ogg_sample_rate = 0;
	int ogg_error_code = 0;
	short* sample_data = NULL;

#if defined(_WIN32)
	ogg_num_samples = stb_vorbis_decode_filename_w(widen(file_name), &ogg_channels, &ogg_sample_rate, &sample_data);
#else
	ogg_num_samples = stb_vorbis_decode_filename(file_name, &ogg_channels, &ogg_sample_rate, &sample_data);
#endif

	if (ogg_num_samples < 0)
	{
		*result = GA_E_GENERIC;
		return NULL;
	}

	*num_samples = (uint64_t)ogg_num_samples;
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

GA_RESULT read_vorbis_file(void* decoder, uint64_t samples_to_read, audio_return_type_t audio_type, uint64_t* samples_read, void* output_buffer)
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
	case as_U8:
		temp_buffer = malloc(samples_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*samples_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		break;
	case as_I16:
		*samples_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		break;
	case as_I32:
		temp_buffer = malloc(samples_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*samples_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_s32((int32_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		break;
	case as_SGL:
		temp_buffer = malloc(samples_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*samples_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_f32((float*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
		break;
	case as_DBL:
		temp_buffer = malloc(samples_to_read * ((stb_vorbis*)decoder)->channels * sizeof(short));
		*samples_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis*)decoder, ((stb_vorbis*)decoder)->channels, (short*)output_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		s16_to_f64((double*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((stb_vorbis*)decoder)->channels);
		free(temp_buffer);
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

GA_RESULT get_wav_info(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

	*num_samples = pWav.totalPCMFrameCount;
	*channels = pWav.channels;
	*sample_rate = pWav.sampleRate;
	*bits_per_sample = pWav.bitsPerSample;

	drwav_uninit(&pWav);

	return GA_SUCCESS;
}

int16_t* load_wav(const char* file_name, uint64_t* num_samples, uint32_t* channels, uint32_t* sample_rate, GA_RESULT* result)
{
	drwav_uint64 wav_num_samples = 0;
	drwav_uint32 wav_channels = 0;
	drwav_uint32 wav_sample_rate = 0;
	int16_t* sample_data = NULL;

#if defined(_WIN32)
	sample_data = drwav_open_file_and_read_pcm_frames_s16_w(widen(file_name), &wav_channels, &wav_sample_rate, &wav_num_samples, NULL);
#else
	sample_data = drwav_open_file_and_read_pcm_frames_s16(file_name, &wav_channels, &wav_sample_rate, &wav_num_samples, NULL);
#endif

	if (sample_data == NULL)
	{
		*result = GA_E_GENERIC;
	}

	*num_samples = wav_num_samples;
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

	if (drwav__is_compressed_format_tag(((drwav*)decoder)->translatedFormatTag))
	{
		*read_offset = ((drwav*)decoder)->compressed.iCurrentPCMFrame;
	}
	else
	{
		uint32_t bytes_per_pcm_frame = drwav_get_bytes_per_pcm_frame((drwav*)decoder);
		uint64_t total_byte_count = ((drwav*)decoder)->totalPCMFrameCount * bytes_per_pcm_frame;
		*read_offset = (total_byte_count - ((drwav*)decoder)->bytesRemaining) / bytes_per_pcm_frame;
	}

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

GA_RESULT read_wav_file(void* decoder, uint64_t samples_to_read, audio_return_type_t audio_type, uint64_t* samples_read, void* output_buffer)
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
		case as_U8:
		temp_buffer = malloc(samples_to_read * ((drwav*)decoder)->channels * sizeof(drwav_int16));
		*samples_read = drwav_read_pcm_frames_s16((drwav*)decoder, samples_to_read, (drwav_int16*)temp_buffer);
		s16_to_u8((uint8_t*)output_buffer, (int16_t*)temp_buffer, samples_to_read * ((drwav*)decoder)->channels);
		free(temp_buffer);
		break;
	case as_I16:
		*samples_read = drwav_read_pcm_frames_s16((drwav*)decoder, samples_to_read, (drwav_int16*)output_buffer);
		break;
	case as_I32:
		*samples_read = drwav_read_pcm_frames_s32((drwav*)decoder, samples_to_read, (drwav_int32*)output_buffer);
		break;
	case as_SGL:
		*samples_read = drwav_read_pcm_frames_f32((drwav*)decoder, samples_to_read, (float*)output_buffer);
		break;
	case as_DBL:
		temp_buffer = malloc(samples_to_read * ((drwav*)decoder)->channels * sizeof(float));
		*samples_read = drwav_read_pcm_frames_f32((drwav*)decoder, samples_to_read, (float*)temp_buffer);
		f32_to_f64((double*)output_buffer, (float*)temp_buffer, samples_to_read * ((drwav*)decoder)->channels);
		free(temp_buffer);
		break;
	default:
		return GA_E_INVALID_TYPE;
		break;
	}

	return GA_SUCCESS;
}

GA_RESULT write_wav_file(void* encoder, uint64_t samples_to_write, void* sample_data, uint64_t* samples_written)
{
	if (encoder == NULL)
	{
		return GA_E_GENERIC;
	}

	if (sample_data == NULL)
	{
		return GA_E_MEMORY;
	}

	// TODO
	// If wav configured as PCM and input is float, convert to int and scale range
	// If wav configured as IEEE Float and input is integer, convert to float and scale range
	// Or simply throw error?

	*samples_written = drwav_write_pcm_frames((drwav*)encoder, samples_to_write, sample_data);

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