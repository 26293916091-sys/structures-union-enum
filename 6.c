/*
 * Problem 3 (Extended): Memory-Efficient Typed Records and Safe Union Access
 * File: 6.c
 * Standard: ISO C11
 * Compilation: gcc -std=c11 -Wall -Wextra -Wpedantic 6.c -o 6
 *
 * Description:
 *   Analyzes the memory layout, size, alignment (alignof), field offsets
 *   (offsetof), and member addresses of StructData and UnionData, comparing
 *   memory usage across 100 records. Implements a type-safe TaggedRecord
 *   combining UnionData with a ValueType enum tag to prevent reading stale
 *   or inactive union members.
 *
 * Sample Run:
 *   1. Prints sizeof, alignof, field offsets, member addresses, and 100-record
 *      memory footprint for StructData, UnionData, and TaggedRecord.
 *   2. Populates and safely displays 5 TaggedRecord instances holding:
 *      INT: 10
 *      FLOAT: 220.50
 *      TEXT: "Union Programming"
 *      INT: -2026
 *      TEXT: "C11 Tagged Union"
 *   3. Runs edge-case tests and prints written memory-layout observations.
 *
 * Edge Cases Tested:
 *   - Maximum-length valid string (19 characters + null terminator = 20 bytes)
 *   - Overlength string (>= 20 bytes) safely rejected without truncation
 *   - Switching a single record across types: INT -> TEXT -> FLOAT
 *   - Invalid/uninitialized tag handling in print_tagged_record
 *   - Platform-independent sizeof and alignof reporting
 */

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define TEXT_BUFFER_SIZE 20
#define RECORD_COUNT 5
#define BULK_COUNT 100

/* Union where all members share the exact same starting memory address */
union UnionData {
    int i;
    float f;
    char str[TEXT_BUFFER_SIZE];
};

/* Structure where each member occupies its own distinct memory offset */
struct StructData {
    int i;
    float f;
    char str[TEXT_BUFFER_SIZE];
};

/* Tag enumeration tracking which member of UnionData is currently active */
typedef enum {
    VALUE_INVALID = 0,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_TEXT
} ValueType;

/* Safe tagged-union container pairing a type tag with the union payload */
typedef struct {
    ValueType tag;
    union UnionData value;
} TaggedRecord;

/* Inspects and prints sizes, alignments, offsets, addresses, and 100-record memory usage */
static void analyze_memory_layout(void) {
    struct StructData s_demo = {10, 220.5f, "Struct Demo"};
    union UnionData u_demo;
    memset(&u_demo, 0, sizeof(u_demo));

    printf("=== 1. Memory Layout, Size, Alignment & Offsets ===\n");
    printf("StructData -> sizeof: %zu bytes | alignof: %zu bytes\n",
           sizeof(struct StructData), alignof(struct StructData));
    printf("UnionData  -> sizeof: %zu bytes | alignof: %zu bytes\n",
           sizeof(union UnionData), alignof(union UnionData));
    printf("TaggedRec  -> sizeof: %zu bytes | alignof: %zu bytes\n\n",
           sizeof(TaggedRecord), alignof(TaggedRecord));

    printf("StructData Field Offsets (via offsetof) and Addresses:\n");
    printf("  i   : offset = %2zu bytes | address = %p\n",
           offsetof(struct StructData, i), (void *)&s_demo.i);
    printf("  f   : offset = %2zu bytes | address = %p\n",
           offsetof(struct StructData, f), (void *)&s_demo.f);
    printf("  str : offset = %2zu bytes | address = %p\n\n",
           offsetof(struct StructData, str), (void *)&s_demo.str);

    printf("UnionData Member Addresses (all overlap at offset 0):\n");
    printf("  i   : address = %p\n", (void *)&u_demo.i);
    printf("  f   : address = %p\n", (void *)&u_demo.f);
    printf("  str : address = %p\n\n", (void *)&u_demo.str);

    size_t struct_100 = BULK_COUNT * sizeof(struct StructData);
    size_t union_100 = BULK_COUNT * sizeof(union UnionData);
    size_t tagged_100 = BULK_COUNT * sizeof(TaggedRecord);

    printf("Memory Comparison for %d Records:\n", BULK_COUNT);
    printf("  %d x StructData   : %zu bytes\n", BULK_COUNT, struct_100);
    printf("  %d x UnionData    : %zu bytes\n", BULK_COUNT, union_100);
    printf("  %d x TaggedRecord : %zu bytes\n\n", BULK_COUNT, tagged_100);
}

/* Safely stores an integer and updates the active tag to VALUE_INT */
static int set_record_int(TaggedRecord *rec, int val) {
    if (rec == NULL) {
        return -1;
    }

    rec->value.i = val;
    rec->tag = VALUE_INT;

    return 0;
}

/* Safely stores a float and updates the active tag to VALUE_FLOAT */
static int set_record_float(TaggedRecord *rec, float val) {
    if (rec == NULL) {
        return -1;
    }

    rec->value.f = val;
    rec->tag = VALUE_FLOAT;

    return 0;
}

/* Validates string length (< 20 bytes) before copying and updating tag to VALUE_TEXT */
static int set_record_text(TaggedRecord *rec, const char *text) {
    if (rec == NULL || text == NULL) {
        return -1;
    }

    size_t len = strlen(text);

    if (len >= TEXT_BUFFER_SIZE) {
        return -1;
    }

    memcpy(rec->value.str, text, len + 1);
    rec->tag = VALUE_TEXT;

    return 0;
}

/* Reads and prints ONLY the active union member indicated by rec->tag */
static int print_tagged_record(const TaggedRecord *rec, size_t index) {
    if (rec == NULL) {
        return -1;
    }

    printf("Record [%zu] -> ", index);

    switch (rec->tag) {
        case VALUE_INT: {
            printf("Type: INT   | Value: %d\n", rec->value.i);
            break;
        }
        case VALUE_FLOAT: {
            printf("Type: FLOAT | Value: %.2f\n", (double)rec->value.f);
            break;
        }
        case VALUE_TEXT: {
            printf("Type: TEXT  | Value: \"%s\"\n", rec->value.str);
            break;
        }
        default: {
            printf("Type: INVALID/UNINITIALIZED (tag=%d) - unsafe read prevented.\n",
                   (int)rec->tag);
            return -1;
        }
    }

    return 0;
}

/* Exercises invalid tags, type transitions, max-length strings, and overlength rejection */
static void run_edge_case_tests(void) {
    printf("\n=== 3. Edge-Case Verification ===\n");

    TaggedRecord test_rec = {VALUE_INVALID, {0}};

    /* Test 1: Uninitialized/invalid tag guard */
    printf("[Test 1] Uninitialized tag check:\n  ");
    print_tagged_record(&test_rec, 99);

    /* Test 2: Switching a single record across types: INT -> TEXT -> FLOAT */
    printf("[Test 2] Switching a single record across types:\n  ");
    set_record_int(&test_rec, 42);
    print_tagged_record(&test_rec, 99);

    printf("  ");
    set_record_text(&test_rec, "Safe Switch");
    print_tagged_record(&test_rec, 99);

    printf("  ");
    set_record_float(&test_rec, 98.6f);
    print_tagged_record(&test_rec, 99);

    /* Test 3: Maximum-length valid string (19 chars + '\0' = 20 bytes) */
    const char *exact_19 = "1234567890123456789";
    printf("[Test 3] Max-length string (%zu chars): ", strlen(exact_19));
    if (set_record_text(&test_rec, exact_19) == 0) {
        printf("Accepted -> \"%s\"\n", test_rec.value.str);
    } else {
        printf("Unexpected failure.\n");
    }

    /* Test 4: Overlength string (20 chars, needs 21 bytes with '\0') */
    const char *overlength_20 = "12345678901234567890";
    printf("[Test 4] Overlength string (%zu chars): ", strlen(overlength_20));
    if (set_record_text(&test_rec, overlength_20) != 0) {
        printf("Rejected safely (previous valid value \"%s\" preserved).\n",
               test_rec.value.str);
    } else {
        printf("Unexpected acceptance.\n");
    }
}

/* Prints written summary explaining memory savings vs mutual exclusion in unions */
static void print_observations(void) {
    printf("\n=== 4. Written Observation on Memory & Safety ===\n");
    printf("1. In StructData, each member has its own distinct offset and address, allowing\n");
    printf("   all three values to coexist simultaneously at the cost of sum-of-sizes plus\n");
    printf("   alignment padding.\n");
    printf("2. In UnionData, all members start at offset 0 (identical memory address). Its size\n");
    printf("   is determined by its largest member (20 bytes) padded to the strictest member\n");
    printf("   alignment (4 bytes), saving memory when a record holds only one type at a time.\n");
    printf("3. Because union members overlap, reading an inactive member reads stale or\n");
    printf("   reinterpreted bytes. Pairing the union with a ValueType tag in TaggedRecord\n");
    printf("   adds minimal overhead while guaranteeing type-safe access.\n");
}

int main(void) {
    analyze_memory_layout();

    printf("=== 2. Safe Tagged Records Demonstration ===\n");
    TaggedRecord records[RECORD_COUNT];
    memset(records, 0, sizeof(records));

    set_record_int(&records, 10);
    set_record_float(&records, 220.5f);
    set_record_text(&records, "Union Programming");
    set_record_int(&records, -2026);
    set_record_text(&records, "C11 Tagged Union");

    for (size_t i = 0; i < RECORD_COUNT; i++) {
        print_tagged_record(&records[i], i + 1);
    }

    run_edge_case_tests();
    print_observations();

    return 0;
}
