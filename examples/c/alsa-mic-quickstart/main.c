/*
 * VoxRT wake-word — ALSA live-mic quickstart.
 *
 * Opens an ALSA capture device at 16 kHz mono S16_LE, pushes every
 * 512-sample chunk into voxrt_wake_word_session_push_pcm_i16(), and
 * prints any detection that crosses the threshold.
 *
 * Build:
 *   gcc main.c $(pkg-config --cflags --libs voxrt-wake-word) -lasound -o alsa-mic-quickstart
 *
 * Run (assuming the .vxrt sits next to the binary):
 *   ./alsa-mic-quickstart voxrt_wake_word.vxrt [<alsa-device>]
 *
 * Defaults: alsa-device = "plughw:0", threshold = 0.9, cooldown = 100.
 *
 * This is the C equivalent of the Rust `score_alsa` example shipped
 * with the wake-word crate — same algorithm, same output schema. The
 * one-line difference: C uses libasound directly; Rust wraps it
 * through the `alsa` crate.
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <voxrt_wake_word.h>

#define SAMPLE_RATE 16000
#define CHUNK_FRAMES 512
#define DET_BUF_CAP 8

static int read_model_mmap(const char *path, const uint8_t **out_bytes, size_t *out_len) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "[error] cannot open model %s: %s\n", path, strerror(errno));
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size <= 0) {
        fprintf(stderr, "[error] cannot stat model %s\n", path);
        close(fd);
        return -1;
    }
    void *map = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map == MAP_FAILED) {
        fprintf(stderr, "[error] mmap failed: %s\n", strerror(errno));
        return -1;
    }
    *out_bytes = (const uint8_t *)map;
    *out_len = (size_t)st.st_size;
    return 0;
}

static int open_alsa_capture(const char *device, snd_pcm_t **out_pcm) {
    snd_pcm_t *pcm = NULL;
    if (snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        fprintf(stderr, "[error] alsa: cannot open %s\n", device);
        return -1;
    }
    snd_pcm_hw_params_t *hwp = NULL;
    snd_pcm_hw_params_alloca(&hwp);
    snd_pcm_hw_params_any(pcm, hwp);
    snd_pcm_hw_params_set_access(pcm, hwp, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hwp, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hwp, 1);
    unsigned int rate = SAMPLE_RATE;
    int dir = 0;
    snd_pcm_hw_params_set_rate_near(pcm, hwp, &rate, &dir);
    snd_pcm_uframes_t period = CHUNK_FRAMES;
    snd_pcm_hw_params_set_period_size_near(pcm, hwp, &period, &dir);
    if (snd_pcm_hw_params(pcm, hwp) < 0) {
        fprintf(stderr, "[error] alsa: cannot commit hwparams\n");
        snd_pcm_close(pcm);
        return -1;
    }
    if (snd_pcm_start(pcm) < 0) {
        fprintf(stderr, "[error] alsa: cannot start capture\n");
        snd_pcm_close(pcm);
        return -1;
    }
    fprintf(stderr, "[init] alsa device=%s rate=%u channels=1 format=S16_LE period=%lu\n",
            device, rate, (unsigned long)period);
    *out_pcm = pcm;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <model.vxrt> [<alsa-device>]\n", argv[0]);
        fprintf(stderr, "       <model.vxrt>   path to a wake-word .vxrt file\n");
        fprintf(stderr, "       <alsa-device>  ALSA PCM name, default \"plughw:0\"\n");
        return 1;
    }
    const char *model_path = argv[1];
    const char *device = (argc >= 3) ? argv[2] : "plughw:0";

    const uint8_t *model_bytes = NULL;
    size_t model_len = 0;
    if (read_model_mmap(model_path, &model_bytes, &model_len) != 0) return 2;

    voxrt_wake_word_t *engine = NULL;
    voxrt_status_t rc = voxrt_wake_word_create(model_bytes, model_len, &engine);
    if (rc != VOXRT_OK) {
        fprintf(stderr, "[error] voxrt_wake_word_create failed: %d\n", rc);
        munmap((void *)model_bytes, model_len);
        return 3;
    }
    voxrt_wake_word_set_threshold(engine, 0.9f);
    voxrt_wake_word_set_cooldown_frames(engine, 100);
    fprintf(stderr, "[init] model=%s sdk=%s\n", model_path, voxrt_wake_word_version());
    fprintf(stderr, "[init] threshold=0.90 cooldown_frames=100\n");

    snd_pcm_t *pcm = NULL;
    if (open_alsa_capture(device, &pcm) != 0) {
        voxrt_wake_word_destroy(engine);
        munmap((void *)model_bytes, model_len);
        return 4;
    }
    fprintf(stderr, "[init] listening — Ctrl-C to stop\n");

    int16_t buf[CHUNK_FRAMES];
    voxrt_wake_word_detection_t dets[DET_BUF_CAP];
    for (;;) {
        snd_pcm_sframes_t n = snd_pcm_readi(pcm, buf, CHUNK_FRAMES);
        if (n < 0) {
            if (snd_pcm_prepare(pcm) < 0) {
                fprintf(stderr, "[error] alsa xrun unrecoverable\n");
                break;
            }
            fprintf(stderr, "[warn] alsa xrun, recovered\n");
            continue;
        }
        size_t written = 0;
        rc = voxrt_wake_word_push_pcm_i16(engine, buf, (size_t)n,
                                          dets, DET_BUF_CAP, &written);
        if (rc != VOXRT_OK && rc != VOXRT_ERR_BUFFER_TOO_SMALL) {
            fprintf(stderr, "[error] push_pcm_i16: %d\n", rc);
            break;
        }
        size_t emitted = (written <= DET_BUF_CAP) ? written : DET_BUF_CAP;
        for (size_t i = 0; i < emitted; ++i) {
            printf("[wake] frame=%llu t=%.3fs score=%.4f\n",
                   (unsigned long long)dets[i].frame_index,
                   dets[i].timestamp_sec, dets[i].score);
            fflush(stdout);
        }
    }

    snd_pcm_close(pcm);
    voxrt_wake_word_destroy(engine);
    munmap((void *)model_bytes, model_len);
    return 0;
}
