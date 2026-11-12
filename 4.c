/*
 * Problem 1 (Extended): Student Records and Academic Analytics System
 * File: 4.c
 * Standard: ISO C11
 * Compilation: gcc -std=c11 -Wall -Wextra -Wpedantic 4.c -o 4
 *
 * Description:
 *   Interactive menu-driven Student Records and Academic Analytics System.
 *   Dynamically allocates and resizes memory using malloc/realloc with overflow
 *   guards, validates all fields, supports non-destructive GPA sorting, and
 *   computes highest, lowest, and average GPA statistics.
 *
 * Sample Run:
 *   Enter initial student capacity (1 - 100000): 2
 *   Select an option (0 - 7): 1  -> Roll: 101, Name: "Aladdin", Age: 20, GPA: 3.50
 *   Select an option (0 - 7): 1  -> Roll: 102, Name: "Bruce Wayne", Age: 21, GPA: 3.70
 *   Select an option (0 - 7): 1  -> Roll: 103, Name: "Selina Kyle", Age: 22, GPA: 3.80 (realloc -> 4)
 *   Select an option (0 - 7): 4  -> Update Roll 102: Age: 22, GPA: 3.85
 *   Select an option (0 - 7): 5  -> Delete Roll 101
 *   Select an option (0 - 7): 6  -> Sorted list: 102 (3.85), 103 (3.80)
 *   Select an option (0 - 7): 7  -> Highest: 102 (3.85), Lowest: 103 (3.80), Average: 3.83
 *   Select an option (0 - 7): 0  -> Frees allocated memory and exits cleanly.
 *
 * Edge Cases Tested:
 *   - Zero, negative, non-numeric, or oversized (>100000) initial capacity
 *   - Operations on an empty collection (display, search, update, delete, sort, summary)
 *   - Duplicate positive roll number rejection
 *   - Out-of-range age (<16 or >100) and GPA (<0.00 or >4.00) rejection
 *   - Names containing spaces and boundary-length names (49 characters)
 *   - Overlength name rejection (>49 characters) with stdin buffer flushing
 *   - Searching, updating, or deleting a non-existent roll number
 *   - Automatic array capacity doubling via realloc with arithmetic overflow guard
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50
#define MIN_AGE 16
#define MAX_AGE 100
#define MIN_GPA 0.00f
#define MAX_GPA 4.00f
#define MAX_INITIAL_CAPACITY 100000
#define INPUT_BUFFER_SIZE 256

/* Represents a single student record */
typedef struct {
    int roll_number;
    char name[MAX_NAME_LEN];
    int age;
    float gpa;
} Student;

/* Manages the dynamic array of students along with its count and capacity */
typedef struct {
    Student *records;
    size_t count;
    size_t capacity;
} StudentSystem;

/* Discards leftover characters from stdin when an input line exceeds buffer size */
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

/* Prompts and validates an integer strictly within [min_val, max_val] */
static bool read_int_in_range(const char *prompt, int min_val, int max_val, int *out_val) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);

        if (!read_line_trimmed(buf, sizeof(buf))) {
            if (feof(stdin)) {
                return false;
            }
            printf("Error: Input line too long. Please try again.\n");
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
            printf("Error: Please enter a valid integer between %d and %d.\n", min_val, max_val);
            continue;
        }

        *out_val = (int)val;

        return true;
    }
}

/* Prompts and validates a float strictly within [min_val, max_val] */
static bool read_float_in_range(const char *prompt, float min_val, float max_val, float *out_val) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);

        if (!read_line_trimmed(buf, sizeof(buf))) {
            if (feof(stdin)) {
                return false;
            }
            printf("Error: Input line too long. Please try again.\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("Error: Input cannot be empty.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        float val = strtof(buf, &endptr);

        if (errno == ERANGE || *endptr != '\0' || val < min_val || val > max_val) {
            printf("Error: Please enter a valid number between %.2f and %.2f.\n",
                   (double)min_val, (double)max_val);
            continue;
        }

        *out_val = val;

        return true;
    }
}

/* Reads a student name allowing spaces while enforcing maximum storage length */
static bool read_student_name(const char *prompt, char *out_name, size_t max_len) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);

        if (!read_line_trimmed(buf, sizeof(buf))) {
            if (feof(stdin)) {
                return false;
            }
            printf("Error: Name input exceeded buffer limit.\n");
            continue;
        }

        size_t len = strlen(buf);

        if (len == 0) {
            printf("Error: Name cannot be empty.\n");
            continue;
        }

        if (len >= max_len) {
            printf("Error: Name exceeds maximum length of %zu characters.\n", max_len - 1);
            continue;
        }

        memcpy(out_name, buf, len + 1);

        return true;
    }
}

/* Allocates initial dynamic memory for the student array with overflow guard */
static int init_system(StudentSystem *sys, size_t initial_capacity) {
    if (sys == NULL || initial_capacity == 0 || initial_capacity > SIZE_MAX / sizeof(Student)) {
        return -1;
    }

    Student *ptr = (Student *)malloc(initial_capacity * sizeof(Student));

    if (ptr == NULL) {
        return -1;
    }

    sys->records = ptr;
    sys->count = 0;
    sys->capacity = initial_capacity;

    return 0;
}

/* Doubles array capacity via realloc when full; preserves existing data on failure */
static int ensure_capacity(StudentSystem *sys) {
    if (sys == NULL) {
        return -1;
    }

    if (sys->count < sys->capacity) {
        return 0;
    }

    size_t new_capacity = (sys->capacity == 0) ? 4 : sys->capacity * 2;

    if (new_capacity < sys->capacity || new_capacity > SIZE_MAX / sizeof(Student)) {
        return -1;
    }

    Student *resized = (Student *)realloc(sys->records, new_capacity * sizeof(Student));

    if (resized == NULL) {
        return -1;
    }

    sys->records = resized;
    sys->capacity = new_capacity;
    printf("Notice: Array capacity expanded to %zu records.\n", sys->capacity);

    return 0;
}

/* Releases dynamically allocated memory and resets system metadata */
static void free_system(StudentSystem *sys) {
    if (sys != NULL) {
        free(sys->records);
        sys->records = NULL;
        sys->count = 0;
        sys->capacity = 0;
    }
}

/* Searches for a student by roll number; returns array index or -1 if not found */
static int find_student_index(const StudentSystem *sys, int roll_number) {
    if (sys == NULL || sys->records == NULL) {
        return -1;
    }

    for (size_t i = 0; i < sys->count; i++) {
        if (sys->records[i].roll_number == roll_number) {
            return (int)i;
        }
    }

    return -1;
}

/* Inserts a new student record after checking uniqueness and capacity */
static int add_student(StudentSystem *sys, int roll_number, const char *name, int age, float gpa) {
    if (sys == NULL || name == NULL || find_student_index(sys, roll_number) != -1) {
        return -1;
    }

    if (ensure_capacity(sys) != 0) {
        return -1;
    }

    Student *target = &sys->records[sys->count];
    target->roll_number = roll_number;
    strncpy(target->name, name, MAX_NAME_LEN - 1);
    target->name[MAX_NAME_LEN - 1] = '\0';
    target->age = age;
    target->gpa = gpa;

    sys->count++;

    return 0;
}

/* Updates name, age, and GPA of an existing student identified by roll number */
static int update_student(StudentSystem *sys, int roll_number, const char *new_name, int new_age, float new_gpa) {
    int idx = find_student_index(sys, roll_number);

    if (idx == -1 || new_name == NULL) {
        return -1;
    }

    strncpy(sys->records[idx].name, new_name, MAX_NAME_LEN - 1);
    sys->records[idx].name[MAX_NAME_LEN - 1] = '\0';
    sys->records[idx].age = new_age;
    sys->records[idx].gpa = new_gpa;

    return 0;
}

/* Deletes a student by roll number and shifts trailing elements left to close the gap */
static int delete_student(StudentSystem *sys, int roll_number) {
    int idx = find_student_index(sys, roll_number);

    if (idx == -1) {
        return -1;
    }

    for (size_t i = (size_t)idx; i + 1 < sys->count; i++) {
        sys->records[i] = sys->records[i + 1];
    }

    sys->count--;

    return 0;
}

/* Prints formatted table column headers */
static void print_student_header(void) {
printf("\n%-10s | %-30s | %-5s | %-6s\n", "Roll No", "Name", "Age", "GPA");
printf("-----------+--------------------------------+-------+-------\n");
}
/* Prints a single formatted student record row */
static void print_student_row(const Student *s) {
if (s != NULL) {
printf("%-10d | %-30s | %-5d | %.2f\n",
s->roll_number, s->name, s->age, (double)s->gpa);
}
}
/* Displays all stored student records in insertion order */
static void display_all_students(const StudentSystem *sys) {
if (sys == NULL || sys->count == 0) {
printf("No student records to display (collection is empty).\n");
return;
}
print_student_header();
for (size_t i = 0; i < sys->count; i++) {
print_student_row(&sys->records[i]);
}
}
/* Comparator for qsort: descending GPA, breaking ties by ascending roll number */
static int compare_students_by_gpa(const void *a, const void *b) {
const Student *sa = (const Student *)a;
const Student *sb = (const Student *)b;
if (sa->gpa < sb->gpa) {
return 1;
}
if (sa->gpa > sb->gpa) {
return -1;
}
if (sa->roll_number > sb->roll_number) {
return 1;
}
if (sa->roll_number < sb->roll_number) {
return -1;
}
return 0;
}
/* Sorts a temporary copy of records so original insertion order is preserved */
static void display_sorted_students(const StudentSystem *sys) {
if (sys == NULL || sys->count == 0) {
printf("No student records to sort (collection is empty).\n");
return;
}
Student *copy = (Student *)malloc(sys->count * sizeof(Student));
if (copy == NULL) {
fprintf(stderr, "Error: Unable to allocate temporary buffer for sorting.\n");
return;
}
memcpy(copy, sys->records, sys->count * sizeof(Student));
qsort(copy, sys->count, sizeof(Student), compare_students_by_gpa);
printf("\n--- Students Sorted by GPA (Descending) ---");
print_student_header();
for (size_t i = 0; i < sys->count; i++) {
print_student_row(©[i]);
}
free(copy);
}
/* Computes and displays highest GPA, lowest GPA, and average GPA across records */
static void display_gpa_summary(const StudentSystem *sys) {
if (sys == NULL || sys->count == 0) {
printf("Academic Analytics unavailable: the student collection is empty.\n");
return;
}
size_t max_idx = 0;
size_t min_idx = 0;
double total_gpa = 0.0;
for (size_t i = 0; i < sys->count; i++) {
if (sys->records[i].gpa > sys->records[max_idx].gpa) {
max_idx = i;
}
if (sys->records[i].gpa < sys->records[min_idx].gpa) {
min_idx = i;
}
total_gpa += (double)sys->records[i].gpa;
}
printf("\n=== Academic GPA Summary (%zu Student%s) ===\n",
sys->count, (sys->count == 1) ? "" : "s");
printf("Highest GPA : %.2f (Roll: %d, Name: %s)\n",
(double)sys->records[max_idx].gpa,
sys->records[max_idx].roll_number,
sys->records[max_idx].name);
printf("Lowest GPA  : %.2f (Roll: %d, Name: %s)\n",
(double)sys->records[min_idx].gpa,
sys->records[min_idx].roll_number,
sys->records[min_idx].name);
printf("Average GPA : %.2f\n", total_gpa / (double)sys->count);
}
/* Displays the interactive console menu options */
static void print_menu(void) {
printf("\n--- Menu ---\n");
printf("1. Add Student Record\n");
printf("2. Display All Students\n");
printf("3. Search Student by Roll Number\n");
printf("4. Update Student Record\n");
printf("5. Delete Student Record\n");
printf("6. Display Students Sorted by GPA\n");
printf("7. Display GPA Summary (Highest, Lowest, Average)\n");
printf("0. Exit\n");
}
int main(void) {
StudentSystem sys = {NULL, 0, 0};
int initial_cap = 0;
printf("=== Student Records and Academic Analytics System ===\n");
/* Prompt for valid initial array capacity */
while (true) {
if (read_int_in_range("Enter initial student capacity (1 - 100000): ",
1, MAX_INITIAL_CAPACITY, &initial_cap)) {
if (init_system(&sys, (size_t)initial_cap) == 0) {
break;
}
fprintf(stderr, "Error: Memory allocation failed for capacity %d.\n", initial_cap);
} else if (feof(stdin)) {
return 1;
}
}
int choice = -1;
/* Main interactive menu loop */
while (choice != 0) {
print_menu();
if (!read_int_in_range("Select an option (0 - 7): ", 0, 7, &choice)) {
printf("\nInput stream closed. Exiting safely...\n");
break;
}
switch (choice) {
case 1: {
int roll = 0;
int age = 0;
float gpa = 0.0f;
char name[MAX_NAME_LEN];
if (!read_int_in_range("Enter unique positive roll number: ", 1, INT_MAX, &roll)) {
break;
}
if (find_student_index(&sys, roll) != -1) {
printf("Error: Roll number %d already exists.\n", roll);
break;
}
if (!read_student_name("Enter student name (max 49 chars): ", name, MAX_NAME_LEN)) {
break;
}
if (!read_int_in_range("Enter age (16 - 100): ", MIN_AGE, MAX_AGE, &age)) {
break;
}
if (!read_float_in_range("Enter GPA (0.00 - 4.00): ", MIN_GPA, MAX_GPA, &gpa)) {
break;
}
if (add_student(&sys, roll, name, age, gpa) == 0) {
printf("Success: Added student '%s' (Roll: %d).\n", name, roll);
} else {
fprintf(stderr, "Error: Memory allocation failed.\n");
}
break;
}
case 2: {
display_all_students(&sys);
break;
}
case 3: {
if (sys.count == 0) {
printf("Collection is empty.\n");
break;
}
int roll = 0;
if (read_int_in_range("Enter roll number to search: ", 1, INT_MAX, &roll)) {
int idx = find_student_index(&sys, roll);
if (idx == -1) {
printf("Not Found: No student with roll number %d.\n", roll);
} else {
print_student_header();
print_student_row(&sys.records[idx]);
}
}
break;
}
case 4: {
if (sys.count == 0) {
printf("Collection is empty.\n");
break;
}
int roll = 0;
if (!read_int_in_range("Enter roll number to update: ", 1, INT_MAX, &roll)) {
break;
}
int idx = find_student_index(&sys, roll);
if (idx == -1) {
printf("Not Found: No student with roll number %d.\n", roll);
break;
}
char new_name[MAX_NAME_LEN];
int new_age = 0;
float new_gpa = 0.0f;
if (!read_student_name("Enter updated name (max 49 chars): ", new_name, MAX_NAME_LEN)) {
break;
}
if (!read_int_in_range("Enter updated age (16 - 100): ", MIN_AGE, MAX_AGE, &new_age)) {
break;
}
if (!read_float_in_range("Enter updated GPA (0.00 - 4.00): ", MIN_GPA, MAX_GPA, &new_gpa)) {
break;
}
if (update_student(&sys, roll, new_name, new_age, new_gpa) == 0) {
printf("Success: Updated record for roll number %d.\n", roll);
}
break;
}
case 5: {
if (sys.count == 0) {
printf("Collection is empty.\n");
break;
}
int roll = 0;
if (read_int_in_range("Enter roll number to delete: ", 1, INT_MAX, &roll)) {
if (delete_student(&sys, roll) == 0) {
printf("Success: Deleted student with roll number %d.\n", roll);
} else {
printf("Not Found: No student with roll number %d.\n", roll);
}
}
break;
}
case 6: {
display_sorted_students(&sys);
break;
}
case 7: {
display_gpa_summary(&sys);
break;
}
case 0: {
printf("Exiting program and releasing memory...\n");
break;
}
default: {
printf("Invalid menu choice.\n");
break;
}
}
}
/* Release all heap memory before program termination */
free_system(&sys);
return 0;
}