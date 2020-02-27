#!/bin/bash

# batch convert pdf files in ./data to .txt files in the same director
# requires pdftotext, part of the xpdf package

# do nothing if empty, rather than error out
shopt -s nullglob

for f in $PWD/data/*.pdf;
	do
		if [ ! -f "${f/.pdf/.txt}" ]; then
			pdftotext -nopgbrk -layout -eol unix "$f" "${f/.pdf/.txt}"
		fi
	done
