#!/bin/sh
gcc -g -O2 -o ./read_keys read_keys.c coeffs_to_key.c imath/imath.c imath/imrat.c
gcc -g -O2 -o ./extract_used_pixels ./extract_used_pixels.c
