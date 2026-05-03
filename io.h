#ifndef IO_H   /* Header guard - prevents this file being included more than once */
#define IO_H   /* If IO_H is not defined yet, define it and include the contents */

#include "waveform.h" /* Includes WaveformSample and PhaseResult struct definitions */

/* ---------------------------------------------------------------
 * load_csv
 * Opens the CSV at filepath, skips the header row, and reads
 * every data row into a dynamically allocated array of WaveformSample.
 *
 * Uses a two pass strategy:
 *   Pass 1 - counts rows so malloc can be called once with exact size
 *   Pass 2 - parses each row into the struct array
 *
 * filepath  - path to the CSV file to load
 * samples   - pointer to pointer, will be set to the allocated array
 *
 * On success: *samples points to the malloc'd array, returns row count.
 * On failure: prints an error, sets *samples to NULL, returns -1.
 *
 * The caller is responsible for free()-ing *samples when done.
 * --------------------------------------------------------------- */
int load_csv(const char *filepath, WaveformSample **samples);

/* ---------------------------------------------------------------
 * write_report
 * Writes the full analysis results to a plain text file.
 *
 * filepath      - path to the output file (usually "results.txt")
 * results       - array of 3 PhaseResult structs (one per phase)
 * freq_mean     - mean frequency across all samples in Hz
 * freq_min      - minimum frequency seen in Hz
 * freq_max      - maximum frequency seen in Hz
 * mean_pf       - mean power factor across all samples
 * mean_thd_val  - mean THD percentage across all samples
 * total_samples - total number of samples that were analysed
 *
 * Returns 0 on success, -1 on failure.
 * --------------------------------------------------------------- */
int write_report(const char *filepath,
                 const PhaseResult results[3],
                 double freq_mean, double freq_min, double freq_max,
                 double mean_pf, double mean_thd_val,
                 int total_samples);

#endif /* IO_H - end of header guard */