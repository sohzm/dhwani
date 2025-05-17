#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define SAMPLE_RATE        48000
#define CHANNELS           2
#define FORMAT             ma_format_s16

/* Since we know FORMAT = s16, bytes per sample = 2. */
#define BYTES_PER_SAMPLE   2

/* 0.1 seconds worth of frames */
#define FRAME_COUNT        (SAMPLE_RATE/10)

/* Total bytes per chunk (compile‐time constant!) */
#define CHUNK_BYTES        (FRAME_COUNT * CHANNELS * BYTES_PER_SAMPLE)

typedef struct {
    uint8_t pcm[CHUNK_BYTES];
    bool    ready;
} CaptureBuffer;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    (void)pOutput;  /* Always NULL in loopback capture */

    CaptureBuffer* buf = (CaptureBuffer*)pDevice->pUserData;
    size_t bytesToCopy = frameCount * CHANNELS * BYTES_PER_SAMPLE;
    if (bytesToCopy > CHUNK_BYTES) {
        bytesToCopy = CHUNK_BYTES;  /* just in case */
    }

    memcpy(buf->pcm, pInput, bytesToCopy);
    buf->ready = true;
}

int main(void)
{
    ma_result          result;
    ma_device          device;
    ma_device_config   config;
    CaptureBuffer*     buf;

  #ifdef _WIN32
    /* On Windows, make stdout binary */
    _setmode(_fileno(stdout), _O_BINARY);
  #endif

    /* Allocate our buffer+flag */
    buf = (CaptureBuffer*)malloc(sizeof(*buf));
    if (!buf) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    buf->ready = false;

    /* Initialize capture device for loopback */
    config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format   = FORMAT;
    config.capture.channels = CHANNELS;
    config.sampleRate       = SAMPLE_RATE;
    config.dataCallback     = data_callback;
    config.pUserData        = buf;

  #ifdef _WIN32
    /* On Windows you must use WASAPI for loopback */
    {
        ma_backend backends[] = { ma_backend_wasapi };
        result = ma_device_init_ex(backends, 1, NULL, &config, &device);
    }
  #else
    /* On Linux/macOS we just let miniaudio pick the default backend */
    result = ma_device_init(NULL, &config, &device);
  #endif

    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize loopback device (%d)\n", result);
        free(buf);
        return 1;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to start device (%d)\n", result);
        ma_device_uninit(&device);
        free(buf);
        return 1;
    }

    fprintf(stderr, "Capturing raw PCM, 100 ms chunks. Press Ctrl+C to stop.\n");

    /* Main loop: wait for each chunk, write it to stdout */
    while (1) {
        while (!buf->ready) {
            ma_sleep(1);
        }
        fwrite(buf->pcm, 1, CHUNK_BYTES, stdout);
        fflush(stdout);
        buf->ready = false;
    }

    /* (Unreachable in this example) */
    ma_device_uninit(&device);
    free(buf);
    return 0;
}
