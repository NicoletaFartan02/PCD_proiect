#include "convert_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

// Function to check file existence and extension
int check_file(const char *input_path, const char *ext) {
    if (strrchr(input_path, '.') == NULL || strcmp(strrchr(input_path, '.'), ext) != 0) {
        fprintf(stderr, "Error: Input file %s is not %s\n", input_path, ext);
        return 0;
    }
    if (access(input_path, F_OK) != 0) {
        fprintf(stderr, "Error: Input file %s doesn't exist\n", input_path);
        return 0;
    }
    return 1;
}

// Generic conversion function using libreoffice
void convert_with_libreoffice(const char *input_path, const char *output_path, const char *format) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/libreoffice", "libreoffice", "--headless", "--convert-to", format, input_path, "--outdir", output_path, NULL);
        fprintf(stderr, "Error: execl failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        fprintf(stderr, "Error: fork failed: %s\n", strerror(errno));
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Successfully converted %s to %s.\n", input_path, output_path);
        } else {
            fprintf(stderr, "Error: Conversion failed.\n");
        }
    }
}

void convert_pdf_to_docx(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".pdf")) return;
    printf("Converting PDF to DOCX: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "docx:writer_pdf_Export");
}

void convert_docx_to_pdf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".docx")) return;
    printf("Converting DOCX to PDF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "pdf");
}

void convert_pdf_to_rtf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".pdf")) return;
    printf("Converting PDF to RTF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "rtf:writer_pdf_Export");
}

void convert_pdf_to_html(const char *input_path, const char *output_path) {
    printf("Converting PDF to HTML: %s to %s\n", input_path, output_path);

    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/libreoffice", "libreoffice", "--headless", "--convert-to", "html", input_path, "--outdir", output_path, NULL);
        fprintf(stderr, "Error: execl failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        fprintf(stderr, "Error: fork failed: %s\n", strerror(errno));
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Successfully converted PDF to HTML.\n");
        } else {
            fprintf(stderr, "Error: Conversion failed.\n");
        }
    }
}

void convert_pdf_to_odt(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".pdf")) return;
    printf("Converting PDF to ODT: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "odt:writer_pdf_Export");
}

void convert_odt_to_pdf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".odt")) return;
    printf("Converting ODT to PDF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "pdf");
}

void convert_odt_to_txt(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".odt")) return;
    printf("Converting ODT to TXT: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "txt");
}

void convert_txt_to_odt(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".txt")) return;
    printf("Converting TXT to ODT: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "odt");
}

void convert_txt_to_pdf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".txt")) return;
    printf("Converting TXT to PDF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "pdf:writer_pdf_Export");
}

void convert_docx_to_rtf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".docx")) return;
    printf("Converting DOCX to RTF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "rtf");
}

void convert_rtf_to_docx(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".rtf")) return;
    printf("Converting RTF to DOCX: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "docx");
}

void convert_rtf_to_pdf(const char *input_path, const char *output_path) {
    if (!check_file(input_path, ".rtf")) return;
    printf("Converting RTF to PDF: %s to %s\n", input_path, output_path);
    convert_with_libreoffice(input_path, output_path, "pdf");
}