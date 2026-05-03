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

#include <stdio.h>    /* Gives access to printf and fprintf for printing to screen */
#include <stdlib.h>   /* Gives access to malloc and free for memory management */

#include "waveform.h" /* Includes our own struct definitions and analysis function declarations */
#include "io.h"       /* Includes our own file loading and report writing functions */

/* main is where the program starts
 * argc = number of arguments passed when running the program
 * argv = array of the actual argument strings
 * argv[0] = the program name, argv[1] = the CSV filename */
int main(int argc, char *argv[])
{
    /* ---- 1. Check command-line argument ---- */

    /* If the user didn't pass exactly one argument (the CSV file), print an error and stop */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <power_quality_log.csv>\n", argv[0]); /* Print usage instructions to stderr */
        return EXIT_FAILURE; /* Exit the program with failure code */
    }

    const char *csv_path    = argv[1];        /* Store the CSV file path from the command line argument */
    const char *report_path = "results.txt";  /* Hard code the output report filename */

    /* ---- 2. Load CSV into dynamically allocated array ---- */

    WaveformSample *samples = NULL;  /* Create a pointer to hold the sample array, starts as NULL */
    int n = load_csv(csv_path, &samples); /* Call load_csv which allocates memory and fills the array,
                                             returns the number of rows loaded, or -1 if it failed */

    /* If loading failed (n is 0 or negative), stop the program */
    if (n <= 0) {
        return EXIT_FAILURE; /* load_csv already printed an error message so just exit */
    }

    /* Print how many samples were successfully loaded */
    printf("Loaded %d samples from '%s'\n\n", n, csv_path);

    /* ---- 3. Analyse each phase ---- */

    PhaseResult results[3]; /* Create an array of 3 PhaseResult structs, one for each phase A, B, C */

    /* Loop through phases 0 (A), 1 (B), 2 (C) and analyse each one */
    for (int phase = 0; phase < 3; phase++) {
        results[phase] = analyse_phase(samples, n, phase); /* Call analyse_phase which computes all
                                                              metrics and returns a PhaseResult struct */
    }

    /* ---- 4. Analyse system-wide columns ---- */

    double freq_mean, freq_min, freq_max; /* Declare variables to hold frequency statistics */
    analyse_frequency(samples, n, &freq_mean, &freq_min, &freq_max); /* Pass pointers so the function
                                                                         can write the results back */

    double mpf  = mean_power_factor(samples, n); /* Calculate the mean power factor across all samples */
    double mthd = mean_thd(samples, n);          /* Calculate the mean THD across all samples */

    /* ---- 5. Print summary to stdout ---- */

    /* Print the header of the summary report */
    printf("=============================================================\n");
    printf("  POWER QUALITY WAVEFORM ANALYSER - SUMMARY\n");
    printf("=============================================================\n\n");

    /* Loop through the three phases and print results for each one */
    for (int i = 0; i < 3; i++) {
        const PhaseResult *r = &results[i]; /* Get a pointer to the current phase result */

        printf("Phase %c:\n", r->label); /* Print the phase label (A, B, or C) */

        /* Print RMS voltage and whether it is compliant with EN 50160 standard */
        printf("  RMS           : %.4f V  [%s]\n",
               r->rms, r->compliant ? "COMPLIANT" : "OUT OF TOLERANCE");

        printf("  Peak-to-peak  : %.4f V\n", r->peak_to_peak); /* Print the peak to peak voltage */
        printf("  DC offset     : %+.6f V\n", r->dc_offset);   /* Print the DC offset, should be ~0V */
        printf("  Std deviation : %.4f V\n", r->std_dev);       /* Print the standard deviation */

        /* Print clipped sample count and the bitmask status flags in hex */
        printf("  Clipped       : %d samples  (flags: 0x%02X)\n\n",
               r->clipped_count, r->status_flags);
    }

    /* Print system wide frequency statistics */
    printf("Frequency : mean=%.4f Hz  min=%.4f Hz  max=%.4f Hz\n",
           freq_mean, freq_min, freq_max);

    printf("Power factor (mean) : %.4f\n", mpf);       /* Print the mean power factor */
    printf("THD (mean)          : %.4f %%\n\n", mthd); /* Print the mean THD percentage */

    /* Add up clipped samples across all three phases and print the total */
    int total_clipped = results[0].clipped_count   /* Clipped count for Phase A */
                      + results[1].clipped_count   /* Clipped count for Phase B */
                      + results[2].clipped_count;  /* Clipped count for Phase C */
    printf("Total clipped samples (all phases): %d\n\n", total_clipped);

    /* ---- 6. Write full report ---- */

    /* Call write_report to save all results to results.txt
     * If it succeeds (returns 0) print a confirmation message */
    if (write_report(report_path, results,
                     freq_mean, freq_min, freq_max,
                     mpf, mthd, n) == 0) {
        printf("Report written to '%s'\n", report_path); /* Confirm the report was saved */
    }

    /* ---- 7. Free dynamically allocated memory ---- */

    free(samples); /* Release the memory that was allocated by malloc in load_csv */

    /* Set to NULL after free to prevent use-after-free bugs
       if the pointer is accidentally accessed again */
    samples = NULL;

    return EXIT_SUCCESS; /* Exit the program successfully */
}