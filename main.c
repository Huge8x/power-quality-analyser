/*
 * main.c
 * Entry point for the Power Quality Waveform Analyser.
 *
 * Usage: ./power_analyser <csv_file>
 *
 * Responsibilities:
 *   1. Parse the command-line argument (filename).
 *   2. Load the CSV into a dynamically allocated struct array.
 *   3. Run analysis on each of the three phases.
 *   4. Print a summary to stdout.
 *   5. Write the full report to results.txt.
 *   6. Free all allocated memory before exit.
 *
 * No analysis logic lives here — that belongs in waveform.c.
 * No file I/O logic lives here — that belongs in io.c.
 */

#include <stdio.h>//This lets me use printf and fprintf to print text to the screen.
#include <stdlib.h>//This lets me use malloc and free for memory management.

#include "waveform.h"
#include "io.h"//These include my own files so main.c can use the functions and structs you defined there.

int main(int argc, char *argv[])//This is where the program starts. argc is how many arguments were passed, argv is the actual arguments. argv[1] is the CSV filename you type when running the program.
{
    /* ---- 1. Check command-line argument ---- */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <power_quality_log.csv>\n", argv[0]);
        return EXIT_FAILURE;//If the user didn't provide exactly one argument (the CSV file), print an error and stop.
    }

    const char *csv_path    = argv[1];
    const char *report_path = "results.txt";

    /* ---- 2. Load CSV into dynamically allocated array ---- */
    WaveformSample *samples = NULL;   /* pointer to be set by load_csv */
    int n = load_csv(csv_path, &samples);//Create a pointer called samples (starts as NULL), then call load_csv which fills it with data from the CSV and returns how many rows were loaded.


    if (n <= 0) {
        /* load_csv already printed an error message */
        return EXIT_FAILURE;//If loading failed, stop the program.

    }

    printf("Loaded %d samples from '%s'\n\n", n, csv_path);

    /* ---- 3. Analyse each phase ---- */
    PhaseResult results[3];
    for (int phase = 0; phase < 3; phase++) {
        results[phase] = analyse_phase(samples, n, phase);//Create an array of 3 PhaseResult structs (one per phase). Loop through phases 0, 1, 2 and analyse each one.

    }

    /* ---- 4. Analyse system-wide columns ---- */
    double freq_mean, freq_min, freq_max;
    analyse_frequency(samples, n, &freq_mean, &freq_min, &freq_max);

    double mpf  = mean_power_factor(samples, n);
    double mthd = mean_thd(samples, n);

    /* ---- 5. Print summary to stdout ---- */
    printf("=============================================================\n");
    printf("  POWER QUALITY WAVEFORM ANALYSER — SUMMARY\n");
    printf("=============================================================\n\n");

    for (int i = 0; i < 3; i++) {
        const PhaseResult *r = &results[i];
        printf("Phase %c:\n", r->label);
        printf("  RMS           : %.4f V  [%s]\n",
               r->rms, r->compliant ? "COMPLIANT" : "OUT OF TOLERANCE");
        printf("  Peak-to-peak  : %.4f V\n", r->peak_to_peak);
        printf("  DC offset     : %+.6f V\n", r->dc_offset);
        printf("  Std deviation : %.4f V\n", r->std_dev);
        printf("  Clipped       : %d samples  (flags: 0x%02X)\n\n",
               r->clipped_count, r->status_flags);
    }

    printf("Frequency : mean=%.4f Hz  min=%.4f Hz  max=%.4f Hz\n",
           freq_mean, freq_min, freq_max);
    printf("Power factor (mean) : %.4f\n", mpf);
    printf("THD (mean)          : %.4f %%\n\n", mthd);

    int total_clipped = results[0].clipped_count
                      + results[1].clipped_count
                      + results[2].clipped_count;
    printf("Total clipped samples (all phases): %d\n\n", total_clipped);

    /* ---- 6. Write full report ---- */
    if (write_report(report_path, results,
                     freq_mean, freq_min, freq_max,
                     mpf, mthd, n) == 0) {
        printf("Report written to '%s'\n", report_path);
    }

    /* ---- 7. Free dynamically allocated memory ---- */
    free(samples);
    /* Set to NULL after free to prevent use-after-free bugs
   if pointer is accidentally accessed again */
    samples = NULL;
    return EXIT_SUCCESS;
}
