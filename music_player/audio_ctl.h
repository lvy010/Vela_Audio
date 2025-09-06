#ifndef AUDIO_CTL_H
#define AUDIO_CTL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <audioutils/nxaudio.h>
#include <pthread.h>

#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
#include <mad.h>
#endif

enum {
    AUDIO_CTL_STATE_NOP,
    AUDIO_CTL_STATE_INIT,
    AUDIO_CTL_STATE_START,
    AUDIO_CTL_STATE_PAUSE,
    AUDIO_CTL_STATE_STOP,
};

enum {
    AUDIO_FORMAT_WAV,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_UNKNOWN,
};

typedef struct wav_riff {
    /* chunk "riff" */
    char chunkID[4];   /* "RIFF" */
    /* sub-chunk-size */
    uint32_t chunksize; /* 36 + subchunk2size */
    /* sub-chunk-data */
    char format[4];    /* "WAVE" */
} riff_s;

typedef struct wav_fmt {
    /* sub-chunk "fmt" */
    char subchunk1ID[4];   /* "fmt " */
    /* sub-chunk-size */
    uint32_t subchunk1size; /* 16 for PCM */
    /* sub-chunk-data */
    uint16_t audioformat;   /* PCM = 1*/
    uint16_t numchannels;   /* Mono = 1, Stereo = 2, etc. */
    uint32_t samplerate;    /* 8000, 44100, etc. */
    uint32_t byterate;  /* = samplerate * numchannels * bitspersample/8 */
    uint16_t blockalign;    /* = numchannels * bitspersample/8 */
    uint16_t bitspersample; /* 8bits, 16bits, etc. */
} fmt_s;

typedef struct wav_data {
    /* sub-chunk "data" */
    char subchunk2ID[4];   /* "data" */
    /* sub-chunk-size */
    uint32_t subchunk2size; /* data size */
    /* sub-chunk-data */
    //Data_block_t block;
} data_s;

typedef struct wav_fotmat {
   riff_s riff;
   fmt_s fmt;
   data_s data;
} wav_s;

#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
typedef struct mp3_decoder {
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    unsigned char *input_buffer;
    unsigned char *output_buffer;
    size_t buffer_size;
    size_t bytes_remaining;
    size_t output_size;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
} mp3_decoder_s;
#endif

typedef struct audioctl {
    struct nxaudio_s nxaudio;
    wav_s wav;
    int fd;
    int state;
    pthread_t pid;
    int seek;
    uint32_t seek_position;
    uint32_t file_position;
    int audio_format;  /* AUDIO_FORMAT_WAV or AUDIO_FORMAT_MP3 */
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
    mp3_decoder_s mp3;
#endif
} audioctl_s;

FAR audioctl_s *audio_ctl_init_nxaudio(FAR const char *arg);
int audio_ctl_start(FAR audioctl_s *ctl);
int audio_ctl_pause(FAR audioctl_s *ctl);
int audio_ctl_resume(FAR audioctl_s *ctl);
int audio_ctl_seek(FAR audioctl_s *ctl, unsigned ms);
int audio_ctl_stop(FAR audioctl_s *ctl);
int audio_ctl_set_volume(FAR audioctl_s *ctl, uint16_t vol);
int audio_ctl_get_position(FAR audioctl_s *ctl);
int audio_ctl_uninit_nxaudio(FAR audioctl_s *ctl);

/* Audio format detection */
int audio_ctl_detect_format(FAR const char *filename);

#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
/* MP3 specific functions */
int mp3_decoder_init(FAR mp3_decoder_s *decoder);
int mp3_decoder_decode(FAR mp3_decoder_s *decoder, FAR const unsigned char *input, 
                       size_t input_size, FAR unsigned char *output, FAR size_t *output_size);
void mp3_decoder_cleanup(FAR mp3_decoder_s *decoder);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* AUDIO_CTL_H */
