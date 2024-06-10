#include <hpdf.h>
#include <stdio.h>
#include <stdlib.h>

// functia creeaza un pdf nou cu textul specificat
void create_text_pdf(const char *new_pdf_filename, const char *text) {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    HPDF_Page page = HPDF_AddPage(pdf);

    // setam fontul si dimensiunea textului
    HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 50, 750, text);
    HPDF_Page_EndText(page);

    // salvam pdf-ul in fisierul specificat
    HPDF_SaveToFile(pdf, new_pdf_filename);
    HPDF_Free(pdf);
}

// functia adauga text intr-un pdf existent
void add_text_to_pdf(const char *filename, const char *text) {
    // cream un pdf nou cu textul specificat
    const char *temp_pdf = "temp_text.pdf";
    create_text_pdf(temp_pdf, text);

    // cream comanda pentru a imbina pdf-urile folosind pdftk
    char command[512];
    snprintf(command, sizeof(command), "pdftk %s stamp %s output %s", filename, temp_pdf, "merged_output.pdf");

    // executam comanda
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "error: failed to merge pdfs using pdftk\n");
    } else {
        // inlocuim fisierul original cu fisierul imbibat
        if (rename("merged_output.pdf", filename) != 0) {
            perror("error: unable to replace the original pdf");
        }
    }

    // stergem fisierul temporar
    remove(temp_pdf);
}

// functia creeaza un pdf nou cu o imagine specificata
void create_image_pdf(const char *new_pdf_filename, const char *image_filename) {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    HPDF_Page page = HPDF_AddPage(pdf);

    // incarcam imaginea din fisierul specificat
    HPDF_Image image = HPDF_LoadJpegImageFromFile(pdf, image_filename);
    float iw = HPDF_Image_GetWidth(image);
    float ih = HPDF_Image_GetHeight(image);

    // calculam raportul de aspect al imaginii
    float aspect_ratio = iw / ih;

    // alegem dimensiunile paginii pdf
    float page_width = 600.0f; // latimea paginii
    float page_height = page_width / aspect_ratio; // inaltimea paginii, pentru a pastra proportiile

    // setam dimensiunile paginii
    HPDF_Page_SetWidth(page, page_width);
    HPDF_Page_SetHeight(page, page_height);

    // calculam pozitia si dimensiunile imaginii pe pagina
    float image_x = (page_width - iw) / 2;
    float image_y = (page_height - ih) / 2;

    // desenam imaginea pe pagina
    HPDF_Page_DrawImage(page, image, image_x, image_y, iw, ih);

    // salvam pdf-ul in fisierul specificat
    HPDF_SaveToFile(pdf, new_pdf_filename);
    HPDF_Free(pdf);
}

// functia imbrina doua pdf-uri folosind pdftk
void merge_pdfs(const char *filename, const char *temp_pdf) {
    char command[512];
    snprintf(command, sizeof(command), "pdftk %s %s cat output %s", filename, temp_pdf, "merged_output.pdf");

    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "error: failed to merge pdfs using pdftk\n");
    } else {
        if (rename("merged_output.pdf", filename) != 0) {
            perror("error: unable to replace the original pdf");
        }
    }
}

// functia adauga o imagine intr-un pdf existent
void add_image_to_pdf(const char *filename, const char *image_filename) {
    const char *temp_pdf = "temp_image.pdf";
    create_image_pdf(temp_pdf, image_filename);
    merge_pdfs(filename, temp_pdf);
    remove(temp_pdf);
}