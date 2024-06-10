#ifndef DOCX_UTILS_H
#define DOCX_UTILS_H

// functia converteste un fisier pdf in docx
void pdf_to_docx(const char *pdf_filename, const char *docx_filename);

// functia converteste un fisier docx in pdf
void docx_to_pdf(const char *docx_filename, const char *pdf_filename);

#endif // DOCX_UTILS_H