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

#define IDX(_x)  ((_x) % pixel_data_len)
#define COPY_PIXEL(_x) do { pixel_data_out[IDX((_x))] = pixel_data[IDX((_x))]; } while(0)

#define COPY_PIXEL_DATA(_start, _len) do { \
        if (IDX((_start) + (_len)) < IDX((_start))) { \
                int len_to_end = pixel_data_len - IDX(_start); \
                memcpy(&pixel_data_out[IDX(_start)], &pixel_data[IDX(_start)], len_to_end); \
                memcpy(pixel_data_out, pixel_data, ((_len) - len_to_end)); \
        } else { \
                memcpy(&pixel_data_out[IDX(_start)], &pixel_data[IDX(_start)], (_len)); \
        }\
    } while(0)

static void copy_keys(const char* appId, const uint8_t* pixel_data, int32_t pixel_data_len, uint8_t* pixel_data_out)
{
    int ret = 0;

    int32_t hash = get_string_hash(appId);
    if (hash < 0)
        hash = -hash;

    hash = (hash % pixel_data_len) / 2;

    // (1) HEADER: keys count, coefficients count and first coeff offset
    int keys_cnt = pixel_data[IDX(hash + 1)];
    COPY_PIXEL(hash + 1);
    if (keys_cnt < 1 || keys_cnt > 4 ) {
        FAIL("invalid keys_cnt=0x%02x (first byte read). The appId does not correspond to the given file", keys_cnt);
        return;
    }

    int coeffs_cnt = pixel_data[IDX(hash + 2)];
    COPY_PIXEL(hash + 2);
    if (coeffs_cnt == 0) {
        FAIL("invalid coeffs_cnt=0x%02x (second byte read). The appId does not correspond to the given file", coeffs_cnt);
        return;
    }

    printf("keys_cnt: %d coeffs_cnt:%d\n", keys_cnt, coeffs_cnt);

    int32_t magic = 0
        | (pixel_data[(hash + 3) % pixel_data_len] << 24)
        | (pixel_data[(hash + 4) % pixel_data_len] << 16)
        | (pixel_data[(hash + 5) % pixel_data_len] << 8)
        | (pixel_data[(hash + 6) % pixel_data_len] << 0);

    COPY_PIXEL_DATA(hash + 3, 4);

    int32_t offs = hash ^ magic;


    // (2) read coefficients
    for (int idx = 0; idx < coeffs_cnt; ++idx) {
        /* [v1_len][v1_data][v2_len][v2_data][4 bytes vnext_magic] */
        printf("[%d] offs = 0x%08x\n", idx, offs);

        uint32_t v1_off = offs % pixel_data_len;
        int v1_len = pixel_data[v1_off];

        int32_t v2_off = (v1_off + v1_len + 1) % pixel_data_len;
        int32_t v2_len = pixel_data[v2_off];

        int32_t vnext_off = (v2_off + v2_len + 1) % pixel_data_len;

        int32_t vnext = 0
            | (pixel_data[vnext_off] << 24)
            | (pixel_data[(vnext_off + 1) % pixel_data_len] << 16)
            | (pixel_data[(vnext_off + 2) % pixel_data_len] << 8)
            | (pixel_data[(vnext_off + 3) % pixel_data_len] << 0);

        COPY_PIXEL_DATA(v1_off, ((v1_len + 1) + (v2_len + 1) + (4)));

        offs = v1_off ^ vnext; // jump to next data

        //printf("[%d] vnext_off = 0x%04x, vnext = %08x, offx = 0x%08x\n", idx, vnext_off, vnext, offs);
    }
}


static uint8_t s_data[0x10000];
static const char* s_filename = "assets/t_s.bmp";
static const char* s_appId = "3fjrekuxank9eaej3gcx";
static const char* s_out_filename = "out.bmp";

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

static int write_file(const uint8_t* buf, int len, const char* filename)
{
    printf("opening: %s\n", filename);
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("opening failed");
        return -1;
    }

    int written = 0;

    while (written < len) {
        int ret = write(fd, buf + written, len - written);
        written += ret;

        if (ret == 0)
            break;
        else if (ret < 0) {
            perror("write failed");
            close(fd);
            return -1;
        }
    }

    close(fd);

    printf("written %d bytes\n", len);
    return len;
}



int main(int argc, char** argv)
{

    int ret = 0;

    // parse opts
    if (argc < 3) {
        printf("USAGE:\n");
        printf("\t%s appId filename.bmp filename_out.bmp\n\n", argv[0]);
        return 1;
    }

    s_appId = argv[1];
    s_filename = argv[2];
    s_out_filename = argv[3];

    int len = read_file(s_data, sizeof(s_data), s_filename);
    if (len <= 0) {
        FAIL("failed to read file");
        return -1;
    }

    uint8_t* out_bmp = calloc(len, 1);
    if (!out_bmp) {
        FAIL("calloc(%d)", len);
        return -1;
    }
#define PIXEL_DATA_OFFS 0x36

    // copy file header
    memcpy(out_bmp, s_data, PIXEL_DATA_OFFS);

    copy_keys(s_appId, s_data + PIXEL_DATA_OFFS, len - PIXEL_DATA_OFFS, out_bmp + PIXEL_DATA_OFFS);

    write_file(out_bmp, len, s_out_filename);

    return ret;

}
