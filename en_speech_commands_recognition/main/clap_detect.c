#include "clap_detect.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// Tuning — adjust these if claps are missed or background noise false-triggers
// ---------------------------------------------------------------------------
// Frame energy must exceed background × this ratio to count as a clap
#define ENERGY_RATIO_NUM    3
#define ENERGY_RATIO_DEN    2
// Background energy smoothing (lower = slower adaptation, more stable)
#define BG_ALPHA            0.01f
// Floor prevents divide-by-zero and false triggers in near-silence
#define BG_FLOOR            1500000
// Gap between the two claps (ms)
#define GAP_MIN_MS          100
#define GAP_MAX_MS          800
// Ignore audio for this long after a successful detection
#define COOLDOWN_MS         2000

typedef enum {
    ST_IDLE,
    ST_CLAP1,
    ST_GAP,
} state_t;

static state_t state;
static int rate_hz;
static int64_t bg_energy;
static int gap_samples;
static int cooldown_samples;

void clap_detect_init(int sample_rate)
{
    state = ST_IDLE;
    rate_hz = sample_rate;
    bg_energy = BG_FLOOR;
    gap_samples = 0;
    cooldown_samples = 0;
}

static int64_t frame_energy(const int16_t *samples, int n, int ch)
{
    int64_t sum = 0;
    for (int i = 0; i < n * ch; i += ch) {
        int64_t s = samples[i];
        sum += s * s;
    }
    return sum / n;
}

static void update_bg(int64_t energy)
{
    bg_energy = (int64_t)((1.0f - BG_ALPHA) * bg_energy + BG_ALPHA * energy);
    if (bg_energy < BG_FLOOR)
        bg_energy = BG_FLOOR;
}

bool clap_detect_feed(const int16_t *samples, int num_samples, int channels)
{
    int64_t energy = frame_energy(samples, num_samples, channels);

    if (cooldown_samples > 0) {
        cooldown_samples -= num_samples;
        if (cooldown_samples < 0)
            cooldown_samples = 0;
        update_bg(energy);
        return false;
    }

    bool is_loud = (energy * ENERGY_RATIO_DEN) > (bg_energy * ENERGY_RATIO_NUM);

    int min_gap = (GAP_MIN_MS * rate_hz) / 1000;
    int max_gap = (GAP_MAX_MS * rate_hz) / 1000;

    switch (state) {
    case ST_IDLE:
        if (is_loud) {
            state = ST_CLAP1;
        } else {
            update_bg(energy);
        }
        break;

    case ST_CLAP1:
        if (!is_loud) {
            state = ST_GAP;
            gap_samples = 0;
        }
        break;

    case ST_GAP:
        gap_samples += num_samples;
        if (is_loud && gap_samples >= min_gap) {
            printf("DOUBLE CLAP (bg=%lld, energy=%lld)\n", bg_energy, energy);
            state = ST_IDLE;
            cooldown_samples = (COOLDOWN_MS * rate_hz) / 1000;
            return true;
        }
        if (gap_samples > max_gap) {
            state = ST_IDLE;
        }
        if (!is_loud) {
            update_bg(energy);
        }
        break;
    }

    return false;
}
