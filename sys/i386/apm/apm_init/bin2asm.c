/*
 * APM (Advanced Power Management) BIOS Device Driver
 *
 * Copyright (c) 1994-1995 by HOSOKAWA, Tatsumi <hosokawa@jp.FreeBSD.org>
 *
 * This software may be used, modified, copied, and distributed, in
 * both source and binary form provided that the above copyright and
 * these terms are retained. Under no circumstances is the author
 * responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its
 * use.
 *
 * Sep., 1994	Implemented on FreeBSD 1.1.5.1R (Toshiba AVS001WD)
 *
 * $FreeBSD$
 */

#include <stdio.h>

#define NCOLS		8

int main(int argc, char *argv[])
{
	int  c, col, lastline;
	FILE *infile, *outfile;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
		exit(1);
	}

	if ((infile = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "Can't open %s.\n", argv[1]);
		exit(1);
	}

	if ((outfile = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "Can't open %s.\n", argv[2]);
		exit(1);
	}

	col = 0;

	fprintf(outfile, "/* This file is automatically generated by bin2asm */\n");
	fprintf(outfile, "/* Original file is '%s' */\n\n", argv[1]);

	lastline = 0;

	while ((c = fgetc(infile)) != EOF) {
		if (col % NCOLS == 0) {
			fprintf(outfile, "\t.byte\t");
		}
		fprintf(outfile, "0x%02x", c);
		if (col % NCOLS == NCOLS - 1) {
			fprintf(outfile, "\n");
			lastline = 1;
		}
		else {
			fprintf(outfile, ", ");
			lastline = 0;
		}
		col++;
	}
	if (!lastline) {
		do {
			fprintf(outfile, "0x00, ");
		} while ((col++) % NCOLS < NCOLS - 2);
		fprintf(outfile, "0x00\n");
	}

	fprintf(outfile, "\n/* Total size = 0x%04x */\n", col);

	fclose(infile);
	fclose(outfile);
	return 0;
}
