#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <stdint.h>

/* ---------------------------------------------------------------
 * WaveformSample
 * Holds all 8 fields for a single row of the CSV.
 * All voltage values are in Volts; frequency in Hz; etc.
 * --------------------------------------------------------------- */
typedef struct {
    double timestamp;        /* seconds */
    double phase_A_voltage;  /* V */
    double phase_B_voltage;  /* V */
    double phase_C_voltage;  /* V */
    double line_current;     /* A */
    double frequency;        /* Hz */
    double power_factor;     /* 0–1 */
    double thd_percent;      /* % */
} WaveformSample;

/* ---------------------------------------------------------------
 * PhaseResult
 * Holds every computed metric for one phase.
 * The status bitmask encodes health flags:
 *   bit 0 (0x01) – clipping detected
 *   bit 1 (0x02) – out of tolerance (RMS outside 207–253 V)
 * --------------------------------------------------------------- */
typedef struct {
    char   label;        /* 'A', 'B', or 'C' */
    double rms;
    double peak_to_peak;
    double dc_offset;
    double std_dev;
    double variance;
    int    clipped_count;
    int    compliant;    /* 1 = within EN 50160 band, 0 = outside */
    uint8_t status_flags; /* bitmask: bit0=clipping, bit1=out-of-tolerance */
} PhaseResult;

/* Sensor hard limit (V) — samples at or beyond this are clipped */
#define CLIP_THRESHOLD  324.9

/* EN 50160 compliance band (±10 % of 230 V nominal) */
#define NOMINAL_VOLTAGE 230.0
#define TOLERANCE_LOW   207.0
#define TOLERANCE_HIGH  253.0

/* Bitmask constants */
#define FLAG_CLIPPING    0x01
#define FLAG_OUT_OF_TOL  0x02

/* ---------------------------------------------------------------
 * Analysis functions  (implemented in waveform.c)
 * All accept a pointer to the sample array and the count n.
 * --------------------------------------------------------------- */

/* RMS of one phase; phase selects which voltage field to use (0=A,1=B,2=C) */
double compute_rms(const WaveformSample *samples, int n, int phase);

/* Peak-to-peak of one phase */
double compute_peak_to_peak(const WaveformSample *samples, int n, int phase);

/* DC offset (arithmetic mean) of one phase */
double compute_dc_offset(const WaveformSample *samples, int n, int phase);

/* Standard deviation of one phase voltage */
double compute_std_dev(const WaveformSample *samples, int n, int phase,
                       double dc_offset);

/* Count samples where |voltage| >= CLIP_THRESHOLD */
int count_clipped(const WaveformSample *samples, int n, int phase);

/* Returns 1 if rms is within [TOLERANCE_LOW, TOLERANCE_HIGH] */
int check_compliance(double rms);

/* Helper – returns a pointer to the correct voltage field */
const double *get_phase_ptr(const WaveformSample *sample, int phase);

/* Compute mean and range of frequency column */
void analyse_frequency(const WaveformSample *samples, int n,
                       double *mean_out, double *min_out, double *max_out);

/* Compute mean power factor */
double mean_power_factor(const WaveformSample *samples, int n);

/* Compute mean THD */
double mean_thd(const WaveformSample *samples, int n);

/* Fill a PhaseResult for the given phase index */
PhaseResult analyse_phase(const WaveformSample *samples, int n, int phase);

#endif /* WAVEFORM_H */
