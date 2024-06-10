#include "convert_utils.h"
#include <stdio.h>
#include <stdlib.h>

// functia converteste un fisier pdf in docx folosind libreoffice
void pdf_to_docx(const char *pdf_filename, const char *docx_filename) {
    char command[1024];
    snprintf(command, sizeof(command), "libreoffice --headless --convert-to docx:writer_pdf_Export \"%s\" --outdir .", pdf_filename);
    system(command);
    rename("output.docx", docx_filename);
}

// functia converteste un fisier docx in pdf folosind libreoffice
void docx_to_pdf(const char *docx_filename, const char *pdf_filename) {
    char command[1024];
    snprintf(command, sizeof(command), "libreoffice --headless --convert-to pdf \"%s\" --outdir .", docx_filename);
    system(command);
}

// functia converteste un fisier pdf in rtf folosind libreoffice
void pdf_to_rtf(const char *pdf_filename, const char *rtf_filename) {
    // construim comanda pentru conversia din pdf in rtf
    char command[1024];
    snprintf(command, sizeof(command), "libreoffice --headless --convert-to rtf:writer_pdf_Export \"%s\" --outdir .", pdf_filename);

    // executam comanda pentru conversie
    int status = system(command);

    // verificam daca conversia a fost realizata cu succes
    if (status == 0) {
        // daca conversia a fost reusita, redenumim fisierul rezultat la numele dorit pentru fisierul rtf
        rename("output.rtf", rtf_filename);
        printf("pdf converted to rtf successfully.\n");
    } else {
        // daca conversia a esuat, afisam un mesaj de eroare
        fprintf(stderr, "error: pdf to rtf conversion failed.\n");
    }
}

// functia converteste un fisier pdf in html folosind pdftohtml
void pdf_to_html(const char *pdf_filename, const char *html_filename) {
    char command[1024];
    snprintf(command, sizeof(command), "pdftohtml -s -i \"%s\" \"%s\"", pdf_filename, html_filename);
    int status = system(command);
    if (status == -1) {
        perror("error executing pdftohtml command");
        exit(EXIT_FAILURE);
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            fprintf(stderr, "pdftohtml command failed with exit status %d\n", exit_status);
            exit(EXIT_FAILURE);
        } else {
            printf("pdf converted to html successfully.\n");
        }
    }
}