/*
 * Test Suite for Path Traversal Protection
 * Tests is_safe_filename() function
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../include/save_load.h"

#define TEST(name) printf("  Testing: %s ... ", name);
#define ASSERT(condition, msg) \
    if (!(condition)) { \
        printf("✗ FAIL: %s\n", msg); \
        return 1; \
    }
#define PASS() printf("✓ PASS\n");

int test_safe_filenames() {
    TEST("Valid alphanumeric filename");
    ASSERT(is_safe_filename("mysave123") == true, "mysave123 should be valid");
    PASS();

    TEST("Valid with underscores");
    ASSERT(is_safe_filename("my_save_slot") == true, "my_save_slot should be valid");
    PASS();

    TEST("Valid with hyphens");
    ASSERT(is_safe_filename("my-save-slot") == true, "my-save-slot should be valid");
    PASS();

    return 0;
}

int test_path_traversal_blocked() {
    TEST("Block directory traversal (..)");
    ASSERT(is_safe_filename("../etc/passwd") == false, "../etc/passwd should be blocked");
    PASS();

    TEST("Block absolute paths");
    ASSERT(is_safe_filename("/etc/passwd") == false, "/etc/passwd should be blocked");
    PASS();

    TEST("Block Windows paths");
    ASSERT(is_safe_filename("C:\\Windows\\System32") == false, "Windows paths should be blocked");
    PASS();

    TEST("Block slashes");
    ASSERT(is_safe_filename("saves/mysave") == false, "slashes should be blocked");
    PASS();

    TEST("Block parent directory reference");
    ASSERT(is_safe_filename("..") == false, ".. should be blocked");
    PASS();

    TEST("Block hidden directory reference");
    ASSERT(is_safe_filename("....evil") == false, ".. in filename should be blocked");
    PASS();

    return 0;
}

int test_invalid_characters() {
    TEST("Block spaces");
    ASSERT(is_safe_filename("my save") == false, "spaces should be blocked");
    PASS();

    TEST("Block special characters");
    ASSERT(is_safe_filename("save$slot") == false, "$ should be blocked");
    PASS();

    TEST("Block semicolons");
    ASSERT(is_safe_filename("save;rm-rf") == false, "; should be blocked");
    PASS();

    TEST("Block pipes");
    ASSERT(is_safe_filename("save|cat") == false, "| should be blocked");
    PASS();

    return 0;
}

int test_edge_cases() {
    TEST("Empty string");
    ASSERT(is_safe_filename("") == false, "empty string should be invalid");
    PASS();

    TEST("NULL pointer");
    ASSERT(is_safe_filename(NULL) == false, "NULL should be invalid");
    PASS();

    TEST("Too long filename (>64 chars)");
    char long_name[100];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[99] = '\0';
    ASSERT(is_safe_filename(long_name) == false, "very long filenames should be blocked");
    PASS();

    return 0;
}

int main() {
    printf("\n");
    printf("=== Path Traversal Protection Test Suite ===\n\n");

    int failed = 0;

    failed += test_safe_filenames();
    failed += test_path_traversal_blocked();
    failed += test_invalid_characters();
    failed += test_edge_cases();

    printf("\n");
    printf("=== Test Results ===\n");
    if (failed == 0) {
        printf("✓ All tests passed!\n");
        printf("\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed!\n", failed);
        printf("\n");
        return 1;
    }
}
