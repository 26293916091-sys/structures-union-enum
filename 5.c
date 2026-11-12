/*
 * Problem 2 (Extended): Rectangle Inventory and Geometry Analysis
 * File: 5.c
 * Standard: ISO C11
 * Compilation: gcc -std=c11 -Wall -Wextra -Wpedantic 5.c -o 5 -lm
 *
 * Description:
 *   Interactive geometry inventory program that dynamically allocates an array
 *   of Rectangle structures, validates finite positive dimensions, maintains
 *   area and perimeter across updates, detects squares using a floating-point
 *   epsilon tolerance, sorts by area ascending, filters by minimum area, and
 *   computes area summary statistics.
 *
 * Sample Run:
 *   Enter initial number of rectangles (1 - 1000): 3
 *   Rectangle 1 -> ID: 201, Width: 5.0, Height: 4.0
 *   Rectangle 2 -> ID: 202, Width: 6.5, Height: 3.5
 *   Rectangle 3 -> ID: 203, Width: 4.0, Height: 4.0
 *   Display All -> 201 (Non-Square), 202 (Non-Square), 203 (Square)
 *   Update ID 201 -> Width: 5.0, Height: 5.0 (Area=25.00, Perimeter=20.00, Square)
 *   Summary -> Largest: 201 (25.00), Smallest: 203 (16.00), Total: 63.75, Avg: 21.25
 *   Exit -> Frees all allocated memory cleanly.
 *
 * Edge Cases Tested:
 *   - Invalid initial count (<= 0 or > 1000)
 *   - Invalid dimensions (0, negative, NaN, inf)
 *   - Duplicate rectangle ID rejection
 *   - Single rectangle min/max/average calculation
 *   - Equal area tie-breaking by ascending rectangle ID
 *   - Non-integer square detection using relative tolerance (1e-5)
 *   - Empty list handling after deleting all rectangles
 *   - Dynamic array growth via realloc with overflow protection
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INITIAL_RECTANGLES 1000
#define SQUARE_EPSILON 1e-5f
#define INPUT_BUFFER_SIZE 256

/* Represents a geometric rectangle and its derived metrics */
typedef struct {
    int id;
    float width;
    float height;
    float area;
    float perimeter;
} Rectangle;

/* Dynamic array container tracking active count and allocated capacity */
typedef struct {
    Rectangle *items;
    size_t count;
    size_t capacity;
} RectangleInventory;

/* Flushes leftover characters from stdin when an input line exceeds buffer size */
static void clear_remaining_line(void) {
    int ch = 0;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

/* Reads a bounded line from stdin and trims leading/trailing whitespace */
static bool read_line_trimmed(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return false;
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return false;
    }

    size_t len = strlen(buffer);

    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    } else if (!feof(stdin)) {
        clear_remaining_line();
        return false;
    }

    size_t start = 0;

    while (start < len && isspace((unsigned char)buffer[start])) {
        start++;
    }

    size_t end = len;

    while (end > start && isspace((unsigned char)buffer[end - 1])) {
        end--;
    }

    size_t trimmed_len = end - start;

    if (start > 0 && trimmed_len > 0) {
        memmove(buffer, buffer + start, trimmed_len);
    }

    buffer[trimmed_len] = '\0';

    return true;
}

/* Prompts until a valid integer within [min_val, max_val] is entered */
static bool read_int_in_range(const char *prompt, int min_val, int max_val, int *out_val) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);

        if (!read_line_trimmed(buf, sizeof(buf))) {
            if (feof(stdin)) {
                return false;
            }
            printf("Error: Input line too long. Try again.\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("Error: Input cannot be empty.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        long val = strtol(buf, &endptr, 10);

        if (errno == ERANGE || *endptr != '\0' || val < min_val || val > max_val) {
            printf("Error: Enter a valid integer between %d and %d.\n", min_val, max_val);
            continue;
        }

        *out_val = (int)val;

        return true;
    }
}

/* Prompts for a finite floating-point value (positive only if require_positive is true) */
static bool read_finite_float(const char *prompt, bool require_positive, float *out_val) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);

        if (!read_line_trimmed(buf, sizeof(buf))) {
            if (feof(stdin)) {
                return false;
            }
            printf("Error: Input line too long. Try again.\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("Error: Input cannot be empty.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        float val = strtof(buf, &endptr);

        bool out_of_bounds = require_positive ? (val <= 0.0f) : (val < 0.0f);

        if (errno == ERANGE || *endptr != '\0' || !isfinite(val) || out_of_bounds) {
            printf("Error: Enter a valid finite %s number (NaN/inf rejected).\n",
                   require_positive ? "positive" : "non-negative");
            continue;
        }

        *out_val = val;

        return true;
    }
}

/* Recomputes area and perimeter to keep derived fields synchronized */
static void compute_rectangle_metrics(Rectangle *rect) {
    if (rect != NULL) {
        rect->area = rect->width * rect->height;
        rect->perimeter = 2.0f * (rect->width + rect->height);
    }
}

/* Classifies a rectangle as a square using relative floating-point tolerance */
static bool is_square(const Rectangle *rect) {
    if (rect == NULL) {
        return false;
    }

    float diff = fabsf(rect->width - rect->height);
    float largest = (rect->width > rect->height) ? rect->width : rect->height;

    return diff <= (SQUARE_EPSILON * largest);
}

/* Allocates initial dynamic storage with arithmetic overflow protection */
static int init_inventory(RectangleInventory *inv, size_t initial_capacity) {
    if (inv == NULL || initial_capacity == 0 || initial_capacity > SIZE_MAX / sizeof(Rectangle)) {
        return -1;
    }

    Rectangle *ptr = (Rectangle *)malloc(initial_capacity * sizeof(Rectangle));

    if (ptr == NULL) {
        return -1;
    }

    inv->items = ptr;
    inv->count = 0;
    inv->capacity = initial_capacity;

    return 0;
}

/* Doubles array capacity via realloc when full, preserving original pointer on failure */
static int ensure_capacity(RectangleInventory *inv) {
    if (inv == NULL) {
        return -1;
    }

    if (inv->count < inv->capacity) {
        return 0;
    }

    size_t new_cap = (inv->capacity == 0) ? 4 : inv->capacity * 2;

    if (new_cap < inv->capacity || new_cap > SIZE_MAX / sizeof(Rectangle)) {
        return -1;
    }

    Rectangle *resized = (Rectangle *)realloc(inv->items, new_cap * sizeof(Rectangle));

    if (resized == NULL) {
        return -1;
    }

    inv->items = resized;
    inv->capacity = new_cap;
    printf("Notice: Inventory capacity expanded to %zu.\n", inv->capacity);

    return 0;
}

/* Releases all dynamically allocated memory and resets inventory state */
static void free_inventory(RectangleInventory *inv) {
    if (inv != NULL) {
        free(inv->items);
        inv->items = NULL;
        inv->count = 0;
        inv->capacity = 0;
    }
}

/* Returns the array index of the rectangle with matching ID, or -1 if not found */
static int find_rectangle_index(const RectangleInventory *inv, int id) {
    if (inv == NULL || inv->items == NULL) {
        return -1;
    }

    for (size_t i = 0; i < inv->count; i++) {
        if (inv->items[i].id == id) {
            return (int)i;
        }
    }

    return -1;
}

/* Validates inputs, checks for duplicate ID, grows array if needed, and inserts record */
static int add_rectangle(RectangleInventory *inv, int id, float width, float height) {
    if (inv == NULL || id <= 0 || !isfinite(width) || !isfinite(height) || width <= 0.0f || height <= 0.0f) {
        return -1;
    }

    if (find_rectangle_index(inv, id) != -1) {
        return -1;
    }

    if (ensure_capacity(inv) != 0) {
        return -1;
    }

    Rectangle *target = &inv->items[inv->count];
    target->id = id;
    target->width = width;
    target->height = height;
    compute_rectangle_metrics(target);
    inv->count++;

    return 0;
}

/* Updates width and height of an existing rectangle and refreshes area/perimeter */
static int update_rectangle(RectangleInventory *inv, int id, float new_width, float new_height) {
    int idx = find_rectangle_index(inv, id);

    if (idx == -1 || !isfinite(new_width) || !isfinite(new_height) || new_width <= 0.0f || new_height <= 0.0f) {
        return -1;
    }

    inv->items[idx].width = new_width;
    inv->items[idx].height = new_height;
    compute_rectangle_metrics(&inv->items[idx]);

    return 0;
}

/* Deletes a rectangle by ID and shifts remaining elements left to close the gap */
static int delete_rectangle(RectangleInventory *inv, int id) {
    int idx = find_rectangle_index(inv, id);

    if (idx == -1) {
        return -1;
    }

    for (size_t i = (size_t)idx; i + 1 < inv->count; i++) {
        inv->items[i] = inv->items[i + 1];
    }

    inv->count--;

    return 0;
}

/* Prints formatted table column headers */
static void print_table_header(void) {
    printf("\n%-8s | %-10s | %-10s | %-12s | %-12s | %-10s\n",
           "ID", "Width", "Height", "Area", "Perimeter", "Shape");
    printf("---------+------------+------------+--------------+--------------+-----------\n");
}

/* Prints a single rectangle record row */
static void print_rectangle_row(const Rectangle *r) {
    if (r != NULL) {
        printf("%-8d | %-10.2f | %-10.2f | %-12.2f | %-12.2f | %-10s\n",
               r->id,
               (double)r->width,
(double)r->height,
(double)r->area,
(double)r->perimeter,
is_square(r) ? "Square" : "Non-Square");
}
}
/* Displays all stored rectangles */
static void display_all_rectangles(const RectangleInventory *inv) {
if (inv == NULL || inv->count == 0) {
printf("Inventory is empty.\n");
return;
}
print_table_header();
for (size_t i = 0; i < inv->count; i++) {
print_rectangle_row(&inv->items[i]);
}
}
/* Comparator for qsort: ascending by area, breaking ties by ascending ID */
static int compare_rectangles_by_area_asc(const void *a, const void *b) {
const Rectangle *ra = (const Rectangle *)a;
const Rectangle *rb = (const Rectangle *)b;
if (ra->area < rb->area) {
return -1;
}
if (ra->area > rb->area) {
return 1;
}
if (ra->id < rb->id) {
return -1;
}
if (ra->id > rb->id) {
return 1;
}
return 0;
}
/* Sorts a temporary copy of the inventory by area so original order is preserved */
static void display_sorted_rectangles(const RectangleInventory *inv) {
if (inv == NULL || inv->count == 0) {
printf("Inventory is empty. Nothing to sort.\n");
return;
}
Rectangle *copy = (Rectangle *)malloc(inv->count * sizeof(Rectangle));
if (copy == NULL) {
fprintf(stderr, "Error: Memory allocation failed for sorting.\n");
return;
}
memcpy(copy, inv->items, inv->count * sizeof(Rectangle));
qsort(copy, inv->count, sizeof(Rectangle), compare_rectangles_by_area_asc);
printf("\n--- Rectangles Sorted by Area (Ascending) ---");
print_table_header();
for (size_t i = 0; i < inv->count; i++) {
print_rectangle_row(©[i]);
}
free(copy);
}
/* Displays only rectangles whose area strictly exceeds the given threshold */
static void display_filtered_by_area(const RectangleInventory *inv, float threshold) {
if (inv == NULL || inv->count == 0) {
printf("Inventory is empty.\n");
return;
}
size_t matches = 0;
for (size_t i = 0; i < inv->count; i++) {
if (inv->items[i].area > threshold) {
if (matches == 0) {
printf("\n--- Rectangles with Area > %.2f ---", (double)threshold);
print_table_header();
}
print_rectangle_row(&inv->items[i]);
matches++;
}
}
if (matches == 0) {
printf("No rectangles found with area exceeding %.2f.\n", (double)threshold);
}
}
/* Computes and displays largest, smallest, total, and average area */
static void display_geometry_summary(const RectangleInventory *inv) {
if (inv == NULL || inv->count == 0) {
printf("Geometry Analysis unavailable: inventory is empty.\n");
return;
}
size_t max_idx = 0;
size_t min_idx = 0;
double total_area = 0.0;
for (size_t i = 0; i < inv->count; i++) {
if (inv->items[i].area > inv->items[max_idx].area) {
max_idx = i;
}
if (inv->items[i].area < inv->items[min_idx].area) {
min_idx = i;
}
total_area += (double)inv->items[i].area;
}
printf("\n=== Geometry Area Summary (%zu Rectangle%s) ===\n",
inv->count, (inv->count == 1) ? "" : "s");
printf("Largest Area  : %.2f (ID: %d, %.2f x %.2f)\n",
(double)inv->items[max_idx].area,
inv->items[max_idx].id,
(double)inv->items[max_idx].width,
(double)inv->items[max_idx].height);
printf("Smallest Area : %.2f (ID: %d, %.2f x %.2f)\n",
(double)inv->items[min_idx].area,
inv->items[min_idx].id,
(double)inv->items[min_idx].width,
(double)inv->items[min_idx].height);
printf("Total Area    : %.2f\n", total_area);
printf("Average Area  : %.2f\n", total_area / (double)inv->count);
}
/* Displays the interactive menu options */
static void print_menu(void) {
printf("\n--- Menu ---\n");
printf("1. Add Rectangle\n");
printf("2. Display All Rectangles\n");
printf("3. Search Rectangle by ID\n");
printf("4. Update Rectangle Dimensions by ID\n");
printf("5. Delete Rectangle by ID\n");
printf("6. Display Sorted by Area (Ascending)\n");
printf("7. Filter Rectangles by Minimum Area\n");
printf("8. Display Geometry Area Summary\n");
printf("0. Exit\n");
}
int main(void) {
RectangleInventory inv = {NULL, 0, 0};
int initial_count = 0;
printf("=== Rectangle Inventory and Geometry Analysis ===\n");
/* Read initial inventory count and allocate memory */
while (true) {
if (read_int_in_range("Enter initial number of rectangles (1 - 1000): ",
1, MAX_INITIAL_RECTANGLES, &initial_count)) {
if (init_inventory(&inv, (size_t)initial_count) == 0) {
break;
}
fprintf(stderr, "Error: Memory allocation failed.\n");
} else if (feof(stdin)) {
return 1;
}
}
/* Populate the initial rectangles */
for (int i = 0; i < initial_count; i++) {
printf("\nEntering initial rectangle %d of %d:\n", i + 1, initial_count);
int id = 0;
float w = 0.0f;
float h = 0.0f;
while (true) {
if (!read_int_in_range("  Rectangle ID (positive integer): ", 1, INT_MAX, &id)) {
free_inventory(&inv);
return 1;
}
if (find_rectangle_index(&inv, id) != -1) {
printf("  Error: Duplicate ID %d rejected.\n", id);
continue;
}
break;
}
if (!read_finite_float("  Width: ", true, &w) ||
!read_finite_float("  Height: ", true, &h)) {
free_inventory(&inv);
return 1;
}
add_rectangle(&inv, id, w, h);
}
/* Main interactive menu loop */
int choice = -1;
while (choice != 0) {
print_menu();
if (!read_int_in_range("Select option (0 - 8): ", 0, 8, &choice)) {
break;
}
switch (choice) {
case 1: {
int id = 0;
float w = 0.0f;
float h = 0.0f;
if (!read_int_in_range("Enter unique positive Rectangle ID: ", 1, INT_MAX, &id)) {
break;
}
if (find_rectangle_index(&inv, id) != -1) {
printf("Error: Rectangle ID %d already exists.\n", id);
break;
}
if (!read_finite_float("Enter width: ", true, &w) ||
!read_finite_float("Enter height: ", true, &h)) {
break;
}
if (add_rectangle(&inv, id, w, h) == 0) {
printf("Success: Added rectangle %d.\n", id);
} else {
fprintf(stderr, "Error: Failed to add rectangle.\n");
}
break;
}
case 2: {
display_all_rectangles(&inv);
break;
}
case 3: {
if (inv.count == 0) {
printf("Inventory is empty.\n");
break;
}
int id = 0;
if (read_int_in_range("Enter Rectangle ID to search: ", 1, INT_MAX, &id)) {
int idx = find_rectangle_index(&inv, id);
if (idx == -1) {
printf("Not Found: Rectangle ID %d does not exist.\n", id);
} else {
print_table_header();
print_rectangle_row(&inv.items[idx]);
}
}
break;
}
case 4: {
if (inv.count == 0) {
printf("Inventory is empty.\n");
break;
}
int id = 0;
if (!read_int_in_range("Enter Rectangle ID to update: ", 1, INT_MAX, &id)) {
break;
}
if (find_rectangle_index(&inv, id) == -1) {
printf("Not Found: Rectangle ID %d does not exist.\n", id);
break;
}
float w = 0.0f;
float h = 0.0f;
if (!read_finite_float("Enter new width: ", true, &w) ||
!read_finite_float("Enter new height: ", true, &h)) {
break;
}
if (update_rectangle(&inv, id, w, h) == 0) {
printf("Success: Updated rectangle %d and recomputed geometry.\n", id);
}
break;
}
case 5: {
if (inv.count == 0) {
printf("Inventory is empty.\n");
break;
}
int id = 0;
if (read_int_in_range("Enter Rectangle ID to delete: ", 1, INT_MAX, &id)) {
if (delete_rectangle(&inv, id) == 0) {
printf("Success: Deleted rectangle %d.\n", id);
} else {
printf("Not Found: Rectangle ID %d does not exist.\n", id);
}
}
break;
}
case 6: {
display_sorted_rectangles(&inv);
break;
}
case 7: {
float threshold = 0.0f;
if (read_finite_float("Enter minimum area threshold: ", false, &threshold)) {
display_filtered_by_area(&inv, threshold);
}
break;
}
case 8: {
display_geometry_summary(&inv);
break;
}
case 0: {
printf("Releasing memory and exiting...\n");
break;
}
default: {
printf("Invalid option.\n");
break;
}
}
}
free_inventory(&inv);
return 0;
}