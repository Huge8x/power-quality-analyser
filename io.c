/*
 * io.c
 * File I/O: CSV loading and results-file writing.
 */

#include "io.h"       /* Includes our own function declarations for load_csv and write_report */
#include <stdio.h>    /* Gives access to fopen, fclose, fgets, fprintf, rewind for file operations */
#include <stdlib.h>   /* Gives access to malloc and free for memory management */
#include <string.h>   /* Gives access to strlen for checking line length */

/* Maximum length of one CSV line - set generously to avoid cutting off long lines */
#define MAX_LINE_LEN 512

/* ---------------------------------------------------------------
 * load_csv
 * Reads the CSV in two passes:
 *   Pass 1 - count data rows so we know how much memory to malloc.
 *   Pass 2 - rewind and parse each row into the struct array.
 *
 * This avoids realloc() churn and gives predictable memory use.
 * Two-pass strategy chosen over realloc to avoid repeated memory
 * copies and unpredictable performance. We count first so malloc
 * is called exactly once with the right size.
 * --------------------------------------------------------------- */
int load_csv(const char *filepath, WaveformSample **samples)
{
    FILE *fp = fopen(filepath, "r"); /* Try to open the CSV file in read mode */
    if (!fp) {                       /* If fopen returns NULL the file could not be opened */
        fprintf(stderr, "Error: cannot open '%s'\n", filepath); /* Print error to stderr */
        *samples = NULL;             /* Set the samples pointer to NULL so caller knows it failed */
        return -1;                   /* Return -1 to signal failure */
    }

    char line[MAX_LINE_LEN];         /* Buffer to hold one line of the CSV at a time */

    /* --- Pass 1: count rows (skip header) --- */
    int row_count = 0;               /* Counter for number of data rows found */

    /* Read and discard the first line which is the header row */
    if (!fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "Error: file '%s' appears to be empty\n", filepath); /* File has no content */
        fclose(fp);                  /* Close the file before returning */
        *samples = NULL;             /* Set samples to NULL so caller knows it failed */
        return -1;                   /* Return -1 to signal failure */
    }

    /* Read every remaining line and count it if it is not blank */
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) > 1) row_count++; /* Only count lines with actual content, skip blank lines */
    }

    /* If no data rows were found the file only had a header */
    if (row_count == 0) {
        fprintf(stderr, "Error: no data rows found in '%s'\n", filepath); /* Print error to stderr */
        fclose(fp);                  /* Close the file before returning */
        *samples = NULL;             /* Set samples to NULL so caller knows it failed */
        return -1;                   /* Return -1 to signal failure */
    }

    /* --- Allocate memory for exactly row_count samples --- */
    /* malloc allocates a block of memory large enough for row_count WaveformSample structs */
    /* sizeof(WaveformSample) gives the size of one struct in bytes */
    *samples = (WaveformSample *)malloc((size_t)row_count * sizeof(WaveformSample));
    if (!*samples) {                 /* If malloc returns NULL it failed to allocate memory */
        fprintf(stderr, "Error: malloc failed for %d samples\n", row_count); /* Print error */
        fclose(fp);                  /* Close the file before returning */
        return -1;                   /* Return -1 to signal failure */
    }

    /* --- Pass 2: rewind and parse --- */
    rewind(fp);                      /* Move back to the beginning of the file for second pass */
    fgets(line, sizeof(line), fp);   /* Skip the header line again */

    int idx = 0;                     /* Index to track how many rows have been successfully parsed */
    WaveformSample *ptr = *samples;  /* Pointer that walks through the allocated array */

    /* Read each data line and parse it into the struct */
    while (idx < row_count && fgets(line, sizeof(line), fp)) {
        if (strlen(line) <= 1) continue; /* Skip blank lines */

        /* sscanf reads 8 comma separated double values from the line into the struct fields */
        /* %lf means read a double precision floating point number */
        int parsed = sscanf(line,
            "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
            &ptr->timestamp,         /* Read timestamp into the timestamp field */
            &ptr->phase_A_voltage,   /* Read Phase A voltage */
            &ptr->phase_B_voltage,   /* Read Phase B voltage */
            &ptr->phase_C_voltage,   /* Read Phase C voltage */
            &ptr->line_current,      /* Read line current */
            &ptr->frequency,         /* Read frequency */
            &ptr->power_factor,      /* Read power factor */
            &ptr->thd_percent);      /* Read THD percentage */

        /* Check that all 8 fields were successfully parsed */
        if (parsed != 8) {
            fprintf(stderr, "Warning: row %d malformed (only %d fields), skipping\n",
                    idx + 1, parsed); /* Print a warning but continue processing */
        } else {
            ptr++;  /* Advance the pointer to the next struct in the array */
            idx++;  /* Increment the counter of successfully parsed rows */
        }
    }

    fclose(fp); /* Close the file now that we are done reading it */

    /* Return the actual number of successfully parsed rows */
    /* This may be less than row_count if some rows were malformed */
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
    FILE *fp = fopen(filepath, "w"); /* Open the report file in write mode, creates it if it doesn't exist */
    if (!fp) {                       /* If fopen returns NULL the file could not be created */
        fprintf(stderr, "Error: cannot write report to '%s'\n", filepath); /* Print error to stderr */
        return -1;                   /* Return -1 to signal failure */
    }

    /* Print the report header */
    fprintf(fp, "=============================================================\n");
    fprintf(fp, "  POWER QUALITY ANALYSIS REPORT\n");
    fprintf(fp, "  UGMFGT-15-1 - Programming for Engineers\n");
    fprintf(fp, "=============================================================\n\n");

    /* Print dataset summary information */
    fprintf(fp, "Dataset summary\n");
    fprintf(fp, "  Total samples analysed : %d\n", total_samples);       /* Number of rows loaded */
    fprintf(fp, "  Sample period          : 0.2 ms  (5000 samples/s)\n"); /* Each sample is 0.2ms apart */
    fprintf(fp, "  Total window           : %.1f ms\n\n",
            total_samples * 0.2); /* Total time covered = samples x 0.2ms */

    /* ---- Per-phase results ---- */
    /* Loop through all three phases and print their results */
    for (int i = 0; i < 3; i++) {
        const PhaseResult *r = &results[i]; /* Get a pointer to the current phase result */

        fprintf(fp, "-------------------------------------------------------------\n");
        fprintf(fp, "  Phase %c\n", r->label);  /* Print the phase label A, B, or C */
        fprintf(fp, "-------------------------------------------------------------\n");

        /* Print RMS voltage and compliance status */
        fprintf(fp, "  RMS voltage        : %8.4f V", r->rms);
        fprintf(fp, "  [%s]\n", r->compliant ? "COMPLIANT  (EN 50160)" : "OUT OF TOLERANCE!");

        fprintf(fp, "  Peak-to-peak       : %8.4f V\n", r->peak_to_peak);  /* Print peak to peak voltage */
        fprintf(fp, "  DC offset          : %+.6f V\n", r->dc_offset);     /* Print DC offset, + forces sign */
        fprintf(fp, "  Standard deviation : %8.4f V\n", r->std_dev);       /* Print standard deviation */
        fprintf(fp, "  Variance           : %8.4f V^2\n", r->variance);    /* Print variance */
        fprintf(fp, "  Clipped samples    : %d\n", r->clipped_count);      /* Print number of clipped samples */

        /* Decode and display the bitmask status flags in a human readable way */
        fprintf(fp, "  Status flags       : 0x%02X", r->status_flags); /* Print flags as hex number */
        if (r->status_flags == 0) {
            fprintf(fp, "  (no faults)\n"); /* All bits are 0 so no faults detected */
        } else {
            fprintf(fp, " -");                                                    /* Separator */
            if (r->status_flags & FLAG_CLIPPING)   fprintf(fp, " CLIPPING");     /* Bit 0 is set */
            if (r->status_flags & FLAG_OUT_OF_TOL) fprintf(fp, " OUT_OF_TOLERANCE"); /* Bit 1 is set */
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n"); /* Blank line between phases */
    }

    /* ---- System-wide metrics ---- */
    fprintf(fp, "-------------------------------------------------------------\n");
    fprintf(fp, "  System-wide metrics\n");
    fprintf(fp, "-------------------------------------------------------------\n");
    fprintf(fp, "  Frequency (mean)   : %.6f Hz\n", freq_mean); /* Print mean frequency */
    fprintf(fp, "  Frequency (min)    : %.6f Hz\n", freq_min);  /* Print minimum frequency seen */
    fprintf(fp, "  Frequency (max)    : %.6f Hz\n", freq_max);  /* Print maximum frequency seen */
    fprintf(fp, "  Mean power factor  : %.4f\n", mean_pf);      /* Print mean power factor */
    fprintf(fp, "  Mean THD           : %.4f %%\n", mean_thd_val); /* Print mean THD percentage */

    /* Add up clipped samples from all three phases and print the combined total */
    int total_clipped = results[0].clipped_count  /* Clipped count for Phase A */
                      + results[1].clipped_count  /* Clipped count for Phase B */
                      + results[2].clipped_count; /* Clipped count for Phase C */
    fprintf(fp, "  Clipped total      : %d samples (all phases combined)\n", total_clipped);

    /* Print the report footer */
    fprintf(fp, "\n=============================================================\n");
    fprintf(fp, "  END OF REPORT\n");
    fprintf(fp, "=============================================================\n");

    fclose(fp); /* Close the report file now that writing is complete */
    return 0;   /* Return 0 to signal success */
}