#include <stdio.h>

int main(void) {
	FILE *fp = tmpfile();
	if (fp) fprintf(fp, "Hello temporary file\n");
	fsync(fp);
	rewind(fp);
	pause(); // wait for signal
	return 0;
}
