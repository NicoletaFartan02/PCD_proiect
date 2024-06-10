#ifndef CONVERT_UTILS_H
#define CONVERT_UTILS_H

// functia converteste un fisier pdf in docx
void pdf_to_docx(const char *pdf_filename, const char *docx_filename);

// functia converteste un fisier docx in pdf
void docx_to_pdf(const char *docx_filename, const char *pdf_filename);

// functia converteste un fisier pdf in rtf
void pdf_to_rtf(const char *pdf_filename, const char *rtf_filename);

// functia converteste un fisier pdf in html
void pdf_to_html(const char *pdf_filename, const char *html_filename);

#endif // CONVERT_UTILS_H