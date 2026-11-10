#pragma once
#include <stdbool.h>
#include <stdint.h>

void clap_detect_init(int sample_rate);

// Feed interleaved PCM16 audio. Returns true when a double-clap is detected.
bool clap_detect_feed(const int16_t *samples, int num_samples, int channels);
