#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h> //is a buffer
}

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    AVAudioFifo *fifo = reinterpret_cast<AVAudioFifo *>(pDevice->pUserData);
    av_audio_fifo_read(fifo, &pOutput, frameCount);

    (void)pInput;
}

int main()
{
    // 1. open audio file
    AVFormatContext *format_ctx{nullptr};

    // open the file and save content to format_ctx
    int audioFileOpen = avformat_open_input(&format_ctx, "../resource/sad-soul-chasing-a-feeling-185750.mp3", nullptr, nullptr);
    if (audioFileOpen < 0)
    {
        fprintf(stderr, "unable to open media...");
        return -1;
    }

    // find if there is streams
    int streamOpen = avformat_find_stream_info(format_ctx, nullptr);
    if (streamOpen < 0)
    {
        fprintf(stderr, "unable to find stream in media...");
        return -1;
    }

    fprintf(stderr, "number of streams inside of media: %d", format_ctx->nb_streams);

    // find the audio stream
    int indexOfStream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, -1);
    if (indexOfStream < 0)
    {
        fprintf(stderr, "No audio stream in media...");
        return -1;
    }

    // the audio stream
    AVStream *stream = format_ctx->streams[indexOfStream];

    // get the acrroding decorder
    const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder)
    {
        fprintf(stderr, "No decoder found");
        return -1;
    }

    // a place to put content of decoded contents
    AVCodecContext *codec_ctx{avcodec_alloc_context3(decoder)};

    avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    int openDecoder = avcodec_open2(codec_ctx, decoder, nullptr); // open the decoder or otherwise unable to use the decoder
    if (openDecoder < 0)
    {
        fprintf(stderr, "Decoder failed to open...");
        return -1;
    }

    // 2. decode the audio
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    SwrContext *resampler{swr_alloc_set_opts(nullptr, stream->codecpar->channel_layout, AV_SAMPLE_FMT_FLT, stream->codecpar->sample_rate, stream->codecpar->channel_layout, (AVSampleFormat)stream->codecpar->format, stream->codecpar->sample_rate, 0, nullptr)};
    // set up the whole resampler, but seems the resampler don't do much change so lots of parameter in and out are the same, but holy cow this is huge chunk of code

    AVAudioFifo *fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, stream->codecpar->channels, 1); // it will automatic grow, so 1 is fine

    while (av_read_frame(format_ctx, packet) == 0)
    { // while runing normally return 0 so continou the loop
        if (packet->stream_index != stream->index)
        {
            continue; // to filter out the frames other than the selected audio one
            // but is this a good way of doing it?
        }
        int resFromDecoder = avcodec_send_packet(codec_ctx, packet); // send this packet to the opened decoder in codec_ctx
        if (resFromDecoder < 0)
        {
            if (resFromDecoder != AVERROR(EAGAIN))
            {
                fprintf(stderr, "decoder encountered unexpected error");
            }
        }
        while (resFromDecoder = avcodec_receive_frame(codec_ctx, frame) == 0)
        { // retrive all reponse frames
            // resample the frame
            AVFrame *resampled_frame = av_frame_alloc();
            resampled_frame->sample_rate = frame->sample_rate;
            resampled_frame->channel_layout = frame->channel_layout;
            resampled_frame->channels = frame->channels;
            resampled_frame->format = AV_SAMPLE_FMT_FLT;

            int resFromResampler = swr_convert_frame(resampler, resampled_frame, frame);
            av_frame_unref(frame); // free memory from decoder
            av_audio_fifo_write(fifo, (void **)resampled_frame->data, resampled_frame->nb_samples);
            av_frame_free(&resampled_frame);
            // that is pre c++11 style hell of freeing stuff, it is what it is, maybe safer ways to do it
        }
    }

    // 3. play back audio
    ma_device_config deviceConfig;
    ma_device device;
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = stream->codecpar->channels;
    deviceConfig.sampleRate = stream->codecpar->sample_rate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = fifo; // the decoded data

    // free all the unneeded memoriess
    avformat_close_input(&format_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    swr_free(&resampler);

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        printf("Failed to open playback device.\n");
        return -3;
    }

    if (ma_device_start(&device) != MA_SUCCESS)
    {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -4;
    }

    getchar(); // wait for user input to end

    av_audio_fifo_free(fifo);
    ma_device_uninit(&device);

    return 0;
}