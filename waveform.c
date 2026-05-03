/*
 * waveform.c
 * Power-quality analysis functions.
 * All maths uses double precision throughout.
 */

#include "waveform.h" /* Includes our struct definitions and function declarations */
#include <math.h>     /* Gives access to sqrt() for RMS and standard deviation calculations */
#include <stdio.h>    /* Gives access to fprintf for printing warning messages */

/* ---------------------------------------------------------------
 * get_phase_ptr
 * Returns a pointer to the correct voltage field inside a single
 * WaveformSample struct based on the phase number (0=A, 1=B, 2=C).
 * This avoids duplicating the loop body three times for each phase.
 * --------------------------------------------------------------- */
const double *get_phase_ptr(const WaveformSample *sample, int phase)
{
    switch (phase) {                          /* Check which phase was requested */
        case 0:  return &sample->phase_A_voltage; /* Return pointer to Phase A voltage field */
        case 1:  return &sample->phase_B_voltage; /* Return pointer to Phase B voltage field */
        default: return &sample->phase_C_voltage; /* Return pointer to Phase C voltage field */
    }
}

/* ---------------------------------------------------------------
 * compute_rms
 * RMS = sqrt( (1/N) * sum( v[i]^2 ) )
 * We square every sample, take the mean, then square root.
 * Squaring is needed because a plain average of AC is always zero.
 * --------------------------------------------------------------- */
double compute_rms(const WaveformSample *samples, int n, int phase)
{
    double sum_sq = 0.0;                     /* Accumulator for the sum of squared values */
    const WaveformSample *p = samples;       /* Pointer that walks through the array */

    for (int i = 0; i < n; i++, p++) {      /* Loop through every sample, p++ moves to next struct */
        double v = *get_phase_ptr(p, phase); /* Dereference the pointer to get the voltage value */
        sum_sq += v * v;                     /* Square the voltage and add to the running total */
    }
    return sqrt(sum_sq / (double)n);         /* Divide by n to get the mean, then square root */
}

/* ---------------------------------------------------------------
 * compute_peak_to_peak
 * V_pp = V_max - V_min across all samples.
 * Walks through every sample keeping track of highest and lowest.
 * --------------------------------------------------------------- */
double compute_peak_to_peak(const WaveformSample *samples, int n, int phase)
{
    const WaveformSample *p = samples;        /* Pointer that walks through the array */
    double vmax = *get_phase_ptr(p, phase);   /* Start with the first sample as both max and min */
    double vmin = vmax;                        /* Initialise min to the same first value */

    for (int i = 1; i < n; i++) {             /* Loop from second sample onwards */
        p++;                                   /* Advance pointer to the next struct */
        double v = *get_phase_ptr(p, phase);   /* Get the voltage value for this sample */
        if (v > vmax) vmax = v;                /* Update max if this sample is higher */
        if (v < vmin) vmin = v;                /* Update min if this sample is lower */
    }
    return vmax - vmin;                        /* Return the difference between highest and lowest */
}

/* ---------------------------------------------------------------
 * compute_dc_offset
 * DC offset = arithmetic mean of all samples.
 * For a pure AC signal this should be ~0V because positive and
 * negative halves cancel exactly. Non-zero means a fault.
 * --------------------------------------------------------------- */
double compute_dc_offset(const WaveformSample *samples, int n, int phase)
{
    double sum = 0.0;                         /* Accumulator for the sum of all voltage values */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */

    for (int i = 0; i < n; i++, p++) {       /* Loop through every sample */
        sum += *get_phase_ptr(p, phase);      /* Add the voltage value to the running total */
    }
    return sum / (double)n;                   /* Divide by n to get the arithmetic mean */
}

/* ---------------------------------------------------------------
 * compute_std_dev
 * sigma = sqrt( (1/N) * sum( (v[i] - mean)^2 ) )
 * Measures how spread out the samples are from the mean.
 * The caller supplies dc_offset (the mean) to avoid a second pass.
 * --------------------------------------------------------------- */
double compute_std_dev(const WaveformSample *samples, int n, int phase,
                       double dc_offset)
{
    double sum_sq_diff = 0.0;                 /* Accumulator for sum of squared differences */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */

    for (int i = 0; i < n; i++, p++) {       /* Loop through every sample */
        double diff = *get_phase_ptr(p, phase) - dc_offset; /* Subtract the mean from this sample */
        sum_sq_diff += diff * diff;           /* Square the difference and add to running total */
    }
    return sqrt(sum_sq_diff / (double)n);     /* Divide by n to get the mean, then square root */
}

/* ---------------------------------------------------------------
 * count_clipped
 * Counts samples where |voltage| >= CLIP_THRESHOLD (324.9V).
 * At that point the sensor hits its physical limit and the
 * waveform goes flat at the top instead of continuing to curve.
 * --------------------------------------------------------------- */
int count_clipped(const WaveformSample *samples, int n, int phase)
{
    int count = 0;                            /* Counter for number of clipped samples found */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */

    for (int i = 0; i < n; i++, p++) {       /* Loop through every sample */
        double v = *get_phase_ptr(p, phase);  /* Get the voltage value for this sample */
        if (v >= CLIP_THRESHOLD || v <= -CLIP_THRESHOLD) { /* Check both positive and negative peaks */
            count++;                          /* Increment counter if this sample is clipped */
        }
    }
    return count;                             /* Return total number of clipped samples found */
}

/* ---------------------------------------------------------------
 * check_compliance
 * Returns 1 if rms is within the EN 50160 band [207, 253] V.
 * EN 50160 is the European standard for power quality.
 * UK nominal voltage is 230V so the allowed range is +/-10%.
 * --------------------------------------------------------------- */
int check_compliance(double rms)
{
    /* Ternary operator: if condition is true return 1, otherwise return 0 */
    return (rms >= TOLERANCE_LOW && rms <= TOLERANCE_HIGH) ? 1 : 0;
}

/* ---------------------------------------------------------------
 * analyse_frequency
 * Walks the frequency column to find mean, min, and max values.
 * Uses pointer parameters so results can be written back to caller.
 * --------------------------------------------------------------- */
void analyse_frequency(const WaveformSample *samples, int n,
                       double *mean_out, double *min_out, double *max_out)
{
    double sum  = 0.0;                        /* Accumulator for sum of all frequency values */
    double fmin = samples->frequency;         /* Start min at the first sample's frequency */
    double fmax = fmin;                        /* Start max at the same first value */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */

    for (int i = 0; i < n; i++, p++) {       /* Loop through every sample */
        double f = p->frequency;              /* Get the frequency value for this sample */
        sum += f;                             /* Add to running total for mean calculation */
        if (f < fmin) fmin = f;              /* Update min if this frequency is lower */
        if (f > fmax) fmax = f;              /* Update max if this frequency is higher */
    }
    *mean_out = sum / (double)n;              /* Write mean back through pointer to caller */
    *min_out  = fmin;                         /* Write min back through pointer to caller */
    *max_out  = fmax;                         /* Write max back through pointer to caller */
}

/* ---------------------------------------------------------------
 * mean_power_factor
 * Calculates the arithmetic mean of the power_factor column.
 * Power factor of 1.0 is ideal, 0.95 is typical industrial load.
 * --------------------------------------------------------------- */
double mean_power_factor(const WaveformSample *samples, int n)
{
    double sum = 0.0;                         /* Accumulator for sum of all power factor values */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */
    for (int i = 0; i < n; i++, p++) sum += p->power_factor; /* Add each power factor to total */
    return sum / (double)n;                   /* Divide by n to get the mean */
}

/* ---------------------------------------------------------------
 * mean_thd
 * Calculates the arithmetic mean of the thd_percent column.
 * THD below 5% is clean, above 8% is excessive per EN 50160.
 * --------------------------------------------------------------- */
double mean_thd(const WaveformSample *samples, int n)
{
    double sum = 0.0;                         /* Accumulator for sum of all THD values */
    const WaveformSample *p = samples;        /* Pointer that walks through the array */
    for (int i = 0; i < n; i++, p++) sum += p->thd_percent; /* Add each THD value to total */
    return sum / (double)n;                   /* Divide by n to get the mean */
}

/* ---------------------------------------------------------------
 * analyse_phase
 * Convenience wrapper that calls all analysis functions and
 * packages every metric into one PhaseResult struct.
 * Also sets the bitmask status flags based on results.
 * --------------------------------------------------------------- */
PhaseResult analyse_phase(const WaveformSample *samples, int n, int phase)
{
    PhaseResult r;                            /* Create a PhaseResult struct to hold all results */
    r.label         = (char)('A' + phase);   /* Set label to A, B, or C based on phase number */
    r.rms           = compute_rms(samples, n, phase);           /* Calculate RMS voltage */
    r.peak_to_peak  = compute_peak_to_peak(samples, n, phase);  /* Calculate peak to peak voltage */
    r.dc_offset     = compute_dc_offset(samples, n, phase);     /* Calculate DC offset */
    r.std_dev       = compute_std_dev(samples, n, phase, r.dc_offset); /* Calculate standard deviation */
    r.variance      = r.std_dev * r.std_dev; /* Variance is standard deviation squared */
    r.clipped_count = count_clipped(samples, n, phase);         /* Count clipped samples */
    r.compliant     = check_compliance(r.rms);                  /* Check EN 50160 compliance */

    /* Build the bitmask status flags byte */
    r.status_flags = 0;                                          /* Start with all bits cleared (no faults) */
    if (r.clipped_count > 0) r.status_flags |= FLAG_CLIPPING;   /* Set bit 0 if clipping was detected */
    if (!r.compliant)        r.status_flags |= FLAG_OUT_OF_TOL; /* Set bit 1 if RMS is out of tolerance */

    return r;                                /* Return the completed PhaseResult struct */
}