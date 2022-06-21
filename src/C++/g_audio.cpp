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
// LabVIEW CLFN Callbacks //
////////////////////////////

extern "C" LV_DLL_EXPORT int32_t clfn_abort(void* data)
{
	clear_audio_backend();
	return 0;
}

////////////////////////////
// LabVIEW Audio File API //
////////////////////////////

extern "C" LV_DLL_EXPORT ga_result get_audio_file_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample, ga_codec* codec)
{
	ga_result result;

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
		case ga_codec_unsupported: return GA_E_UNSUPPORTED_CODEC;
		default: break;
	}

	return GA_E_GENERIC;
}

extern "C" LV_DLL_EXPORT int16_t* load_audio_file_s16(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_codec* codec, ga_result* result)
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
		case ga_codec_unsupported: *result = GA_E_UNSUPPORTED_CODEC; break;
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

extern "C" LV_DLL_EXPORT ga_result open_audio_file(const char* file_name, int32_t* refnum)
{
	ga_result result;
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
			return GA_E_UNSUPPORTED_CODEC;
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

extern "C" LV_DLL_EXPORT ga_result open_audio_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, ga_codec codec, int32_t has_specific_info, void* codec_specific, int32_t* refnum)
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
			return GA_E_UNSUPPORTED_CODEC;
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

extern "C" LV_DLL_EXPORT ga_result get_basic_audio_file_info(int32_t refnum, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
{
	ga_result result = GA_SUCCESS;

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

extern "C" LV_DLL_EXPORT ga_result seek_audio_file(int32_t refnum, uint64_t offset, uint64_t* new_offset)
{
	ga_result result = GA_SUCCESS;

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

extern "C" LV_DLL_EXPORT ga_result read_audio_file(int32_t refnum, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
{
	ga_result result = GA_SUCCESS;

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

extern "C" LV_DLL_EXPORT ga_result write_audio_file(int32_t refnum, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written)
{
	ga_result result = GA_SUCCESS;

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

extern "C" LV_DLL_EXPORT ga_result close_audio_file(int32_t refnum)
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

extern "C" LV_DLL_EXPORT ga_result get_audio_file_tags(const char* file_name, int32_t* count, intptr_t* tags, intptr_t* lengths)
{
	ga_result result;
	ga_codec codec;

	result = get_audio_file_codec(file_name, &codec);

	if (result != GA_SUCCESS)
	{
		return result;
	}

	switch (codec)
	{
		case ga_codec_flac: return get_flac_tags(file_name, count, tags, lengths); break;
		case ga_codec_mp3: return get_id3_tags(file_name, count, tags, lengths); break;
		case ga_codec_vorbis: return get_vorbis_tags(file_name, count, tags, lengths); break;
		case ga_codec_wav: return get_wav_tags(file_name, count, tags, lengths); break;
		case ga_codec_unsupported: return GA_E_UNSUPPORTED_TAG; break;
		default: break;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result free_audio_file_tags(int32_t count, intptr_t tags, intptr_t lengths)
{
	char** tags_ptr = (char**)tags;
	int32_t* lengths_ptr = (int32_t*)lengths;

	for (int i = 0; i < count; i++)
	{
		free(tags_ptr[i]);
		tags_ptr[i] = NULL;
	}
	free(tags_ptr);
	tags_ptr = NULL;

	free(lengths_ptr);
	lengths_ptr = NULL;

	return GA_SUCCESS;
}

ga_result get_audio_file_codec(const char* file_name, ga_codec* codec)
{
	FILE* pFile;

	*codec = ga_codec_unsupported;

	pFile = ga_fopen(file_name);
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
		return GA_E_UNSUPPORTED_CODEC;
	}
	return GA_SUCCESS;
}

///////////////////////////
// FLAC decoding wrapper //
///////////////////////////

inline ga_result convert_flac_result(int32_t result)
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

ga_result get_flac_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

int16_t* load_flac(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_result* result)
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

ga_result open_flac_file(const char* file_name, void** decoder)
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

ga_result get_basic_flac_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
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

ga_result seek_flac_file(void* decoder, uint64_t offset, uint64_t* new_offset)
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

ga_result read_flac_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
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

ga_result close_flac_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	drflac_close((drflac*)decoder);

	return GA_SUCCESS;
}

void flac_metadata_callback(void* pUserData, drflac_metadata* meta)
{
	audio_file_tags* tag_info = (audio_file_tags*)pUserData;
	switch (meta->type)
	{
		case DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT:
		{
			drflac_vorbis_comment_iterator iterator;
			const char* comment;
			uint32_t length;
			int i = 0;

			tag_info->count = meta->data.vorbis_comment.commentCount;

			if (tag_info->count > 0)
			{
				tag_info->tags = (char**)malloc(sizeof(char*) * tag_info->count);
				tag_info->lengths = (int32_t*)malloc(sizeof(int32_t) * tag_info->count);

				drflac_init_vorbis_comment_iterator(&iterator, meta->data.vorbis_comment.commentCount, meta->data.vorbis_comment.pComments);
				while ((comment = drflac_next_vorbis_comment(&iterator, &length)) != NULL)
				{
					tag_info->tags[i] = (char*)malloc(sizeof(char) * length);
					tag_info->lengths[i] = length;
					memcpy(tag_info->tags[i], comment, length);
					i++;
				}
			}
			break;
		}
		default: break;
	}
}

ga_result get_flac_tags(const char* file_name, int32_t* count, intptr_t* tags, intptr_t* lengths)
{
	drflac* decoder;
	audio_file_tags tag_info = {};

#if defined(_WIN32)
	decoder = drflac_open_file_with_metadata_w(widen(file_name), flac_metadata_callback, &tag_info, NULL);
#else
	decoder = drflac_open_file_with_metadata(file_name, flac_metadata_callback, &tag_info, NULL);
#endif

	if (decoder == NULL)
	{
		return GA_E_TAG;
	}

	drflac_close(decoder);

	*count = tag_info.count;
	*tags = (intptr_t)tag_info.tags;
	*lengths = (intptr_t)tag_info.lengths;

	return GA_SUCCESS;
}


//////////////////////////
// MP3 decoding wrapper //
//////////////////////////

inline ga_result convert_mp3_result(int32_t result)
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

ga_result get_mp3_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	ga_result result = GA_SUCCESS;
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

int16_t* load_mp3(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_result* result)
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

ga_result open_mp3_file(const char* file_name, void** decoder)
{
	ga_result result = GA_SUCCESS;

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

ga_result get_basic_mp3_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
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

ga_result seek_mp3_file(void* decoder, uint64_t offset, uint64_t* new_offset)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}
	
	// MP3 decoder offset values are in terms of samples, and not frames like the other decoders or LabVIEW wrapper.
	// Multiply by channels to get actual number of samples to read.
	return convert_mp3_result(mp3dec_ex_seek((mp3dec_ex_t*)decoder, offset * ((mp3dec_ex_t*)decoder)->info.channels));
}

ga_result read_mp3_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
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

ga_result close_mp3_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	mp3dec_ex_close((mp3dec_ex_t*)decoder);
	free(decoder);

	return GA_SUCCESS;
}

id3tag_t* load_id3_tag(const char* file_name)
{
	FILE* pFile;
	ID3TAG_U8 tag_size_data[10];
	size_t tag_size;
	void* tag_data;
	id3tag_t* id3tag;

	pFile = ga_fopen(file_name);
	if (pFile == NULL)
	{
		return NULL;
	}

	fseek(pFile, 0, SEEK_SET);
	fread(&tag_size_data, 1, sizeof(tag_size_data), pFile);

	tag_size = id3tag_size(tag_size_data);
	// ID3v2 tag found
	if (tag_size > 0)
	{
		tag_data = malloc(tag_size);

		if (tag_data == NULL)
		{
			fclose(pFile);
			return NULL;
		}

		fseek(pFile, 0, SEEK_SET);
		fread(tag_data, 1, tag_size, pFile);
		fclose(pFile);
		id3tag = id3tag_load(tag_data, tag_size, ID3TAG_ALL_FIELDS, NULL);
		free(tag_data);
	}
	// No ID3V2 tag, try ID3v1
	else
	{
		tag_data = malloc(128);

		if (tag_data == NULL)
		{
			fclose(pFile);
			return NULL;
		}

		fseek(pFile, -128, SEEK_END);
		fread(tag_data, 1, 128, pFile);
		fclose(pFile);
		id3tag = id3tag_load_id3v1(tag_data, 128, NULL);
		free(tag_data);
	}

	return id3tag;
}

ga_result get_id3_tags(const char* file_name, int32_t* count, intptr_t* tags, intptr_t* lengths)
{
	enum fields_t
	{
		FIELD_TITLE,
		FIELD_ARTIST,
		FIELD_ALBUM_ARTIST,
		FIELD_ALBUM,
		FIELD_GENRE,
		FIELD_YEAR,
		FIELD_TRACK,
		FIELD_TRACKS,
		FIELD_DISC,
		FIELD_DISCS,
		FIELD_COMPILATION,
		FIELD_BPM,
		FIELD_COMMENT,
		FIELD_USER_TEXT,

		FIELDCOUNT
	};

	const char* tag_field_prefix[FIELDCOUNT] = { "TITLE=", "ARTIST=", "ALBUMARTIST=", "ALBUM=", "GENRE=", "DATE=", "TRACKNUMBER=", "TRACKTOTAL=", "DISCNUMBER=", "DISCTOTAL=", "COMPILATION=", "BPM=", "COMMENT=" };

	audio_file_tags tag_info = {};
	const char* current_tag = NULL;
	id3tag_t* id3tag;

	id3tag = load_id3_tag(file_name);

	if (id3tag == NULL)
	{
		return GA_E_TAG;
	}

	tag_info.count = 0;
	for (int field_index = 0; field_index < FIELDCOUNT; field_index++)
	{
		current_tag = NULL;
		switch (field_index)
		{
			case FIELD_TITLE:        if (id3tag->title != NULL) { current_tag = id3tag->title; } break;
			case FIELD_ARTIST:       if (id3tag->artist != NULL) { current_tag = id3tag->artist; } break;
			case FIELD_ALBUM_ARTIST: if (id3tag->album_artist != NULL) { current_tag = id3tag->album_artist; } break;
			case FIELD_ALBUM:        if (id3tag->album != NULL) { current_tag = id3tag->album; } break;
			case FIELD_GENRE:        if (id3tag->genre != NULL) { current_tag = id3tag->genre; } break;
			case FIELD_YEAR:         if (id3tag->year != NULL) { current_tag = id3tag->year; } break;
			case FIELD_TRACK:        if (id3tag->track != NULL) { current_tag = id3tag->track; } break;
			case FIELD_TRACKS:       if (id3tag->tracks != NULL) { current_tag = id3tag->tracks; } break;
			case FIELD_DISC:         if (id3tag->disc != NULL) { current_tag = id3tag->disc; } break;
			case FIELD_DISCS:        if (id3tag->discs != NULL) { current_tag = id3tag->discs; } break;
			case FIELD_COMPILATION:  if (id3tag->compilation != NULL) { current_tag = id3tag->compilation; } break;
			case FIELD_BPM:          if (id3tag->bpm != NULL) { current_tag = id3tag->bpm; } break;
			case FIELD_COMMENT:      if (id3tag->comment != NULL) { current_tag = id3tag->comment; } break;
			default:                 current_tag = NULL; break;
		}

		if (current_tag != NULL)
		{
			int tag_index = tag_info.count;
			tag_info.count++;
			tag_info.tags = (char**)realloc(tag_info.tags, sizeof(char*) * tag_info.count);
			tag_info.lengths = (int32_t*)realloc(tag_info.lengths, sizeof(int32_t) * tag_info.count);

			int field_length = strlen(tag_field_prefix[field_index]);
			int value_length = strlen(current_tag);
			tag_info.lengths[tag_index] = field_length + value_length;
			tag_info.tags[tag_index] = (char*)malloc(sizeof(char) * tag_info.lengths[tag_index]);
			memcpy(tag_info.tags[tag_index], tag_field_prefix[field_index], field_length);
			memcpy(tag_info.tags[tag_index] + field_length, current_tag, value_length);
		}

		if (field_index == FIELD_USER_TEXT && id3tag->user_text != NULL && id3tag->user_text_count > 0)
		{
			int tag_offset = tag_info.count;
			int tag_index = 0;
			tag_info.count += id3tag->user_text_count;
			tag_info.tags = (char**)realloc(tag_info.tags, sizeof(char*) * tag_info.count);
			tag_info.lengths = (int32_t*)realloc(tag_info.lengths, sizeof(int32_t) * tag_info.count);

			for (int user_text_index = 0; user_text_index < id3tag->user_text_count; user_text_index++)
			{
				int field_length = strlen(id3tag->user_text[user_text_index].desc);
				int value_length = strlen(id3tag->user_text[user_text_index].value);

				tag_index = tag_offset + user_text_index;
				tag_info.lengths[tag_index] = field_length + value_length + 1; // +1 is for the '=' separator
				tag_info.tags[tag_index] = (char*)malloc(sizeof(char) * tag_info.lengths[tag_index]);
				memcpy(tag_info.tags[tag_index], id3tag->user_text[user_text_index].desc, field_length);
				*(tag_info.tags[tag_index] + field_length) = '=';
				memcpy(tag_info.tags[tag_index] + field_length + 1, id3tag->user_text[user_text_index].value, value_length);
			}
		}
	}

	id3tag_free(id3tag);

	*count = tag_info.count;
	*tags = (intptr_t)tag_info.tags;
	*lengths = (intptr_t)tag_info.lengths;

	return GA_SUCCESS;
}


/////////////////////////////////
// Ogg Vorbis decoding wrapper //
/////////////////////////////////

inline ga_result convert_vorbis_result(int32_t result)
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

ga_result get_vorbis_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
{
	ga_result result = GA_SUCCESS;
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

int16_t* load_vorbis(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_result* result)
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

ga_result open_vorbis_file(const char* file_name, void** decoder)
{
	ga_result result = GA_SUCCESS;
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

ga_result get_basic_vorbis_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
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

ga_result seek_vorbis_file(void* decoder, uint64_t offset, uint64_t* new_offset)
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

ga_result read_vorbis_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
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

ga_result close_vorbis_file(void* decoder)
{
	if (decoder == NULL)
	{
		return GA_E_GENERIC;
	}

	stb_vorbis_close((stb_vorbis*)decoder);

	return GA_SUCCESS;
}

ga_result get_vorbis_tags(const char* file_name, int32_t* count, intptr_t* tags, intptr_t* lengths)
{
	ga_result result = GA_SUCCESS;
	int error = 0;
	stb_vorbis* vorbis_decoder;
	audio_file_tags tag_info = {};

#if defined(_WIN32)
	vorbis_decoder = stb_vorbis_open_filename_w(widen(file_name), &error, NULL);
#else
	vorbis_decoder = stb_vorbis_open_filename(file_name, &error, NULL);
#endif

	result = convert_vorbis_result(error);

	if (vorbis_decoder == NULL || result != GA_SUCCESS)
	{
		return GA_E_TAG;
	}

	tag_info.count = vorbis_decoder->comment_list_length;
	if (tag_info.count > 0)
	{
		tag_info.tags = (char**)malloc(sizeof(char*) * tag_info.count);
		tag_info.lengths = (int32_t*)malloc(sizeof(int32_t) * tag_info.count);

		for (int i = 0; i < vorbis_decoder->comment_list_length; i++)
		{
			tag_info.lengths[i] = strlen(vorbis_decoder->comment_list[i]);
			tag_info.tags[i] = (char*)malloc(sizeof(char) * tag_info.lengths[i]);
			memcpy(tag_info.tags[i], vorbis_decoder->comment_list[i], tag_info.lengths[i]);
		}
	}

	stb_vorbis_close(vorbis_decoder);

	*count = tag_info.count;
	*tags = (intptr_t)tag_info.tags;
	*lengths = (intptr_t)tag_info.lengths;

	return result;
}


//////////////////////////
// WAV decoding wrapper //
//////////////////////////

inline ga_result convert_wav_result(int32_t result)
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

ga_result get_wav_info(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, uint32_t* bits_per_sample)
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

int16_t* load_wav(const char* file_name, uint64_t* num_frames, uint32_t* channels, uint32_t* sample_rate, ga_result* result)
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

ga_result open_wav_file(const char* file_name, void** decoder)
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

ga_result open_wav_file_write(const char* file_name, uint32_t channels, uint32_t sample_rate, uint32_t bits_per_sample, void* codec_specific, void** encoder)
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

ga_result get_basic_wav_file_info(void* decoder, uint32_t* channels, uint32_t* sample_rate, uint64_t* read_offset)
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

ga_result seek_wav_file(void* decoder, uint64_t offset, uint64_t* new_offset)
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

ga_result read_wav_file(void* decoder, uint64_t frames_to_read, ga_data_type audio_type, uint64_t* frames_read, void* output_buffer)
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

ga_result write_wav_file(void* encoder, uint64_t frames_to_write, void* input_buffer, uint64_t* frames_written)
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

ga_result close_wav_file(void* decoder)
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

ga_result get_wav_tags(const char* file_name, int32_t* count, intptr_t* tags, intptr_t* lengths)
{
	enum fields_t
	{
		FIELD_TITLE,
		FIELD_ARTIST,
		FIELD_ALBUM,
		FIELD_GENRE,
		FIELD_YEAR,
		FIELD_TRACK,
		FIELD_COMMENT,

		FIELD_COUNT
	};

	const char* tag_field_prefix[FIELD_COUNT] = { "TITLE=", "ARTIST=", "ALBUM=", "GENRE=", "DATE=", "TRACKNUMBER=", "COMMENT=" };

	audio_file_tags tag_info = {};
	drwav_bool32 result = DRWAV_FALSE;
	drwav* wav_decoder = (drwav*)malloc(sizeof(drwav));
	int field_index = -1;

	if (wav_decoder == NULL)
	{
		return GA_E_MEMORY;
	}

#if defined(_WIN32)
	result = drwav_init_file_with_metadata_w(wav_decoder, widen(file_name), 0, NULL);
#else
	result = drwav_init_file_with_metadata(wav_decoder, file_name, NULL);
#endif

	if (result == DRWAV_FALSE)
	{
		free(wav_decoder);
		wav_decoder = NULL;
		return GA_E_GENERIC;
	}

	tag_info.count = 0;
	tag_info.tags = NULL;
	tag_info.lengths = NULL;
	for (int i = 0; i < wav_decoder->metadataCount; i++)
	{
		field_index = -1;
		switch (wav_decoder->pMetadata[i].type)
		{
			case drwav_metadata_type_list_info_title:       field_index = FIELD_TITLE; break;
			case drwav_metadata_type_list_info_artist:      field_index = FIELD_ARTIST; break;
			case drwav_metadata_type_list_info_album:       field_index = FIELD_ALBUM; break;
			case drwav_metadata_type_list_info_genre:       field_index = FIELD_GENRE; break;
			case drwav_metadata_type_list_info_date:        field_index = FIELD_YEAR; break;
			case drwav_metadata_type_list_info_tracknumber: field_index = FIELD_TRACK; break;
			case drwav_metadata_type_list_info_comment:     field_index = FIELD_COMMENT; break;
			default: field_index = -1; break;
		}

		if (field_index >= 0)
		{
			int tag_index = tag_info.count;
			tag_info.count++;
			tag_info.tags = (char**)realloc(tag_info.tags, sizeof(char*) * tag_info.count);
			tag_info.lengths = (int32_t*)realloc(tag_info.lengths, sizeof(int32_t) * tag_info.count);

			int field_length = strlen(tag_field_prefix[field_index]);
			int value_length = wav_decoder->pMetadata[i].data.infoText.stringLength;
			tag_info.lengths[tag_index] = field_length + value_length;
			tag_info.tags[tag_index] = (char*)malloc(sizeof(char) * tag_info.lengths[tag_index]);
			memcpy(tag_info.tags[tag_index], tag_field_prefix[field_index], field_length);
			memcpy(tag_info.tags[tag_index] + field_length, wav_decoder->pMetadata[i].data.infoText.pString, value_length);
		}
	}

	drwav_uninit(wav_decoder);
	free(wav_decoder);

	*count = tag_info.count;
	*tags = (intptr_t)tag_info.tags;
	*lengths = (intptr_t)tag_info.lengths;

	return GA_SUCCESS;
}




//////////////////////////////
// LabVIEW Audio Device API //
//////////////////////////////

extern "C" LV_DLL_EXPORT ga_result query_audio_backends(uint16_t* backends, uint16_t* num_backends)
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

extern "C" LV_DLL_EXPORT ga_result query_audio_devices(uint16_t* backend_in, uint8_t* playback_device_ids, int32_t* num_playback_devices, uint8_t* capture_device_ids, int32_t* num_capture_devices)
{
	ma_context context;
	ma_context_config context_config;
	ma_device_info* pPlaybackDeviceInfos;
	ma_uint32 playbackDeviceCount;
	ma_device_info* pCaptureDeviceInfos;
	ma_uint32 captureDeviceCount;
	ga_combined_result result;
	uint32_t iDevice;
	ma_backend backend = (ma_backend)*backend_in;

	context_config = ma_context_config_init();
	// TODO: Only set this flag on Raspberry Pi
	context_config.alsa.useVerboseDeviceEnumeration = MA_TRUE;

	// Thread safety - ma_context_init, ma_context_get_devices, ma_context_uninit are unsafe
	lock_ga_mutex(ga_mutex_context);
	// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
	// The backends enum in LabVIEW adds Default after Null
	result.ma = ma_context_init((*backend_in > ma_backend_null ? NULL : &backend), 1, &context_config, &context);
	if (result.ma != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return ga_return_code(result);
	}

	result.ma = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
	if (result.ma != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return ga_return_code(result);
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

	*backend_in = (uint16_t)context.backend;
	*num_playback_devices = playbackDeviceCount;
	*num_capture_devices = captureDeviceCount;

	ma_context_uninit(&context);

	unlock_ga_mutex(ga_mutex_context);

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result get_audio_device_info(uint16_t backend_in, const uint8_t* device_id, uint16_t device_type, char* device_name, uint32_t* device_default,
                                                         uint32_t* device_native_data_format_count, uint16_t* device_native_data_format, uint32_t* device_native_data_channels,
                                                         uint32_t* device_native_data_sample_rate, uint32_t* device_native_data_exclusive_mode)
{
	ma_context context;
	ma_context_config context_config;
	ma_device_id deviceId;
	ma_device_info deviceInfo;
	ga_combined_result result;
	ma_backend backend = (ma_backend)backend_in;

	if ((ma_device_type)device_type == ma_device_type_duplex)
	{
		return GA_E_UNSUPPORTED_DEVICE;
	}

	memcpy(&deviceId, device_id, sizeof(ma_device_id));

	context_config = ma_context_config_init();
	// TODO: Only set this flag on Raspberry Pi
	context_config.alsa.useVerboseDeviceEnumeration = MA_TRUE;

	// Thread safety - ma_context_init, ma_context_uninit are unsafe
	lock_ga_mutex(ga_mutex_context);
	// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
	// The backends enum in LabVIEW adds Default after Null
	result.ma = ma_context_init((backend_in > ma_backend_null ? NULL : &backend), 1, &context_config, &context);
	if (result.ma != MA_SUCCESS)
	{
		unlock_ga_mutex(ga_mutex_context);
		return ga_return_code(result);
	}

	result.ma = ma_context_get_device_info(&context, (ma_device_type)device_type, &deviceId, &deviceInfo);
	if (result.ma != MA_SUCCESS)
	{
		ma_context_uninit(&context);
		unlock_ga_mutex(ga_mutex_context);
		return ga_return_code(result);
	}

#if defined(_WIN32)
	strcpy_s(device_name, sizeof(deviceInfo.name), deviceInfo.name);
#else
	strncpy(device_name, deviceInfo.name, sizeof(deviceInfo.name));
	device_name[sizeof(deviceInfo.name)-1] = '\0';
#endif

	*device_default = deviceInfo.isDefault;
	*device_native_data_format_count = deviceInfo.nativeDataFormatCount;
	for (int i = 0; i < deviceInfo.nativeDataFormatCount; i++)
	{
		device_native_data_format[i]         = deviceInfo.nativeDataFormats[i].format;
		device_native_data_channels[i]       = deviceInfo.nativeDataFormats[i].channels;
		device_native_data_sample_rate[i]    = deviceInfo.nativeDataFormats[i].sampleRate;
		device_native_data_exclusive_mode[i] = deviceInfo.nativeDataFormats[i].flags & MA_DATA_FORMAT_FLAG_EXCLUSIVE_MODE;
	}

	ma_context_uninit(&context);

	unlock_ga_mutex(ga_mutex_context);

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result configure_audio_device(uint16_t backend_in, const uint8_t* device_id, uint16_t device_type, uint32_t channels, uint32_t sample_rate, uint16_t format, uint8_t exclusive_mode, uint32_t period_size, uint32_t num_periods, int32_t buffer_size, int32_t* refnum)
{
	ga_combined_result result;
	ma_device_id deviceId;
	ma_format device_format_init;
	ma_uint32 device_channels_init;
	ma_uint32 device_internal_buffer_size = 0;
	ma_backend backend = (ma_backend)backend_in;
	uint8_t blank_device_id[sizeof(ma_device_id)] = { 0 };
	audio_device* pDevice = NULL;

	if ((ma_device_type)device_type == ma_device_type_duplex)
	{
		return GA_E_UNSUPPORTED_DEVICE;
	}

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
		// TODO: Only set this flag on Raspberry Pi
		context_config.alsa.useVerboseDeviceEnumeration = MA_TRUE;

		// Can safely pass 1 as backendCount, as it's ignored when backends is NULL.
		// The backends enum in LabVIEW adds Default after Null
		result.ma = ma_context_init((backend_in > ma_backend_null ? NULL : &backend), 1, &context_config, global_context);
		if (result.ma != MA_SUCCESS)
		{
			free(global_context);
			global_context = NULL;
			unlock_ga_mutex(ga_mutex_context);
			return ga_return_code(result);
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
		device_config.periodSizeInFrames = period_size;
		device_config.periods = num_periods;
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
		device_config.periodSizeInFrames = period_size;
		device_config.periods = num_periods;
		break;
	}

	pDevice = (audio_device*)malloc(sizeof(audio_device));
	if (pDevice == NULL)
	{
		return GA_E_MEMORY;
	}

	result.ma = ma_device_init(global_context, &device_config, &pDevice->device);
	if (result.ma != MA_SUCCESS)
	{
		free(pDevice);
		pDevice = NULL;
		return ga_return_code(result);
	}

	pDevice->buffer_size = buffer_size;
	// Store the device ID used to configure this device. Will be zeroed, or device ID.
	pDevice->device_id = deviceId;

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

	result.ma = ma_pcm_rb_init(device_format_init, device_channels_init, buffer_size, NULL, NULL, &pDevice->buffer);
	if (result.ma != MA_SUCCESS)
	{
		ma_device_uninit(&pDevice->device);
		free(pDevice);
		pDevice = NULL;
		return ga_return_code(result);
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

extern "C" LV_DLL_EXPORT ga_result get_configured_backend(uint16_t* backend)
{
	if (global_context == NULL)
	{
		*backend = ma_backend_null;
	}
	else
	{
		*backend = global_context->backend;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result get_configured_audio_devices(int32_t* playback_refnums, int32_t* num_playback_refnums, int32_t* capture_refnums, int32_t* num_capture_refnums, int32_t* loopback_refnums, int32_t* num_loopback_refnums)
{
	audio_device* pDevice;
	std::vector<int32_t> refnums = get_all_references(ga_refnum_audio_device);

	for (int i = 0; i < refnums.size(); i++)
	{
		pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnums[i]);

		if (pDevice != NULL)
		{
			switch (pDevice->device.type)
			{
				case ma_device_type_playback:
					playback_refnums[*num_playback_refnums] = refnums[i];
					(*num_playback_refnums)++;
					break;
				case ma_device_type_capture:
					capture_refnums[*num_capture_refnums] = refnums[i];
					(*num_capture_refnums)++;
					break;
				case ma_device_type_loopback:
					loopback_refnums[*num_loopback_refnums] = refnums[i];
					(*num_loopback_refnums)++;
					break;
				default:
					break;
			}
		}
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result get_configured_audio_device_info(int32_t refnum, uint8_t* config_device_id, uint8_t* actual_device_id)
{
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	// There are two device IDs returned.
	// config_device_id is the device ID specified at configuration time (all zeros if default).
	// actual_device_id is the device ID after configuration. miniaudio states this should be the same as the configured id, but does differ if it was default and WASAPI is the backend.

	memcpy(config_device_id, (void*)&pDevice->device_id, sizeof(ma_device_id));

	switch (pDevice->device.type)
	{
	case ma_device_type_playback:
		memcpy(actual_device_id, (void*)&pDevice->device.playback.id, sizeof(ma_device_id));
		break;
	case ma_device_type_capture:
	case ma_device_type_loopback:
		memcpy(actual_device_id, (void*)&pDevice->device.capture.id, sizeof(ma_device_id));
		break;
	default:
		return GA_E_UNSUPPORTED_DEVICE;
		break;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result get_audio_device_configuration(int32_t refnum, uint32_t* sample_rate, uint32_t* channels, uint16_t* format, uint8_t* exclusive_mode, uint32_t* period_size, uint32_t* num_periods)
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
		*period_size = pDevice->device.capture.internalPeriodSizeInFrames;
		*num_periods = pDevice->device.capture.internalPeriods;
		break;
	default:
		*format = pDevice->device.playback.format;
		*channels = pDevice->device.playback.channels;
		*sample_rate = pDevice->device.sampleRate;
		*exclusive_mode = pDevice->device.playback.shareMode;
		*period_size = pDevice->device.playback.internalPeriodSizeInFrames;
		*num_periods = pDevice->device.playback.internalPeriods;
		break;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result get_audio_device_volume(int32_t refnum, float* volume)
{
	ga_combined_result result;
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	result.ma = ma_device_get_master_volume(&pDevice->device, volume);
	if (result.ma != MA_SUCCESS)
	{
		return ga_return_code(result);
	}
    
	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result set_audio_device_volume(int32_t refnum, float volume)
{
	ga_combined_result result;
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	result.ma = ma_device_set_master_volume(&pDevice->device, volume);
	if (result.ma != MA_SUCCESS)
	{
		return ga_return_code(result);
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result start_audio_device(int32_t refnum)
{
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	return check_and_start_audio_device(&pDevice->device);
}

extern "C" LV_DLL_EXPORT ga_result playback_audio(int32_t refnum, void* buffer, int32_t num_frames, uint32_t channels, ga_data_type audio_type)
{
	ga_combined_result result;
	ma_uint32 framesToWrite = num_frames;
	ma_uint32 pcmFramesProcessed = 0;
	ma_uint32 bufferOffset = 0;
	ma_uint32 bytesToWrite;
	void* pWriteBuffer;
	int i = 0;
	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);
	void* output_buffer = buffer;
	ma_bool32 passthrough = true;

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

	if (!((audio_type == ga_data_type_u8 && pDevice->device.playback.format == ma_format_u8) ||
		(audio_type == ga_data_type_i16 && pDevice->device.playback.format == ma_format_s16) ||
		(audio_type == ga_data_type_i32 && pDevice->device.playback.format == ma_format_s32) ||
		(audio_type == ga_data_type_float && pDevice->device.playback.format == ma_format_f32)))
	{
		void* conversion_buffer = NULL;
		uint64_t num_channel_samples = num_frames * channels;

		switch (audio_type)
		{
			case ga_data_type_u8:
				conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
				ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, output_buffer, ma_format_u8, num_channel_samples, ma_dither_mode_none);
				break;
			case ga_data_type_i16:
				conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
				ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, output_buffer, ma_format_s16, num_channel_samples, ma_dither_mode_none);
				break;
			case ga_data_type_i32:
				conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
				ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, output_buffer, ma_format_s32, num_channel_samples, ma_dither_mode_none);
				break;
			case ga_data_type_float:
				conversion_buffer = malloc(num_channel_samples * ma_get_bytes_per_sample(pDevice->device.playback.format));
				ma_pcm_convert(conversion_buffer, pDevice->device.playback.format, output_buffer, ma_format_f32, num_channel_samples, ma_dither_mode_none);
				break;
			case ga_data_type_double:
				switch (pDevice->device.playback.format)
				{
				case ma_format_u8:
					conversion_buffer = malloc(num_channel_samples * sizeof(uint8_t));
					f64_to_u8((uint8_t*)conversion_buffer, (double*)output_buffer, num_channel_samples);
					break;
				case ma_format_s16:
					conversion_buffer = malloc(num_channel_samples * sizeof(int16_t));
					f64_to_s16((int16_t*)conversion_buffer, (double*)output_buffer, num_channel_samples);
					break;
				case ma_format_s32:
					conversion_buffer = malloc(num_channel_samples * sizeof(int32_t));
					f64_to_s32((int32_t*)conversion_buffer, (double*)output_buffer, num_channel_samples);
					break;
				case ma_format_f32:
					conversion_buffer = malloc(num_channel_samples * sizeof(float));
					f64_to_f32((float*)conversion_buffer, (double*)output_buffer, num_channel_samples);
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

		output_buffer = conversion_buffer;
		conversion_buffer = NULL;
		passthrough = MA_FALSE;
	}

	if (pDevice->device.playback.channels != channels)
	{
		void* channel_conversion_buffer = NULL;
		ma_channel_converter converter;
		ma_channel_converter_config converter_config = ma_channel_converter_config_init(pDevice->device.playback.format, channels, NULL, pDevice->device.playback.channels, NULL, ma_channel_mix_mode_default);

		result.ma = ma_channel_converter_init(&converter_config, NULL, &converter);
		if (result.ma != MA_SUCCESS)
		{
			if (!passthrough)
			{
			    free(output_buffer);
				output_buffer = NULL;
			}
			return ga_return_code(result);
		}

		channel_conversion_buffer = malloc(num_frames * ma_get_bytes_per_frame(pDevice->device.playback.format, pDevice->device.playback.channels));
		if (channel_conversion_buffer == NULL)
		{
			ma_channel_converter_uninit(&converter, NULL);
			if (!passthrough)
			{
				free(output_buffer);
				output_buffer = NULL;
			}
			return GA_E_MEMORY;
		}

		result.ma = ma_channel_converter_process_pcm_frames(&converter, channel_conversion_buffer, output_buffer, num_frames);
		if (result.ma != MA_SUCCESS)
		{
			ma_channel_converter_uninit(&converter, NULL);
			free(channel_conversion_buffer);
			channel_conversion_buffer = NULL;
			if (!passthrough)
			{
				free(output_buffer);
				output_buffer = NULL;
			}
			return ga_return_code(result);
		}

		ma_channel_converter_uninit(&converter, NULL);
		if (!passthrough)
		{
			free(output_buffer);
		}
		output_buffer = channel_conversion_buffer;
		channel_conversion_buffer = NULL;
		passthrough = MA_FALSE;
	}

	result.ga = check_and_start_audio_device(&pDevice->device);
	if (result.ga != GA_SUCCESS)
	{
		return ga_return_code(result);
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

		result.ma = ma_pcm_rb_acquire_write(&pDevice->buffer, &framesToWrite, &pWriteBuffer);
		if (result.ma != MA_SUCCESS)
		{
			return ga_return_code(result);
		}
		{
			bytesToWrite = framesToWrite * ma_get_bytes_per_frame(pDevice->device.playback.format, pDevice->device.playback.channels);
			memcpy(pWriteBuffer, (ma_uint8*)output_buffer + bufferOffset, bytesToWrite);
		}
		result.ma = ma_pcm_rb_commit_write(&pDevice->buffer, framesToWrite);
		if (!((result.ma == MA_SUCCESS) || (result.ma == MA_AT_END)))
		{
			return ga_return_code(result);
		}
		pcmFramesProcessed += framesToWrite;
		bufferOffset += bytesToWrite;
	} while (pcmFramesProcessed < num_frames);

	if (!passthrough)
	{
		free(output_buffer);
		output_buffer = NULL;
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result capture_audio(int32_t refnum, void* buffer, int32_t* num_frames, ga_data_type audio_type)
{
	ga_combined_result result;
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

	result.ga = check_and_start_audio_device(&pDevice->device);
	if (result.ga != GA_SUCCESS)
	{
		return ga_return_code(result);
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

		result.ma = ma_pcm_rb_acquire_read(&pDevice->buffer, &framesToRead, &pReadBuffer);
		if (result.ma != MA_SUCCESS)
		{
			return ga_return_code(result);
		}
		{
			bytesToRead = framesToRead * ma_get_bytes_per_frame(pDevice->device.capture.format, pDevice->device.capture.channels);
			memcpy((ma_uint8*)conversion_buffer + bufferOffset, pReadBuffer, bytesToRead);
		}
		result.ma = ma_pcm_rb_commit_read(&pDevice->buffer, framesToRead);
		if (!((result.ma == MA_SUCCESS) || (result.ma == MA_AT_END)))
		{
			return ga_return_code(result);
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

extern "C" LV_DLL_EXPORT ga_result playback_wait(int32_t refnum)
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

extern "C" LV_DLL_EXPORT ga_result stop_audio_device(int32_t refnum)
{
	ga_combined_result result;

	audio_device* pDevice = (audio_device*)get_reference_data(ga_refnum_audio_device, refnum);

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	switch (ma_device_get_state(&pDevice->device))
	{
		case ma_device_state_stopping:
		case ma_device_state_stopped:
			return GA_SUCCESS;
			break;
		default:
			break;
	}

	result.ma = ma_device_stop(&pDevice->device);
	if (result.ma != MA_SUCCESS)
	{
		return ga_return_code(result);
	}

	return GA_SUCCESS;
}

extern "C" LV_DLL_EXPORT ga_result clear_audio_device(int32_t refnum)
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

extern "C" LV_DLL_EXPORT ga_result clear_audio_backend()
{
	ga_combined_result result;

	std::vector<int32_t> refnums = get_all_references(ga_refnum_audio_device);

	for (int i = 0; i < refnums.size(); i++)
	{
		clear_audio_device(refnums[i]);
	}

	lock_ga_mutex(ga_mutex_context);
	if (global_context != NULL)
	{
		result.ma = ma_context_uninit(global_context);
		free(global_context);
		global_context = NULL;
	}
	unlock_ga_mutex(ga_mutex_context);

	if (result.ma != MA_SUCCESS)
	{
		return ga_return_code(result);
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
			ma_pcm_rb_commit_read((ma_pcm_rb*)pDevice->pUserData, framesToRead);

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
			ma_pcm_rb_commit_write((ma_pcm_rb*)pDevice->pUserData, framesToWrite);

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
	ma_device_state state;
	state = ma_device_get_state(pDevice);
	return (state == ma_device_state_started) || (state == ma_device_state_starting);
}

inline ga_result check_and_start_audio_device(ma_device* pDevice)
{
	ga_combined_result result;

	if (pDevice == NULL)
	{
		return GA_E_REFNUM;
	}

	if (!device_is_started(pDevice))
	{
		result.ma = ma_device_start(pDevice);
		if (result.ma != MA_SUCCESS)
		{
			return ga_return_code(result);
		}
	}

	return GA_SUCCESS;
}


////////////////////////////
// LabVIEW Audio Data API //
////////////////////////////
extern "C" LV_DLL_EXPORT ga_result channel_converter(ga_data_type audio_type, uint64_t num_frames, void* audio_buffer_in, uint32_t channels_in, void* audio_buffer_out, uint32_t channels_out)
{
	ga_combined_result result;
	ma_channel_converter converter;
	ma_channel_converter_config converter_config = ma_channel_converter_config_init(ga_data_type_to_ma_format(audio_type), channels_in, NULL, channels_out, NULL, ma_channel_mix_mode_default);

	result.ma = ma_channel_converter_init(&converter_config, NULL, &converter);
	if (result.ma != MA_SUCCESS)
	{
		return ga_return_code(result);
	}

	result.ma = ma_channel_converter_process_pcm_frames(&converter, audio_buffer_out, audio_buffer_in, num_frames);
	if (result.ma != MA_SUCCESS)
	{
		ma_channel_converter_uninit(&converter, NULL);
		return ga_return_code(result);
	}

	ma_channel_converter_uninit(&converter, NULL);

	return GA_SUCCESS;
}

inline ma_format ga_data_type_to_ma_format(ga_data_type audio_type)
{
	switch (audio_type)
	{
		case ga_data_type_u8: return ma_format_u8; break;
		case ga_data_type_i16: return ma_format_s16; break;
		case ga_data_type_i32: return ma_format_s32; break;
		case ga_data_type_float: return ma_format_f32; break;
		case ga_data_type_double: return ma_format_f32; break;
		default: return ma_format_s16; break;
	}

	return ma_format_s16;
}
