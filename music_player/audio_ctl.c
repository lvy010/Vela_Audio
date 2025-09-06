/*********************
 *      INCLUDES
 *********************/

// MP3调试支持 - 启用详细日志
#ifndef MP3_DEBUG
#define MP3_DEBUG 1  // 启用调试
#endif

#if MP3_DEBUG
#define MP3_LOG(fmt, ...) printf("[MP3 DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define MP3_LOG(fmt, ...)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "audio_ctl.h"

#include <audioutils/nxaudio.h>

// 检查MP3支持和MAD库是否可用
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
#include <string.h>
#ifdef CONFIG_LIB_MAD
#include <mad.h>
#define MP3_DECODER_AVAILABLE 1
#else
#define MP3_DECODER_AVAILABLE 0
#warning "MAD library not available, MP3 support disabled"
#endif
#else
#define MP3_DECODER_AVAILABLE 0
#endif

// MP3解码缓冲区大小
#define MP3_BUFFER_SIZE 8192
#define PCM_BUFFER_SIZE 4096

#if MP3_DECODER_AVAILABLE
// MP3解码状态结构
typedef struct {
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    unsigned char input_buffer[MP3_BUFFER_SIZE];
    short output_buffer[PCM_BUFFER_SIZE];
    int input_length;
    int output_length;
    bool initialized;
} mp3_decoder_t;

static mp3_decoder_t g_mp3_decoder = {0};
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void app_dequeue_cb(unsigned long arg,
                           FAR struct ap_buffer_s *apb);
static void app_complete_cb(unsigned long arg);
static void app_user_cb(unsigned long arg,
                        FAR struct audio_msg_s *msg, FAR bool *running);

#if MP3_DECODER_AVAILABLE
static int mp3_decoder_init(mp3_decoder_t *decoder);
static void mp3_decoder_cleanup(mp3_decoder_t *decoder);
static int mp3_decode_frame(mp3_decoder_t *decoder, const unsigned char *data, int len);
static short mp3_scale_sample(mad_fixed_t sample);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

static struct nxaudio_callbacks_s cbs =
{
  app_dequeue_cb,
  app_complete_cb,
  app_user_cb
};

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void app_dequeue_cb(unsigned long arg, FAR struct ap_buffer_s *apb)
{
    FAR audioctl_s *ctl = (FAR audioctl_s *)(uintptr_t)arg;

    if (!apb)
    {
        return;
    }

    if (ctl->seek) {
        lseek(ctl->fd, ctl->seek_position, SEEK_SET);
        ctl->file_position = ctl->seek_position;
        ctl->seek = false;
    }

    apb->curbyte = 0;
    apb->flags = 0;

    if (ctl->audio_format == AUDIO_FORMAT_WAV) {
        /* Handle WAV format (original logic) */
        apb->nbytes = read(ctl->fd, apb->samp, apb->nmaxbytes);

        while (0 < apb->nbytes && apb->nbytes < apb->nmaxbytes)
        {
            int n = apb->nmaxbytes - apb->nbytes;
            int ret = read(ctl->fd, &apb->samp[apb->nbytes], n);

            if (0 >= ret)
            {
                break;
            }
            apb->nbytes += ret;
        }

        if (apb->nbytes < apb->nmaxbytes)
        {
            close(ctl->fd);
            ctl->fd = -1;
            return;
        }

        ctl->file_position += apb->nbytes;
    }
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
    else if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        /* Handle MP3 format */
        static unsigned char mp3_input_buffer[CONFIG_LVX_MUSIC_PLAYER_MP3_BUFFER_SIZE];
        size_t bytes_read = read(ctl->fd, mp3_input_buffer, sizeof(mp3_input_buffer));
        
        MP3_LOG("📖 读取MP3数据: %zu bytes (位置: %ld)", bytes_read, ctl->file_position);
        
        if (bytes_read <= 0) {
            MP3_LOG("📄 MP3文件读取完成或出错，关闭文件");
            close(ctl->fd);
            ctl->fd = -1;
            apb->nbytes = 0;
            return;
        }

        MP3_LOG("🔄 开始MP3解码: 输入%zu bytes, 输出缓冲区%zu bytes", bytes_read, apb->nmaxbytes);
        
        // 使用内部解码器进行MP3解码
        int decoded_samples = mp3_decode_frame(&g_mp3_decoder, mp3_input_buffer, bytes_read);
        
        if (decoded_samples > 0) {
            // 复制解码后的PCM数据到音频缓冲区
            size_t bytes_to_copy = decoded_samples * sizeof(short);
            if (bytes_to_copy > apb->nmaxbytes) {
                bytes_to_copy = apb->nmaxbytes;
            }
            
            memcpy(apb->samp, g_mp3_decoder.output_buffer, bytes_to_copy);
            apb->nbytes = bytes_to_copy;
            MP3_LOG("✅ MP3解码成功: 输出%zu bytes (%d samples)", bytes_to_copy, decoded_samples);
        } else {
            apb->nbytes = 0;
            MP3_LOG("❌ MP3解码失败或无输出");
        }

        if (apb->nbytes == 0) {
            MP3_LOG("⚠️ 解码输出为空，关闭文件");
            close(ctl->fd);
            ctl->fd = -1;
            return;
        }

        ctl->file_position += bytes_read;
    }
#endif
    else {
        /* Unknown format */
        apb->nbytes = 0;
        return;
    }

    nxaudio_enqbuffer(&ctl->nxaudio, apb);
}

static void app_complete_cb(unsigned long arg)
{
    /* Do nothing.. */

    printf("Audio loop is Done\n");
}

static void app_user_cb(unsigned long arg,
                        FAR struct audio_msg_s *msg, FAR bool *running)
{
    /* Do nothing.. */
}

static FAR void *audio_loop_thread(pthread_addr_t arg)
{
    FAR audioctl_s *ctl = (FAR audioctl_s *)arg;

    nxaudio_start(&ctl->nxaudio);
    nxaudio_msgloop(&ctl->nxaudio, &cbs,
                    (unsigned long)(uintptr_t)ctl);

    return NULL;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

FAR audioctl_s *audio_ctl_init_nxaudio(FAR const char *arg)
{
    FAR audioctl_s *ctl;
    int ret;
    int i;

    ctl = (FAR audioctl_s *)malloc(sizeof(audioctl_s));
    if(ctl == NULL)
    {
        return NULL;
    }

    memset(ctl, 0, sizeof(audioctl_s));
    ctl->seek = false;
    ctl->seek_position = 0;
    ctl->file_position = 0;

    /* Detect audio format */
    ctl->audio_format = audio_ctl_detect_format(arg);
    MP3_LOG("🎵 检测音频格式: %s -> %d", arg, ctl->audio_format);
    
    if (ctl->audio_format == AUDIO_FORMAT_UNKNOWN) {
        MP3_LOG("❌ 不支持的音频格式: %s", arg);
        printf("Unsupported audio format: %s\n", arg);
        free(ctl);
        return NULL;
    } else if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        MP3_LOG("✅ 检测到MP3格式，准备初始化解码器");
    }

    ctl->fd = open(arg, O_RDONLY);
    if (ctl->fd < 0) {
       MP3_LOG("❌ 无法打开音频文件: %s (errno: %d)", arg, errno);
       printf("can't open audio file: %s\n", arg);
       free(ctl);
       return NULL;
    }
    
    // 获取文件大小
    struct stat st;
    if (fstat(ctl->fd, &st) == 0) {
        MP3_LOG("✅ 音频文件打开成功: %s (大小: %lld bytes)", arg, (long long)st.st_size);
    }

    uint32_t sample_rate = 44100;
    uint16_t bits_per_sample = 16;
    uint16_t channels = 2;

    if (ctl->audio_format == AUDIO_FORMAT_WAV) {
        /* Read WAV header */
        read(ctl->fd, &ctl->wav, sizeof(ctl->wav));
        sample_rate = ctl->wav.fmt.samplerate;
        bits_per_sample = ctl->wav.fmt.bitspersample;
        channels = ctl->wav.fmt.numchannels;
    }
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
    else if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        /* Initialize global MP3 decoder */
        MP3_LOG("🔧 初始化全局MP3解码器...");
        if (mp3_decoder_init(&g_mp3_decoder) < 0) {
            MP3_LOG("❌ MP3解码器初始化失败");
            printf("Failed to initialize MP3 decoder\n");
            close(ctl->fd);
            free(ctl);
            return NULL;
        }
        MP3_LOG("✅ MP3解码器初始化成功");
        
        /* Use default values for MP3, will be updated during decode */
        sample_rate = 44100;
        bits_per_sample = 16;
        channels = 2;
        MP3_LOG("🎵 MP3默认参数: %dHz, %d位, %d声道", sample_rate, bits_per_sample, channels);
    }
#endif

    ret = init_nxaudio(&ctl->nxaudio, sample_rate, bits_per_sample, channels);
    if (ret < 0)
    {
        printf("init_nxaudio() return with error!!\n");
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
        if (ctl->audio_format == AUDIO_FORMAT_MP3) {
            mp3_decoder_cleanup(&g_mp3_decoder);
        }
#endif
        close(ctl->fd);
        free(ctl);
        return NULL;
    }

    for (i = 0; i < ctl->nxaudio.abufnum; i++)
    {
        app_dequeue_cb((unsigned long)ctl, ctl->nxaudio.abufs[i]);
    }

    ctl->state = AUDIO_CTL_STATE_INIT;

    return ctl;
}

int audio_ctl_start(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    if (ctl->state != AUDIO_CTL_STATE_INIT && ctl->state != AUDIO_CTL_STATE_PAUSE)
    {
        return -1;
    }

    ctl->state = AUDIO_CTL_STATE_START;

    pthread_attr_t tattr;
    struct sched_param sparam;

    pthread_attr_init(&tattr);
    sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
    pthread_attr_setschedparam(&tattr, &sparam);
    pthread_attr_setstacksize(&tattr, 4096);

    pthread_create(&ctl->pid, &tattr, audio_loop_thread,
                                (pthread_addr_t)ctl);

    pthread_attr_destroy(&tattr);
    pthread_setname_np(ctl->pid, "audioctl_thread");

    return 0;
}

int audio_ctl_pause(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    if (ctl->state != AUDIO_CTL_STATE_START)
    {
        return -1;
    }

    ctl->state = AUDIO_CTL_STATE_PAUSE;

    return nxaudio_pause(&ctl->nxaudio);
}

int audio_ctl_resume(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    if (ctl->state != AUDIO_CTL_STATE_PAUSE)
    {
        return -1;
    }

    ctl->state = AUDIO_CTL_STATE_START;

    return nxaudio_resume(&ctl->nxaudio);
}

int audio_ctl_seek(FAR audioctl_s *ctl, unsigned ms)
{
    if (ctl == NULL)
        return -EINVAL;

    ctl->seek_position = ms * ctl->wav.fmt.samplerate * ctl->wav.fmt.bitspersample * ctl->wav.fmt.numchannels / 8;
    ctl->seek = true;

    return 0;
}

int audio_ctl_stop(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    if (ctl->state != AUDIO_CTL_STATE_PAUSE && ctl->state != AUDIO_CTL_STATE_START)
    {
        return -1;
    }

    ctl->state = AUDIO_CTL_STATE_STOP;

    nxaudio_stop(&ctl->nxaudio);

    if (ctl->pid > 0)
    {
        pthread_join(ctl->pid, NULL);
    }

    return 0;
}

int audio_ctl_set_volume(FAR audioctl_s *ctl, uint16_t vol)
{
    if (ctl == NULL)
        return -EINVAL;

    return nxaudio_setvolume(&ctl->nxaudio, vol);
}

int audio_ctl_get_position(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    return ctl->file_position / (ctl->wav.fmt.bitspersample * ctl->wav.fmt.numchannels * ctl->wav.fmt.samplerate / 8);
}

int audio_ctl_uninit_nxaudio(FAR audioctl_s *ctl)
{
    if (ctl == NULL)
        return -EINVAL;

    if (ctl->state == AUDIO_CTL_STATE_NOP)
    {
        return 0;
    }

#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
    /* Cleanup MP3 decoder if used */
    if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        mp3_decoder_cleanup(&ctl->mp3);
    }
#endif

    if (ctl->fd > 0)
    {
        close(ctl->fd);
        ctl->fd = -1;
    }

    fin_nxaudio(&ctl->nxaudio);

    free(ctl);

    return 0;
}

/**********************
 *   MP3 FUNCTIONS
 **********************/

/**
 * @brief Detect audio format based on file extension
 * @param filename Audio file path
 * @return AUDIO_FORMAT_WAV, AUDIO_FORMAT_MP3, or AUDIO_FORMAT_UNKNOWN
 */
int audio_ctl_detect_format(FAR const char *filename)
{
    if (!filename) {
        return AUDIO_FORMAT_UNKNOWN;
    }
    
    size_t len = strlen(filename);
    if (len < 4) {
        return AUDIO_FORMAT_UNKNOWN;
    }
    
    const char *ext = filename + len - 4;
    
    if (strncasecmp(ext, ".wav", 4) == 0) {
        return AUDIO_FORMAT_WAV;
    }
#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT
    else if (strncasecmp(ext, ".mp3", 4) == 0) {
        return AUDIO_FORMAT_MP3;
    }
#endif
    
    return AUDIO_FORMAT_UNKNOWN;
}

#ifdef CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT

/**
 * @brief Initialize MP3 decoder
 * @param decoder MP3 decoder structure
 * @return 0 on success, -1 on error
 */
int mp3_decoder_init(FAR mp3_decoder_s *decoder)
{
    if (!decoder) {
        return -1;
    }
    
    memset(decoder, 0, sizeof(mp3_decoder_s));
    
    /* Initialize libmad structures */
    mad_stream_init(&decoder->stream);
    mad_frame_init(&decoder->frame);
    mad_synth_init(&decoder->synth);
    
    /* Allocate buffers */
    decoder->buffer_size = CONFIG_LVX_MUSIC_PLAYER_MP3_BUFFER_SIZE;
    decoder->input_buffer = malloc(decoder->buffer_size);
    decoder->output_buffer = malloc(decoder->buffer_size * 4); /* PCM output is larger */
    
    if (!decoder->input_buffer || !decoder->output_buffer) {
        mp3_decoder_cleanup(decoder);
        return -1;
    }
    
    decoder->bytes_remaining = 0;
    decoder->output_size = 0;
    decoder->sample_rate = 44100; /* Default */
    decoder->channels = 2;       /* Default stereo */
    decoder->bits_per_sample = 16; /* Default 16-bit */
    
    return 0;
}

/**
 * @brief Convert MP3 sample to 16-bit PCM
 * @param sample MAD fixed-point sample
 * @return 16-bit PCM sample
 */
static inline int16_t mp3_sample_to_pcm(mad_fixed_t sample)
{
    /* MAD produces samples in 24.8 fixed point format */
    /* Round and clip to 16-bit */
    sample += (1L << (MAD_F_FRACBITS - 16));
    
    if (sample >= MAD_F_ONE) {
        sample = MAD_F_ONE - 1;
    } else if (sample < -MAD_F_ONE) {
        sample = -MAD_F_ONE;
    }
    
    return (int16_t)(sample >> (MAD_F_FRACBITS + 1 - 16));
}

/**
 * @brief Decode MP3 data to PCM
 * @param decoder MP3 decoder structure  
 * @param input Input MP3 data
 * @param input_size Size of input data
 * @param output Output PCM buffer
 * @param output_size Size of output data (in/out parameter)
 * @return 0 on success, -1 on error
 */
int mp3_decoder_decode(FAR mp3_decoder_s *decoder, FAR const unsigned char *input,
                       size_t input_size, FAR unsigned char *output, FAR size_t *output_size)
{
    if (!decoder || !input || !output || !output_size) {
        return -1;
    }
    
    int16_t *pcm_output = (int16_t *)output;
    size_t pcm_samples = 0;
    size_t max_samples = *output_size / sizeof(int16_t);
    
    /* Copy input data to decoder buffer */
    if (input_size > decoder->buffer_size) {
        input_size = decoder->buffer_size;
    }
    
    memcpy(decoder->input_buffer, input, input_size);
    mad_stream_buffer(&decoder->stream, decoder->input_buffer, input_size);
    
    while (pcm_samples < max_samples) {
        /* Decode frame */
        if (mad_frame_decode(&decoder->frame, &decoder->stream) == -1) {
            if (MAD_RECOVERABLE(decoder->stream.error)) {
                continue;
            } else {
                break;
            }
        }
        
        /* Update audio format info from first frame */
        if (decoder->sample_rate == 44100) { /* First frame */
            decoder->sample_rate = decoder->frame.header.samplerate;
            decoder->channels = MAD_NCHANNELS(&decoder->frame.header);
        }
        
        /* Synthesize PCM data */
        mad_synth_frame(&decoder->synth, &decoder->frame);
        
        /* Convert to 16-bit PCM */
        for (int i = 0; i < decoder->synth.pcm.length; i++) {
            if (pcm_samples >= max_samples) {
                break;
            }
            
            /* Left channel */
            pcm_output[pcm_samples++] = mp3_sample_to_pcm(decoder->synth.pcm.samples[0][i]);
            
            /* Right channel (if stereo) */
            if (decoder->channels == 2 && pcm_samples < max_samples) {
                pcm_output[pcm_samples++] = mp3_sample_to_pcm(decoder->synth.pcm.samples[1][i]);
            }
        }
    }
    
    *output_size = pcm_samples * sizeof(int16_t);
    return 0;
}

/**
 * @brief Cleanup MP3 decoder
 * @param decoder MP3 decoder structure
 */
void mp3_decoder_cleanup(FAR mp3_decoder_s *decoder)
{
    if (!decoder) {
        return;
    }
    
    mad_synth_finish(&decoder->synth);
    mad_frame_finish(&decoder->frame);
    mad_stream_finish(&decoder->stream);
    
    if (decoder->input_buffer) {
        free(decoder->input_buffer);
        decoder->input_buffer = NULL;
    }
    
    if (decoder->output_buffer) {
        free(decoder->output_buffer);
        decoder->output_buffer = NULL;
    }
    
    memset(decoder, 0, sizeof(mp3_decoder_s));
}

#if MP3_DECODER_AVAILABLE
/**
 * @brief 初始化MP3解码器
 */
static int mp3_decoder_init(mp3_decoder_t *decoder)
{
    if (!decoder || decoder->initialized) {
        return -1;
    }
    
    memset(decoder, 0, sizeof(mp3_decoder_t));
    
    mad_stream_init(&decoder->stream);
    mad_frame_init(&decoder->frame);
    mad_synth_init(&decoder->synth);
    
    decoder->initialized = true;
    MP3_LOG("MP3解码器初始化成功");
    return 0;
}

/**
 * @brief 清理MP3解码器
 */
static void mp3_decoder_cleanup(mp3_decoder_t *decoder)
{
    if (!decoder || !decoder->initialized) {
        return;
    }
    
    mad_synth_finish(&decoder->synth);
    mad_frame_finish(&decoder->frame);
    mad_stream_finish(&decoder->stream);
    
    decoder->initialized = false;
    MP3_LOG("MP3解码器清理完成");
}

/**
 * @brief 缩放MAD采样值到16位PCM
 */
static short mp3_scale_sample(mad_fixed_t sample)
{
    // MAD固定点格式转换为16位PCM
    if (sample >= MAD_F_ONE) {
        return 32767;
    } else if (sample <= -MAD_F_ONE) {
        return -32768;
    } else {
        return (short)(sample >> (MAD_F_FRACBITS - 15));
    }
}

/**
 * @brief 解码MP3帧数据
 */
static int mp3_decode_frame(mp3_decoder_t *decoder, const unsigned char *data, int len)
{
    if (!decoder || !decoder->initialized || !data || len <= 0) {
        return -1;
    }
    
    // 设置输入数据
    mad_stream_buffer(&decoder->stream, data, len);
    
    decoder->output_length = 0;
    
    while (decoder->stream.bufend - decoder->stream.next_frame > 0) {
        // 解码帧
        if (mad_frame_decode(&decoder->frame, &decoder->stream) != 0) {
            if (MAD_RECOVERABLE(decoder->stream.error)) {
                MP3_LOG("可恢复的MP3解码错误: %s", mad_stream_errorstr(&decoder->stream));
                continue;
            } else {
                MP3_LOG("不可恢复的MP3解码错误: %s", mad_stream_errorstr(&decoder->stream));
                break;
            }
        }
        
        // 合成PCM数据
        mad_synth_frame(&decoder->synth, &decoder->frame);
        
        // 转换为16位PCM
        for (int i = 0; i < decoder->synth.pcm.length && decoder->output_length < PCM_BUFFER_SIZE - 2; i++) {
            // 左声道
            decoder->output_buffer[decoder->output_length++] = 
                mp3_scale_sample(decoder->synth.pcm.samples[0][i]);
            
            // 右声道（如果是立体声）
            if (decoder->synth.pcm.channels == 2) {
                decoder->output_buffer[decoder->output_length++] = 
                    mp3_scale_sample(decoder->synth.pcm.samples[1][i]);
            } else {
                // 单声道复制到右声道
                decoder->output_buffer[decoder->output_length++] = 
                    decoder->output_buffer[decoder->output_length - 1];
            }
        }
    }
    
    MP3_LOG("解码完成，输出PCM样本数: %d", decoder->output_length);
    return decoder->output_length;
}
#endif /* MP3_DECODER_AVAILABLE */

#endif /* CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT */
