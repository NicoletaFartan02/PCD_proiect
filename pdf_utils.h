#ifndef PDF_UTILS_H
#define PDF_UTILS_H

// functia adauga text intr-un fisier pdf
void add_text_to_pdf(const char *filename, const char *text);

// functia adauga o imagine intr-un fisier pdf
void add_image_to_pdf(const char *filename, const char *image_filename);

#endif // PDF_UTILS_H