struct strs_s {
    char* str1;
    char* str2;
};
typedef struct strs_s strs_t;

int coeffs_to_key(char **result, strs_t* in_arr, int mul);
