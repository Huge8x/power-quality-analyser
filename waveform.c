/*
 * waveform.c
 * Power-quality analysis functions.
 * All maths uses double precision throughout.
 */

#include "waveform.h"
#include <math.h>
#include <stdio.h>

/* ---------------------------------------------------------------
 * get_phase_ptr
 * Returns a pointer to the voltage field for the requested phase
 * inside a single WaveformSample.  Using pointer arithmetic on
 * the struct avoids duplicating every loop body three times.
 * phase: 0 = A, 1 = B, 2 = C
 * --------------------------------------------------------------- */
const double *get_phase_ptr(const WaveformSample *sample, int phase)
{
    switch (phase) {
        case 0:  return &sample->phase_A_voltage;
        case 1:  return &sample->phase_B_voltage;
        default: return &sample->phase_C_voltage;
    }
}

/* ---------------------------------------------------------------
 * compute_rms
 * RMS = sqrt( (1/N) * sum( v[i]^2 ) )
 * --------------------------------------------------------------- */
double compute_rms(const WaveformSample *samples, int n, int phase)
{
    double sum_sq = 0.0;
    const WaveformSample *p = samples;          /* pointer walk */

    for (int i = 0; i < n; i++, p++) {
        double v = *get_phase_ptr(p, phase);
        sum_sq += v * v;
    }
    return sqrt(sum_sq / (double)n);
}

/* ---------------------------------------------------------------
 * compute_peak_to_peak
 * V_pp = V_max - V_min across all samples.
 * --------------------------------------------------------------- */
double compute_peak_to_peak(const WaveformSample *samples, int n, int phase)
{
    const WaveformSample *p = samples;
    double vmax = *get_phase_ptr(p, phase);
    double vmin = vmax;

    for (int i = 1; i < n; i++) {
        p++;
        double v = *get_phase_ptr(p, phase);
        if (v > vmax) vmax = v;
        if (v < vmin) vmin = v;
    }
    return vmax - vmin;
}

/* ---------------------------------------------------------------
 * compute_dc_offset
 * DC offset = arithmetic mean of all samples.
 * For a clean AC signal this should be ~0 V.
 * --------------------------------------------------------------- */
double compute_dc_offset(const WaveformSample *samples, int n, int phase)
{
    double sum = 0.0;
    const WaveformSample *p = samples;

    for (int i = 0; i < n; i++, p++) {
        sum += *get_phase_ptr(p, phase);
    }
    return sum / (double)n;
}

/* ---------------------------------------------------------------
 * compute_std_dev
 * σ = sqrt( (1/N) * sum( (v[i] - mean)^2 ) )
 * The caller supplies the mean (dc_offset) so we avoid a second pass.
 * --------------------------------------------------------------- */
double compute_std_dev(const WaveformSample *samples, int n, int phase,
                       double dc_offset)
{
    double sum_sq_diff = 0.0;
    const WaveformSample *p = samples;

    for (int i = 0; i < n; i++, p++) {
        double diff = *get_phase_ptr(p, phase) - dc_offset;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / (double)n);
}

/* ---------------------------------------------------------------
 * count_clipped
 * Returns the number of samples where |voltage| >= CLIP_THRESHOLD.
 * --------------------------------------------------------------- */
int count_clipped(const WaveformSample *samples, int n, int phase)
{
    int count = 0;
    const WaveformSample *p = samples;

    for (int i = 0; i < n; i++, p++) {
        double v = *get_phase_ptr(p, phase);
        if (v >= CLIP_THRESHOLD || v <= -CLIP_THRESHOLD) {
            count++;
        }
    }
    return count;
}

/* ---------------------------------------------------------------
 * check_compliance
 * Returns 1 if rms is within the EN 50160 band [207, 253] V.
 * --------------------------------------------------------------- */
int check_compliance(double rms)
{
    return (rms >= TOLERANCE_LOW && rms <= TOLERANCE_HIGH) ? 1 : 0;
}

/* ---------------------------------------------------------------
 * analyse_frequency
 * Walks the frequency column to find mean, min, and max.
 * --------------------------------------------------------------- */
void analyse_frequency(const WaveformSample *samples, int n,
                       double *mean_out, double *min_out, double *max_out)
{
    double sum  = 0.0;
    double fmin = samples->frequency;
    double fmax = fmin;
    const WaveformSample *p = samples;

    for (int i = 0; i < n; i++, p++) {
        double f = p->frequency;
        sum += f;
        if (f < fmin) fmin = f;
        if (f > fmax) fmax = f;
    }
    *mean_out = sum / (double)n;
    *min_out  = fmin;
    *max_out  = fmax;
}

/* ---------------------------------------------------------------
 * mean_power_factor
 * --------------------------------------------------------------- */
double mean_power_factor(const WaveformSample *samples, int n)
{
    double sum = 0.0;
    const WaveformSample *p = samples;
    for (int i = 0; i < n; i++, p++) sum += p->power_factor;
    return sum / (double)n;
}

/* ---------------------------------------------------------------
 * mean_thd
 * --------------------------------------------------------------- */
double mean_thd(const WaveformSample *samples, int n)
{
    double sum = 0.0;
    const WaveformSample *p = samples;
    for (int i = 0; i < n; i++, p++) sum += p->thd_percent;
    return sum / (double)n;
}

/* ---------------------------------------------------------------
 * analyse_phase
 * Convenience wrapper: computes all metrics for one phase and
 * packages them in a PhaseResult struct, including bitmask flags.
 * --------------------------------------------------------------- */
PhaseResult analyse_phase(const WaveformSample *samples, int n, int phase)
{
    PhaseResult r;
    r.label         = (char)('A' + phase);
    r.rms           = compute_rms(samples, n, phase);
    r.peak_to_peak  = compute_peak_to_peak(samples, n, phase);
    r.dc_offset     = compute_dc_offset(samples, n, phase);
    r.std_dev       = compute_std_dev(samples, n, phase, r.dc_offset);
    r.variance      = r.std_dev * r.std_dev;
    r.clipped_count = count_clipped(samples, n, phase);
    r.compliant     = check_compliance(r.rms);

    /* Build bitmask status flags */
    r.status_flags = 0;
    if (r.clipped_count > 0) r.status_flags |= FLAG_CLIPPING;
    if (!r.compliant)        r.status_flags |= FLAG_OUT_OF_TOL;

    return r;
}
