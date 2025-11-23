/*
 * Test Suite for Parser Module
 * Tests command parsing, verb extraction, and multi-word support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/parser.h"

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  Testing: %s ... ", name); \
    fflush(stdout);

#define PASS() \
    do { \
        printf("✓ PASS\n"); \
        tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { \
        printf("✗ FAIL: %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT_STR_EQ(expected, actual, msg) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            char err[256]; \
            snprintf(err, sizeof(err), "%s (expected: '%s', got: '%s')", msg, expected, actual); \
            FAIL(err); \
            return; \
        } \
    } while(0)

// Test basic single-word commands
void test_single_word_commands(void) {
    TEST("Single-word commands");

    Command cmd;

    // Test "look"
    cmd = parse_input("look");
    ASSERT_STR_EQ("look", cmd.verb, "look verb");
    ASSERT_STR_EQ("", cmd.noun, "look should have no noun");

    // Test "inventory"
    cmd = parse_input("inventory");
    ASSERT_STR_EQ("inventory", cmd.verb, "inventory verb");
    ASSERT_STR_EQ("", cmd.noun, "inventory should have no noun");

    // Test "quit"
    cmd = parse_input("quit");
    ASSERT_STR_EQ("quit", cmd.verb, "quit verb");
    ASSERT_STR_EQ("", cmd.noun, "quit should have no noun");

    PASS();
}

// Test two-word commands
void test_two_word_commands(void) {
    TEST("Two-word commands");

    Command cmd;

    // Test "go north"
    cmd = parse_input("go north");
    ASSERT_STR_EQ("go", cmd.verb, "go verb");
    ASSERT_STR_EQ("north", cmd.noun, "north noun");

    // Test "take key"
    cmd = parse_input("take key");
    ASSERT_STR_EQ("take", cmd.verb, "take verb");
    ASSERT_STR_EQ("key", cmd.noun, "key noun");

    // Test "drop sword"
    cmd = parse_input("drop sword");
    ASSERT_STR_EQ("drop", cmd.verb, "drop verb");
    ASSERT_STR_EQ("sword", cmd.noun, "sword noun");

    PASS();
}

// Test multi-word noun handling
void test_multi_word_nouns(void) {
    TEST("Multi-word nouns");

    Command cmd;

    // Test "take rusty key"
    cmd = parse_input("take rusty key");
    ASSERT_STR_EQ("take", cmd.verb, "take verb");
    ASSERT_STR_EQ("rusty key", cmd.noun, "rusty key noun");

    // Test "examine burning torch"
    cmd = parse_input("examine burning torch");
    ASSERT_STR_EQ("examine", cmd.verb, "examine verb");
    ASSERT_STR_EQ("burning torch", cmd.noun, "burning torch noun");

    // Test "go dark tower"
    cmd = parse_input("go dark tower");
    ASSERT_STR_EQ("go", cmd.verb, "go verb");
    ASSERT_STR_EQ("dark tower", cmd.noun, "dark tower noun");

    PASS();
}

// Test case insensitivity
void test_case_insensitive(void) {
    TEST("Case insensitivity");

    Command cmd;

    // Test uppercase
    cmd = parse_input("LOOK");
    ASSERT_STR_EQ("look", cmd.verb, "LOOK should become look");

    // Test mixed case
    cmd = parse_input("TaKe KEY");
    ASSERT_STR_EQ("take", cmd.verb, "TaKe should become take");
    ASSERT_STR_EQ("key", cmd.noun, "KEY should become key");

    // Test with multi-word
    cmd = parse_input("EXAMINE Rusty KEY");
    ASSERT_STR_EQ("examine", cmd.verb, "EXAMINE should become examine");
    ASSERT_STR_EQ("rusty key", cmd.noun, "Rusty KEY should become rusty key");

    PASS();
}

// Test whitespace handling
void test_whitespace_handling(void) {
    TEST("Whitespace handling");

    Command cmd;

    // Test leading whitespace
    cmd = parse_input("  look");
    ASSERT_STR_EQ("look", cmd.verb, "leading whitespace");

    // Test trailing whitespace
    cmd = parse_input("look  ");
    ASSERT_STR_EQ("look", cmd.verb, "trailing whitespace");

    // Test multiple spaces
    cmd = parse_input("take    key");
    ASSERT_STR_EQ("take", cmd.verb, "multiple spaces verb");
    ASSERT_STR_EQ("key", cmd.noun, "multiple spaces noun");

    // Test tabs
    cmd = parse_input("go\tnorth");
    ASSERT_STR_EQ("go", cmd.verb, "tab separator verb");
    ASSERT_STR_EQ("north", cmd.noun, "tab separator noun");

    PASS();
}

// Test empty and invalid input
void test_empty_invalid_input(void) {
    TEST("Empty and invalid input");

    Command cmd;

    // Test empty string
    cmd = parse_input("");
    ASSERT_STR_EQ("", cmd.verb, "empty string verb");
    ASSERT_STR_EQ("", cmd.noun, "empty string noun");

    // Test only whitespace
    cmd = parse_input("   ");
    ASSERT_STR_EQ("", cmd.verb, "whitespace only verb");
    ASSERT_STR_EQ("", cmd.noun, "whitespace only noun");

    PASS();
}

// Test direction shortcuts
void test_direction_shortcuts(void) {
    TEST("Direction shortcuts");

    Command cmd;

    // Test single letter directions
    const char *shortcuts[] = {"n", "s", "e", "w", "u", "d"};
    const char *expected[] = {"n", "s", "e", "w", "u", "d"};

    for (int i = 0; i < 6; i++) {
        cmd = parse_input(shortcuts[i]);
        ASSERT_STR_EQ(expected[i], cmd.verb, "shortcut direction");
    }

    PASS();
}

// Test special commands
void test_special_commands(void) {
    TEST("Special commands");

    Command cmd;

    // Test help variants
    cmd = parse_input("help");
    ASSERT_STR_EQ("help", cmd.verb, "help command");

    cmd = parse_input("?");
    ASSERT_STR_EQ("?", cmd.verb, "? command");

    // Test save/load
    cmd = parse_input("save slot1");
    ASSERT_STR_EQ("save", cmd.verb, "save verb");
    ASSERT_STR_EQ("slot1", cmd.noun, "save slot");

    cmd = parse_input("load mysave");
    ASSERT_STR_EQ("load", cmd.verb, "load verb");
    ASSERT_STR_EQ("mysave", cmd.noun, "load slot");

    PASS();
}

// Main test runner
int main(void) {
    printf("\n=== Parser Test Suite ===\n\n");

    test_single_word_commands();
    test_two_word_commands();
    test_multi_word_nouns();
    test_case_insensitive();
    test_whitespace_handling();
    test_empty_invalid_input();
    test_direction_shortcuts();
    test_special_commands();

    printf("\n=== Test Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n\n");
        return 1;
    }
}
