// alsa-mic-quickstart-cpp — C++17 live-mic wake-word demo on top of
// the voxrt-wake-word C ABI. Companion to the C example that ships
// in the SDK tarball; same runtime, same API, idiomatic C++ shape.

#include <alsa/asoundlib.h>

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <voxrt_wake_word.h>

namespace {

constexpr unsigned SAMPLE_RATE  = 16000;
constexpr size_t   CHUNK_FRAMES = 512;
constexpr size_t   DET_BUF_CAP  = 8;

std::atomic<bool> g_stop{false};
extern "C" void on_sigint(int) { g_stop.store(true); }

class VoxrtWakeWord {
public:
    explicit VoxrtWakeWord(const std::vector<std::uint8_t>& model_bytes) {
        auto rc = voxrt_wake_word_create(model_bytes.data(), model_bytes.size(), &h_);
        if (rc != VOXRT_OK)
            throw std::runtime_error("voxrt_wake_word_create failed: " + std::to_string(rc));
    }
    ~VoxrtWakeWord() { if (h_) voxrt_wake_word_destroy(h_); }
    VoxrtWakeWord(const VoxrtWakeWord&) = delete;
    VoxrtWakeWord& operator=(const VoxrtWakeWord&) = delete;
    VoxrtWakeWord(VoxrtWakeWord&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
    VoxrtWakeWord& operator=(VoxrtWakeWord&& o) noexcept {
        if (this != &o) { if (h_) voxrt_wake_word_destroy(h_); h_ = std::exchange(o.h_, nullptr); }
        return *this;
    }

    void set_threshold(float t) { voxrt_wake_word_set_threshold(h_, t); }
    void set_cooldown(size_t f) { voxrt_wake_word_set_cooldown_frames(h_, f); }

    std::vector<voxrt_wake_word_detection_t>
    push_pcm(const std::int16_t* pcm, std::size_t len) {
        std::vector<voxrt_wake_word_detection_t> out(DET_BUF_CAP);
        std::size_t written = 0;
        auto rc = voxrt_wake_word_push_pcm_i16(h_, pcm, len, out.data(), out.size(), &written);
        if (rc != VOXRT_OK && rc != VOXRT_ERR_BUFFER_TOO_SMALL)
            throw std::runtime_error("push_pcm_i16: " + std::to_string(rc));
        out.resize(std::min(written, out.size()));
        return out;
    }

private:
    voxrt_wake_word_t* h_ = nullptr;
};

class AlsaCapture {
public:
    explicit AlsaCapture(const std::string& device) {
        if (snd_pcm_open(&pcm_, device.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0)
            throw std::runtime_error("snd_pcm_open failed: " + device);

        snd_pcm_hw_params_t* hwp = nullptr;
        snd_pcm_hw_params_alloca(&hwp);
        snd_pcm_hw_params_any(pcm_, hwp);
        snd_pcm_hw_params_set_access(pcm_, hwp, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_, hwp, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm_, hwp, 1);
        unsigned rate = SAMPLE_RATE;
        int dir = 0;
        snd_pcm_hw_params_set_rate_near(pcm_, hwp, &rate, &dir);
        snd_pcm_uframes_t period = CHUNK_FRAMES;
        snd_pcm_hw_params_set_period_size_near(pcm_, hwp, &period, &dir);
        if (snd_pcm_hw_params(pcm_, hwp) < 0)
            throw std::runtime_error("snd_pcm_hw_params commit failed");
        if (snd_pcm_start(pcm_) < 0)
            throw std::runtime_error("snd_pcm_start failed");

        std::fprintf(stderr,
                     "[init] alsa device=%s rate=%u channels=1 format=S16_LE period=%lu\n",
                     device.c_str(), rate, static_cast<unsigned long>(period));
    }
    ~AlsaCapture() { if (pcm_) snd_pcm_close(pcm_); }
    AlsaCapture(const AlsaCapture&) = delete;
    AlsaCapture& operator=(const AlsaCapture&) = delete;

    snd_pcm_sframes_t read(std::int16_t* buf, std::size_t frames) {
        auto n = snd_pcm_readi(pcm_, buf, frames);
        if (n < 0) {
            if (snd_pcm_prepare(pcm_) < 0)
                throw std::runtime_error("unrecoverable ALSA xrun");
            std::fprintf(stderr, "[warn] alsa xrun, recovered\n");
            return 0;
        }
        return n;
    }

private:
    snd_pcm_t* pcm_ = nullptr;
};

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    return {std::istreambuf_iterator<char>(in), {}};
}

} // namespace

int main(int argc, char** argv) try {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <model.vxrt> [<alsa-device>]\n", argv[0]);
        return 1;
    }
    const std::string model_path = argv[1];
    const std::string device     = (argc >= 3) ? argv[2] : "plughw:0";

    std::signal(SIGINT, on_sigint);

    const auto model_bytes = read_file(model_path);
    VoxrtWakeWord ww(model_bytes);
    ww.set_threshold(0.9f);
    ww.set_cooldown(100);
    std::fprintf(stderr, "[init] model=%s sdk=%s\n", model_path.c_str(),
                 voxrt_wake_word_version());
    std::fprintf(stderr, "[init] threshold=0.90 cooldown_frames=100\n");

    AlsaCapture cap(device);
    std::fprintf(stderr, "[init] listening — Ctrl-C to stop\n");

    std::vector<std::int16_t> buf(CHUNK_FRAMES);
    while (!g_stop.load()) {
        auto n = cap.read(buf.data(), CHUNK_FRAMES);
        if (n <= 0) continue;
        for (const auto& d : ww.push_pcm(buf.data(), static_cast<std::size_t>(n))) {
            std::printf("[wake] frame=%llu t=%.3fs score=%.4f\n",
                        static_cast<unsigned long long>(d.frame_index),
                        d.timestamp_sec, d.score);
            std::fflush(stdout);
        }
    }
    std::fprintf(stderr, "[stop] shutting down\n");
    return 0;
}
catch (const std::exception& e) {
    std::fprintf(stderr, "[fatal] %s\n", e.what());
    return 1;
}
