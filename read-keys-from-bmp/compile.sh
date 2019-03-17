#!/bin/sh
exec gcc -g -o ./read_keys read_keys.c coeffs_to_key.c imath/imath.c imath/imrat.c
