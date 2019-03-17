#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "coeffs_to_key.h"


#define FAIL(fmt, ...) do { if (1) { printf("%s:%d FATAL: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} } while (0)

static uint32_t get_string_hash(const char* str)
{
    int len = strlen(str);
    uint32_t acc = 0;

    while (len > 0) { // do not include '\0'
        acc = (acc << 5) - acc;
        acc = acc + *str++;
        --len;
        //printf("[%c] str hash: %x\n", *c, acc);
    }

    printf("str hash: 0x%08x\n", acc);
    return acc;
}

static int hex_to_int(char a)
{
    if (a <= '9')
        return a - '0';
    return 10 + a - 'A';
}

static const char* hex_to_str(const char* hex)
{
    static char tmp[64];
    int xlen = 0;

    int len = strlen(hex);
    int i = 0;
    while (i + 1 < len) {
        tmp[xlen++] = hex_to_int(hex[i]) << 4 | hex_to_int(hex[i+1]);
        i += 2;
    }

    tmp[xlen] = '\0';
    return tmp;
}

static char* bytes_to_str(uint8_t* pixel_data, int32_t pixel_data_len, uint32_t cnt, int idx)
{
    //printf("%s(cnt=%u, offs=0x%08x)\n", __func__, cnt, idx);

    int nmemb = 2 * cnt + 1;
    char* mem = calloc(nmemb, sizeof(char));
    char* memcurr = mem;

    if (!mem)
        return NULL;

    while (cnt > 0) {
        uint8_t next_val = pixel_data[idx % pixel_data_len];
        memcurr += sprintf(memcurr, "%02x", next_val);

        cnt -= 1;
        idx += 1;
    }

    //printf("  result: %s\n", mem);
    return mem;
}

static int read_keys(const char *appId, uint8_t* pixel_data, int32_t pixel_data_len, char*** out_keys, int* out_keys_cnt)
{
    int ret = 0;

    int32_t hash = get_string_hash(appId);
    if (hash < 0)
        hash = -hash;

    hash = (hash % pixel_data_len) / 2;

    // (1) HEADER: keys count, coefficients count and first coeff offset
    int keys_cnt = pixel_data[(hash + 1) % pixel_data_len];
    if (keys_cnt < 1 || keys_cnt > 4 ) {
        FAIL("invalid keys_cnt=0x%02x (first byte read). The appId does not correspond to the given file", keys_cnt);
        return 0x15; // invalid signature
    }

    int coeffs_cnt = pixel_data[(hash + 2) % pixel_data_len];
    if (coeffs_cnt == 0) {
        FAIL("invalid coeffs_cnt=0x%02x (second byte read). The appId does not correspond to the given file", coeffs_cnt);
        return -1; // TODO goto lab_0x36e0_2;
    }

    printf("keys_cnt: %d coeffs_cnt:%d\n", keys_cnt, coeffs_cnt);
    *out_keys_cnt = keys_cnt;


    char** results = calloc(keys_cnt, sizeof(char*));
    if (!results) {
        FAIL("calloc(results)");
        return 1;
    }
    *out_keys = results;


    strs_t* coeffs = calloc(coeffs_cnt * keys_cnt, sizeof(strs_t));
    if (!coeffs) {
        FAIL("calloc(coeffs)");
        return 1; // results will be cleared by the caller
    }

    int32_t magic = 0
        | (pixel_data[(hash + 3) % pixel_data_len] << 24)
        | (pixel_data[(hash + 4) % pixel_data_len] << 16)
        | (pixel_data[(hash + 5) % pixel_data_len] << 8)
        | (pixel_data[(hash + 6) % pixel_data_len] << 0);


    int32_t offs = hash ^ magic;


    // (2) read coefficients
    for (int idx = 0; idx < coeffs_cnt; ++idx) {
        /* [v1_len][v1_data][v2_len][v2_data][4 bytes vnext_magic] */
        printf("[%d] offs = 0x%08x\n", idx, offs);

        uint32_t v1_off = offs % pixel_data_len;
        int v1_len = pixel_data[v1_off];
        coeffs[idx].str1 = bytes_to_str(pixel_data, pixel_data_len, v1_len, v1_off + 1);


        if (coeffs[idx].str1 == NULL) {
            FAIL("cannot allocate string for next value (idx=%d, len=%d)", idx, v1_len);
            ret = -1;
            goto exit;
        }

        int32_t v2_off = (v1_off + v1_len + 1) % pixel_data_len;
        int32_t v2_len = pixel_data[v2_off];

        coeffs[idx].str2 = bytes_to_str(pixel_data, pixel_data_len, v2_len, v2_off + 1);

        if (coeffs[idx].str2 == NULL) {
            FAIL("cannot allocate string for next value (idx=%d, len=%d)", idx, v1_len);
            ret = -1;
            goto exit;
        }

        int32_t vnext_off = (v2_off + v2_len + 1) % pixel_data_len;

        int32_t vnext = 0
            | (pixel_data[vnext_off] << 24)
            | (pixel_data[(vnext_off + 1) % pixel_data_len] << 16)
            | (pixel_data[(vnext_off + 2) % pixel_data_len] << 8)
            | (pixel_data[(vnext_off + 3) % pixel_data_len] << 0);

        offs = v1_off ^ vnext; // jump to next data

        //printf("[%d] vnext_off = 0x%04x, vnext = %08x, offx = 0x%08x\n", idx, vnext_off, vnext, offs);
    }

    // compute each key using provided coeffs
    for (int idx = 0; idx < keys_cnt; ++idx) {
        int res = coeffs_to_key(&results[idx], &coeffs[idx * coeffs_cnt], coeffs_cnt);
        if (res != 0) {
            FAIL("coeffs_to_key(idx=%d) == %d", idx, res);
            ret = 0xb;
        }
    }

exit:

    if (coeffs != NULL) {
        for (int i = 0; i < coeffs_cnt * keys_cnt; ++i) {
            if (coeffs[i].str1 != NULL)
                free(coeffs[i].str1);

            if (coeffs[i].str2 != NULL)
                free(coeffs[i].str2);
        }

        free(coeffs);
    }

    // NOTE: not cleaning results - this has to be done by the caller
    return ret;
}


static uint8_t s_data[0x10000];
static const char* s_filename = "assets/t_s.bmp";
static const char* s_appId = "3fjrekuxank9eaej3gcx";

static int read_file(uint8_t* buf, int max_len, const char* filename)
{
    printf("opening: %s\n", filename);
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("opening failed");
        return -1;
    }

    int len = 0;

    while (1) {
        int ret = read(fd, buf + len, max_len - len);
        len += ret;

        if (ret == 0)
            break;
        else if (ret < 0) {
            perror("read failed");
            close(fd);
            return -1;
        }
    }

    close(fd);

    printf("read %d bytes\n", len);
    return len;
}


int main(int argc, char** argv)
{

    int ret = 0;

    // parse opts
    if (argc < 2) {
        printf("USAGE:\n");
        printf("\t%s appId filename.bmp\n\n", argv[0]);
        return 1;
    }

    s_appId = argv[1];
    s_filename = argv[2];

    int len = read_file(s_data, sizeof(s_data), s_filename);
    if (len <= 0) {
        FAIL("failed to read file");
        return -1;
    }

#define PIXEL_DATA_OFFS 0x36
    char** keys_arr = NULL;
    int keys_cnt = 0;
    int res = read_keys(s_appId, s_data + PIXEL_DATA_OFFS, len - PIXEL_DATA_OFFS, &keys_arr, &keys_cnt);

    if (res != 0) {
        FAIL("read_keys failed (res=%d)", res);
        ret = 1;
        goto exit;
    }

    // print result keys
    for (int i = 0; i < keys_cnt; ++i) {
        printf("[KEY][%d] hex: %s\n", i, keys_arr[i]);
        printf("[KEY][%d] str: %s\n", i, hex_to_str(keys_arr[i]));
    }

// RESULT: "76617939673539673967393971663372747170746D6333656D686B616E776B78"
// hex-to-str: "vay9g59g9g99qf3rtqptmc3emhkanwkx"

exit:
    if (keys_arr != NULL) {
        for (int i = 0; i < keys_cnt; ++i) {
            if (keys_arr[i] != NULL)
                free(keys_arr[i]);
        }

        free(keys_arr);
    }

    return ret;

}
