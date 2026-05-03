/*
 * io.c
 * File I/O: CSV loading and results-file writing.
 */

#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum length of one CSV line (generous margin) */
#define MAX_LINE_LEN 512

/* ---------------------------------------------------------------
 * load_csv
 * Reads the CSV in two passes:
 *   Pass 1 – count data rows so we know how much to malloc.
 *   Pass 2 – rewind and parse each row into the struct array.
 *
 * This avoids realloc() churn and gives predictable memory use.
 * --------------------------------------------------------------- */
int load_csv(const char *filepath, WaveformSample **samples)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", filepath);
        *samples = NULL;
        return -1;
    }

    char line[MAX_LINE_LEN];

    /* --- Pass 1: count rows (skip header) ---
 * Two-pass strategy chosen over realloc to avoid
 * repeated memory copies and unpredictable performance.
 * We count first so malloc is called exactly once. --- */
    int row_count = 0;
    /* Skip header line */
    if (!fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "Error: file '%s' appears to be empty\n", filepath);
        fclose(fp);
        *samples = NULL;
        return -1;
    }
    while (fgets(line, sizeof(line), fp)) {
        /* Ignore blank lines */
        if (strlen(line) > 1) row_count++;
    }

    if (row_count == 0) {
        fprintf(stderr, "Error: no data rows found in '%s'\n", filepath);
        fclose(fp);
        *samples = NULL;
        return -1;
    }

    /* --- Allocate memory for exactly row_count samples --- */
    *samples = (WaveformSample *)malloc((size_t)row_count * sizeof(WaveformSample));
    if (!*samples) {
        fprintf(stderr, "Error: malloc failed for %d samples\n", row_count);
        fclose(fp);
        return -1;
    }

    /* --- Pass 2: rewind and parse --- */
    rewind(fp);
    fgets(line, sizeof(line), fp);   /* skip header again */

    int idx = 0;
    WaveformSample *ptr = *samples;  /* pointer-based write */

    while (idx < row_count && fgets(line, sizeof(line), fp)) {
        if (strlen(line) <= 1) continue;  /* skip blank lines */

        int parsed = sscanf(line,
            "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
            &ptr->timestamp,
            &ptr->phase_A_voltage,
            &ptr->phase_B_voltage,
            &ptr->phase_C_voltage,
            &ptr->line_current,
            &ptr->frequency,
            &ptr->power_factor,
            &ptr->thd_percent);

        if (parsed != 8) {
            fprintf(stderr, "Warning: row %d malformed (only %d fields), skipping\n",
                    idx + 1, parsed);
        } else {
            ptr++;
            idx++;
        }
    }

    fclose(fp);

    /* If some rows were skipped, return the actual count */
    return idx;
}

/* ---------------------------------------------------------------
 * write_report
 * Writes a human-readable plain-text report to the given file.
 * --------------------------------------------------------------- */
int write_report(const char *filepath,
                 const PhaseResult results[3],
                 double freq_mean, double freq_min, double freq_max,
                 double mean_pf, double mean_thd_val,
                 int total_samples)
{
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot write report to '%s'\n", filepath);
        return -1;
    }

    fprintf(fp, "=============================================================\n");
    fprintf(fp, "  POWER QUALITY ANALYSIS REPORT\n");
    fprintf(fp, "  UGMFGT-15-1 – Programming for Engineers\n");
    fprintf(fp, "=============================================================\n\n");

    fprintf(fp, "Dataset summary\n");
    fprintf(fp, "  Total samples analysed : %d\n", total_samples);
    fprintf(fp, "  Sample period          : 0.2 ms  (5000 samples/s)\n");
    fprintf(fp, "  Total window           : %.1f ms\n\n",
            total_samples * 0.2);

    /* ---- Per-phase results ---- */
    for (int i = 0; i < 3; i++) {
        const PhaseResult *r = &results[i];
        fprintf(fp, "-------------------------------------------------------------\n");
        fprintf(fp, "  Phase %c\n", r->label);
        fprintf(fp, "-------------------------------------------------------------\n");
        fprintf(fp, "  RMS voltage        : %8.4f V", r->rms);
        fprintf(fp, "  [%s]\n", r->compliant ? "COMPLIANT  (EN 50160)" : "OUT OF TOLERANCE!");
        fprintf(fp, "  Peak-to-peak       : %8.4f V\n", r->peak_to_peak);
        fprintf(fp, "  DC offset          : %+.6f V\n", r->dc_offset);
        fprintf(fp, "  Standard deviation : %8.4f V\n", r->std_dev);
        fprintf(fp, "  Variance           : %8.4f V^2\n", r->variance);
        fprintf(fp, "  Clipped samples    : %d\n", r->clipped_count);

        /* Decode and display bitmask flags */
        fprintf(fp, "  Status flags       : 0x%02X", r->status_flags);
        if (r->status_flags == 0) {
            fprintf(fp, "  (no faults)\n");
        } else {
            fprintf(fp, " –");
            if (r->status_flags & FLAG_CLIPPING)   fprintf(fp, " CLIPPING");
            if (r->status_flags & FLAG_OUT_OF_TOL) fprintf(fp, " OUT_OF_TOLERANCE");
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    /* ---- System-wide metrics ---- */
    fprintf(fp, "-------------------------------------------------------------\n");
    fprintf(fp, "  System-wide metrics\n");
    fprintf(fp, "-------------------------------------------------------------\n");
    fprintf(fp, "  Frequency (mean)   : %.6f Hz\n", freq_mean);
    fprintf(fp, "  Frequency (min)    : %.6f Hz\n", freq_min);
    fprintf(fp, "  Frequency (max)    : %.6f Hz\n", freq_max);
    fprintf(fp, "  Mean power factor  : %.4f\n", mean_pf);
    fprintf(fp, "  Mean THD           : %.4f %%\n", mean_thd_val);

    /* Total clipped across all phases */
    int total_clipped = results[0].clipped_count
                      + results[1].clipped_count
                      + results[2].clipped_count;
    fprintf(fp, "  Clipped total      : %d samples (all phases combined)\n",
            total_clipped);

    fprintf(fp, "\n=============================================================\n");
    fprintf(fp, "  END OF REPORT\n");
    fprintf(fp, "=============================================================\n");

    fclose(fp);
    return 0;
}
