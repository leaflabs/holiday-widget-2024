#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    This script will auto generate look up tables and the required
    #defines for a list of math functions. Ouptut is sent to stdout
    so forward the results to the correct directory.
*/

/*
    Struct for a new lookup table and function to generate.
    Note: the range (upper_bound - lower_bound) must be a power
    of 2.
*/
struct lut_entry {
    char *name;            // Name of function
    float lower_bound;     // Lower bound for range
    float upper_bound;     // Upper bound for range
    int num_entries;       // Number of entries in the table
    int entries_per_line;  // Number of entries on a line
};

void gen_lut_tables(struct lut_entry *entries, int size) {
    printf("#ifndef __LMATH_LUTS_H__\n");
    printf("#define __LMATH_LUTS_H__\n\n\n");

    printf("/*************************************\n");
    printf("\tThis is a generated file from\n");
    printf("\tscripts/lut_generator.c\n");
    printf("**************************************/\n\n\n");

    for (int i = 0; i < size; i++) {
        struct lut_entry entry = entries[i];

        /***** Process arguments *****/
        char *function_name = entry.name;
        float lower_bound = entry.lower_bound;
        float upper_bound = entry.upper_bound;
        int entries = entry.num_entries;
        int num_per_line = entry.entries_per_line;

        // Define our function pointer
        double (*function)(double) = NULL;

        // Find the correct function to work off of
        if (strcmp(function_name, "sqrt") == 0) {
            function = sqrt;
        } else if (strcmp(function_name, "sin") == 0) {
            function = sin;
        } else if (strcmp(function_name, "asin") == 0) {
            function = asin;
        } else if (strcmp(function_name, "cos") == 0) {
            function = cos;
        } else if (strcmp(function_name, "acos") == 0) {
            function = acos;
        } else if (strcmp(function_name, "tan") == 0) {
            function = tan;
        } else if (strcmp(function_name, "atan") == 0) {
            function = atan;
        } else {
            fprintf(stderr, "Function '%s' not supported\n", function_name);
            exit(1);
        }

        // Make sure the range is a power of 2
        int range = upper_bound - lower_bound;
        if (ceil(log2(range)) != floor(log2(range))) {
            fprintf(stderr, "Range must be a power of 2\n");
            exit(1);
        }

        /***** Generate defines *****/
        printf("#define %s_LOWER_BOUND %fk\n", function_name, lower_bound);
        printf("#define %s_UPPER_BOUND %fk\n", function_name, upper_bound);

        // Calculate how many places to shift to simulate a multiply by entries
        // and divide by range
        int sum_of_shifts =
            (int)log2(entries) - (int)log2(upper_bound - lower_bound);
        if (sum_of_shifts < 0) {
            fprintf(
                stderr,
                "The number of entries is too small relative to the range\n");
            exit(1);
        }
        printf("#define %s_SHIFT_VALUE %d\n\n", function_name, sum_of_shifts);

        /***** Generate look up table *****/
        int newline_counter = 0;

        printf("_Accum %s_lut[] = {\n\t", function_name);
        // Add 1 extra entry than necessary to avoid lerp going out of bounds
        for (int i = 0; i <= entries; i++) {
            // Linear interpolate between lower and upper bound values
            float t = (float)i / (float)entries;
            float val = (1 - t) * lower_bound + t * upper_bound;

            // Get the value from the function
            float result = function(val);
            printf("%fk, ", result);
            newline_counter++;
            if (newline_counter == num_per_line) {
                newline_counter = 0;
                printf("\n\t");
            }
        }
        printf("0.0k");
        printf("\n};\n\n");
    }

    printf("#endif /* __LMATH_LUTS_H__ */\n");
}

int main(int argc, char **argv) {
    // Specify all functions here
    struct lut_entry entries[] = {
        {"sqrt", 0, 8, 256, 10},  {"sin", -4, 4, 256, 10},
        {"cos", -4, 4, 256, 10},  {"tan", -4, 4, 256, 10},
        {"asin", -1, 1, 256, 10}, {"acos", -1, 1, 256, 10},
        {"atan", -1, 1, 256, 10}};

    int num_funcs = sizeof(entries) / sizeof(struct lut_entry);

    gen_lut_tables(entries, num_funcs);
}
