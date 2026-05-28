#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_FILE "/tmp/python_ver.log"
#define CMD_CHECK "which python3 > /dev/null 2>&1" 
#define CMD_VERSION "python3 --version 2>&1"

int main() {
    char version_buffer[128] = {0};

    // =========================
    // CASE 1: check existence
    // =========================
    int status = system(CMD_CHECK);

    if (status != 0) {
        fprintf(stderr, "CASE 1: Python 3 NOT FOUND\n");

        FILE *log_fp = fopen(LOG_FILE, "w");
        if (log_fp) {
            fprintf(log_fp, "CASE 1: Python 3 NOT FOUND\n");
            fclose(log_fp);
        }

        return 1;
    }

    // =========================
    // CASE 2 & 3: get version
    // =========================
    FILE *fp = popen(CMD_VERSION, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to run python version command\n");
        return 1;
    }

    if (fgets(version_buffer, sizeof(version_buffer), fp) == NULL) {
        fprintf(stderr, "Error: Could not read Python version output\n");
        pclose(fp);
        return 1;
    }

    pclose(fp);

    // remove newline
    version_buffer[strcspn(version_buffer, "\n")] = 0;

    printf("Detected: %s\n", version_buffer);

    // =========================
    // CASE 2: wrong version
    // =========================
    if (strstr(version_buffer, "Python 3.9") == NULL) {
        fprintf(stderr, "CASE 2: Python exists but NOT version 3.9\n");

        FILE *log_fp = fopen(LOG_FILE, "w");
        if (log_fp) {
            fprintf(log_fp, "CASE 2: Wrong Python version: %s\n", version_buffer);
            fclose(log_fp);
        }

        return 2;
    }

    // =========================
    // CASE 3: correct version
    // =========================
    printf("CASE 3: Python 3.9 OK\n");

    FILE *log_fp = fopen(LOG_FILE, "w");
    if (log_fp) {
        fprintf(log_fp, "CASE 3: Python 3.9 OK (%s)\n", version_buffer);
        fclose(log_fp);
    }

    return 0;
}