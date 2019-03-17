#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "imath/imrat.h"
#include "imath/imath.h"
#include "coeffs_to_key.h"

#define FAIL(fmt, ...) do { if (1) { printf("%s:%d FATAL: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} } while (0)


// M[mul][mul+1] -> M[x][y]
static mp_rat get_xy(mp_rat arr, int mul, int x, int y)
{
    return arr + ((mul+1) * (y-1)) + (x-1);
}

// hash2: mul rows of mul+1 values
static mp_rat get_row(mp_rat arr, int mul, int x)
{
    return arr + ((mul+1) * (x-1));
}


static void print_rat(const char* tag, mp_rat rat)
{
    char tmp[256];
    mp_rat_to_string(rat, 16, tmp, sizeof(tmp));
    printf("[RAT] %s = %s\n", tag, tmp);
}

static void print_matrix(mp_rat M, int mul)
{
    char tmp[256];

    for (int row = 0; row < mul; ++row) {
        printf("[");
        for (int col = 0; col < mul + 1; ++col) {
            mp_rat val = M + ((mul+1) * row) + col;
            mp_rat_to_string(val, 16, tmp, sizeof(tmp));
            printf(" %-60s", tmp);
        }
        printf(" ]\n");
    }
}

// input data in_arr[mul] = {a_i, b_i}
// output: array[mul][mul+1], each row = [ a_i^(mul-1), a_i^(mul-2), ..., a_i, 1, b_i]
static mp_rat construct_matrix(strs_t* in_arr, int mul)
{
    // ARRAY[mul][mul+1]
    mp_rat rats = calloc(sizeof(mpq_t), (mul+1)*mul);
    mp_rat ratptr = rats;
    mpq_t tmp;

    mp_rat_init(&tmp);

    if (!rats) {
        FAIL("calloc(%zu, %u)", sizeof(mpq_t), (mul+1)*mul);
        return NULL;
    }

    // read data - in every loop add mul + 1 rats
    for (int i = 0; i < mul; ++i) {

        if (mp_rat_read_string(&tmp, 16, in_arr[i].str1) != MP_OK) {
            FAIL("mp_rat_read_string(in_arr[%d].str1)", i);
            return NULL;
        }

        for (int exp = mul - 1; exp >= 0; exp -= 1) {
            if (mp_rat_expt(&tmp, exp, ratptr) != MP_OK) {
                FAIL("mp_rat_expt");
                return NULL;
            }
            ratptr += 1;
        }

        if (mp_rat_read_string(ratptr, 16, in_arr[i].str2) != MP_OK) {
            FAIL("mp_rat_read_string(in_arr[%d].str2)", i);
            return NULL;
        }

        ratptr += 1;
    }

    mp_rat_clear(&tmp);
    return rats;
}

static void destroy_matrix(mp_rat rats, int mul)
{
    if (rats) {
        mp_rat ratptr = rats;
        int cnt = mul * (mul+1);

        while (cnt > 0) {
            mp_rat_clear(ratptr);
            ratptr++;
            cnt -= 1;
        }

        free(rats);
    }
}

// bring matrix [mul][mul+1] to triangular form
int matrix_make_triangular(mp_rat rats, int mul)
{
    mpq_t tmp1, tmp2, ratio;
    mp_rat_init(&tmp1);
    mp_rat_init(&tmp2);
    mp_rat_init(&ratio);

    for (int row = 1; row <= mul; ++row) {
        // checking if diagonal is non-zero
        if (mp_rat_compare_zero(get_xy(rats, mul, row, row)) == 0) { // is 0
            printf("compare 0 is TRUE for (%u, %u)\n", row, row);
            int row2 = row;
            // find row below which have this column non-zero
            int ret = 0;
            while (ret == 0) {
                if (row2 > mul) {
                    FAIL("failed to make diagonal non-zero for row=%d", row);
                    return -1;
                }

                row2 += 1;
                ret = mp_rat_compare_zero(get_xy(rats, mul, row, row2));
            }

            printf("compare_zero changing rows: %d <=> %d\n", row, row2);
            mp_rat r1 = get_row(rats, mul, row);
            mp_rat r2 = get_row(rats, mul, row2);

            // changing matrix rows
            for (int to_swap = mul + 1; to_swap > 0; to_swap -= 1) {
                mpq_t tmp;
                memcpy(&tmp, r1, sizeof(mpq_t));
                memcpy(r1, r2, sizeof(mpq_t));
                memcpy(r2, &tmp, sizeof(mpq_t));

                r1 += 1;
                r2 += 1;
            }

        }

        // covert matrix to triangle form - nothing to do for last row
        if (row < mul) {
            mp_rat A_xx = get_xy(rats, mul, row, row);
            for (int y = row + 1; y <= mul; ++y) {
                mp_rat A_xy = get_xy(rats, mul, row, y);
                if (mp_rat_compare_zero(A_xy) != 0) {
                    //print_rat("A_xx", A_xx);
                    //print_rat("A_xy", A_xy);
                    if (mp_rat_div(A_xx, A_xy, &ratio) != MP_OK) {
                        FAIL("A_xx / A_xy => RATIO");
                        return -1;
                    }

                    //print_rat("  ==", ratio);

                    // multiply whole row by ratio and substract xrow
                    mp_rat A_xrow = A_xx;
                    mp_rat A_yrow = A_xy;
                    for (int xrest = row; xrest <= mul + 1; ++xrest) {
                        mp_rat_mul(A_yrow, &ratio, &tmp1);
                        mp_rat_sub(&tmp1, A_xrow, &tmp2);
                        mp_rat_copy(&tmp2, A_yrow);

                        A_xrow += 1;
                        A_yrow += 1;
                    };
                }
            }
        } // else x == mul -> nothing to do
    }

    mp_rat_clear(&tmp1);
    mp_rat_clear(&tmp2);
    mp_rat_clear(&ratio);
    return 0; // success
}

#define FAIL_RET(_retcode, ...) do { FAIL(__VA_ARGS__); ret = _retcode; goto exit; } while(0)

// mul is the amount of rows in array
int coeffs_to_key(char **result, strs_t* in_arr, int mul)
{
    int ret = 1;

#if 0 // debug: print input values
    for (int i = 0; i < mul; ++i) {
        printf("[%i].0 %s\n", i, in_arr[i].str1);
        printf("[%i].1 %s\n", i, in_arr[i].str2);
    }
#endif

    mp_rat rats = construct_matrix(in_arr, mul);
    if (rats == NULL)
        FAIL_RET(-1, "construct_matrix");

    print_matrix(rats, mul);

    if (matrix_make_triangular(rats, mul) < 0)
        FAIL_RET(-1, "matrix_make_triangular");

    // matrix is now in triangular form
    print_matrix(rats, mul);

    // compute final result: res = (A[mul][mul+1] / A[mul][mul])
    mpq_t res;
    mp_rat_init(&res);
    mp_rat A_mm = get_xy(rats, mul, mul, mul); // is this last val?
    if (mp_rat_compare_zero(A_mm) != 0) {
        mp_rat A_m_1 = get_xy(rats, mul, mul + 1, mul);
        if (mp_rat_div(A_m_1, A_mm, &res) != MP_OK)
            FAIL_RET(-1, "final mp_rat_div");

        mp_rat_reduce(&res);
        print_rat("RESULT red" , &res);
        if (mp_int_compare_value(&res.den, 1) == 0) { // denominator == 1
            int nemb = mp_int_string_len(&res.num, 16);
            char* str = calloc(nemb, 1);
            *result = str;
            ret = 1;
            if (str == NULL)
                FAIL_RET(1, "failed to allocate result string");

            mp_int_to_string(&res.num, 16, str, 128); // FIXME 128
            ret = 0;
        } else {
            FAIL_RET(0xb, "result denominator is not 1");
        }
    } else {
        FAIL_RET(0xb, "final A_mm is zero");
    }


exit:
    mp_rat_clear(&res);
    destroy_matrix(rats, mul);

#if 0
    if (*result)
        printf("RESULT: %s\n", *result);
    printf(" => %d\n", ret);
#endif

    return ret;
}

