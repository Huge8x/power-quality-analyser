#ifndef IO_H
#define IO_H

#include "waveform.h"

/*
 * load_csv
 * Opens the CSV at 'filepath', skips the header row, and reads
 * every data row into a dynamically allocated array of WaveformSample.
 *
 * On success: *samples points to the malloc'd array, returns row count.
 * On failure: prints an error, sets *samples to NULL, returns -1.
 *
 * The caller is responsible for free()-ing *samples when done.
 */
int load_csv(const char *filepath, WaveformSample **samples);

/*
 * write_report
 * Writes the analysis results to 'filepath' (usually "results.txt").
 * Returns 0 on success, -1 on failure.
 */
int write_report(const char *filepath,
                 const PhaseResult results[3],
                 double freq_mean, double freq_min, double freq_max,
                 double mean_pf, double mean_thd_val,
                 int total_samples);

#endif /* IO_H */
