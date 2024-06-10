#ifndef CONVERT_UTILS_H
#define CONVERT_UTILS_H

void convert_pdf_to_docx(const char *input_path, const char *output_path) ;
void convert_docx_to_pdf(const char *input_path, const char *output_path);
void convert_pdf_to_rtf(const char *input_path, const char *output_path);
void convert_pdf_to_html(const char *input_path, const char *output_path);
void convert_pdf_to_odt(const char *input_path, const char *output_path);
void convert_odt_to_pdf(const char *input_path, const char *output_path);
void convert_odt_to_txt(const char *input_path, const char *output_path);
void convert_txt_to_odt(const char *input_path, const char *output_path);
void convert_txt_to_pdf(const char *input_path, const char *output_path);
void convert_docx_to_rtf(const char *input_path, const char *output_path);
void convert_rtf_to_docx(const char *input_path, const char *output_path);
void convert_rtf_to_pdf(const char *input_path, const char *output_path);


#endif // CONVERT_UTILS_H