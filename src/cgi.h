#ifndef _CGI_H_
#define _CGI_H_

int cgiInit(const char *cgiDataDir);
void cgiSetFromArgs(int argc, char **argv);

void cgiStartHTML(const char *title);
void cgiEndHTML(void);
void cgiStartCSV(const char *filename);
void cgiError(const char *fmt, ...);

void cgiCopyTemplate(const char *tmplname); // lookup("LANG")
void cgiCopyTemplateLang(const char *tmplname, const char *lang);
void cgiCopyVerbatim(const char *filename, const char *mimetype);

char *cgiPathPrefix(const char *path, const char *prefix);
char *cgiGuessMimeType(const char *fn);
char *cgiTemplateBase(void);

#endif // _CGI_H_
