#ifndef WAVEFORM_H   /* Header guard - prevents this file being included more than once */
#define WAVEFORM_H   /* If WAVEFORM_H is not defined yet, define it and include the contents */

#include <stdint.h>  /* Gives access to uint8_t which is an unsigned 8-bit integer for the bitmask */

/* ---------------------------------------------------------------
 * WaveformSample
 * Holds all 8 fields for a single row of the CSV file.
 * All voltage values are in Volts, frequency in Hz etc.
 * Using a struct keeps all values from the same timestamp together
 * rather than having 8 separate arrays which would be harder to manage.
 * --------------------------------------------------------------- */
typedef struct {
    double timestamp;        /* Time in seconds when this sample was taken */
    double phase_A_voltage;  /* Voltage of Phase A in Volts */
    double phase_B_voltage;  /* Voltage of Phase B in Volts - lags Phase A by 120 degrees */
    double phase_C_voltage;  /* Voltage of Phase C in Volts - leads Phase A by 120 degrees */
    double line_current;     /* Current in Amperes */
    double frequency;        /* Frequency in Hz - should be ~50Hz for UK mains */
    double power_factor;     /* Power factor between 0 and 1 - 1.0 is ideal, 0.95 is typical */
    double thd_percent;      /* Total Harmonic Distortion as a percentage - below 5% is clean */
} WaveformSample;            /* typedef lets us use WaveformSample as the type name */

/* ---------------------------------------------------------------
 * PhaseResult
 * Holds every computed metric for one phase after analysis.
 * The status_flags bitmask encodes health flags:
 *   bit 0 (0x01) - clipping detected
 *   bit 1 (0x02) - RMS out of EN 50160 tolerance
 * --------------------------------------------------------------- */
typedef struct {
    char   label;         /* Phase label - A, B, or C */
    double rms;           /* RMS voltage in Volts - the most important metric */
    double peak_to_peak;  /* Peak to peak voltage in Volts - difference between max and min sample */
    double dc_offset;     /* DC offset in Volts - arithmetic mean, should be ~0V for clean AC */
    double std_dev;       /* Standard deviation of the voltage samples in Volts */
    double variance;      /* Variance in Volts squared - standard deviation squared */
    int    clipped_count; /* Number of samples where voltage hit the sensor limit of 324.9V */
    int    compliant;     /* 1 if RMS is within EN 50160 band (207-253V), 0 if outside */
    uint8_t status_flags; /* Bitmask: bit0=clipping detected, bit1=RMS out of tolerance */
} PhaseResult;

/* ---------------------------------------------------------------
 * Constants
 * Using #define means if values need changing we only change them
 * in one place here rather than hunting through all the code.
 * --------------------------------------------------------------- */

/* Sensor hard limit in Volts - samples at or beyond this value are clipped */
#define CLIP_THRESHOLD  324.9

/* EN 50160 compliance band - UK nominal is 230V so allowed range is +/-10% */
#define NOMINAL_VOLTAGE 230.0  /* Nominal UK mains voltage in Volts */
#define TOLERANCE_LOW   207.0  /* Lower limit - 230V minus 10% */
#define TOLERANCE_HIGH  253.0  /* Upper limit - 230V plus 10% */

/* Bitmask flag constants - used with bitwise OR to set individual bits */
#define FLAG_CLIPPING    0x01  /* Binary 00000001 - bit 0 indicates clipping was detected */
#define FLAG_OUT_OF_TOL  0x02  /* Binary 00000010 - bit 1 indicates RMS is out of tolerance */

/* ---------------------------------------------------------------
 * Analysis function declarations
 * These tell the compiler what functions exist in waveform.c
 * so other files can call them without seeing the implementation.
 * phase parameter: 0 = Phase A, 1 = Phase B, 2 = Phase C
 * --------------------------------------------------------------- */

/* Returns a pointer to the correct voltage field for the given phase */
const double *get_phase_ptr(const WaveformSample *sample, int phase);

/* Calculates RMS voltage using square-mean-root method */
double compute_rms(const WaveformSample *samples, int n, int phase);

/* Finds the difference between the highest and lowest voltage samples */
double compute_peak_to_peak(const WaveformSample *samples, int n, int phase);

/* Calculates the arithmetic mean of all samples - should be ~0V for clean AC */
double compute_dc_offset(const WaveformSample *samples, int n, int phase);

/* Calculates standard deviation - caller supplies mean to avoid a second loop */
double compute_std_dev(const WaveformSample *samples, int n, int phase,
                       double dc_offset);

/* Counts samples where absolute voltage is at or above the clip threshold */
int count_clipped(const WaveformSample *samples, int n, int phase);

/* Returns 1 if RMS is within EN 50160 tolerance band, 0 if outside */
int check_compliance(double rms);

/* Finds mean, min and max of the frequency column across all samples */
void analyse_frequency(const WaveformSample *samples, int n,
                       double *mean_out, double *min_out, double *max_out);

/* Calculates the mean power factor across all samples */
double mean_power_factor(const WaveformSample *samples, int n);

/* Calculates the mean THD percentage across all samples */
double mean_thd(const WaveformSample *samples, int n);

/* Wrapper that calls all analysis functions and returns a complete PhaseResult */
PhaseResult analyse_phase(const WaveformSample *samples, int n, int phase);

#endif /* WAVEFORM_H - end of header guard */