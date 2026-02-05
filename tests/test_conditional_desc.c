/*
 * Test Suite for Conditional Room Descriptions
 * Issue #6: Tests condition evaluation and description selection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/world.h"
#include "../include/world_loader.h"

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  Testing: %s ... ", name); \
    fflush(stdout);

#define PASS() \
    do { \
        printf("\xE2\x9C\x93 PASS\n"); \
        tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { \
        printf("\xE2\x9C\x97 FAIL: %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual, msg) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            char err[512]; \
            snprintf(err, sizeof(err), "%s\n      expected: '%s'\n      got:      '%s'", msg, expected, actual); \
            FAIL(err); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_CONTAINS(haystack, needle, msg) \
    do { \
        if (strstr(haystack, needle) == NULL) { \
            char err[512]; \
            snprintf(err, sizeof(err), "%s\n      expected to contain: '%s'\n      got: '%s'", msg, needle, haystack); \
            FAIL(err); \
            return; \
        } \
    } while(0)

// Test default description when no conditions match
void test_default_description(void) {
    TEST("Default description when no conditions");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default description.");
    world.current_room = room;

    Room *r = world_current_room(&world);
    const char *desc = world_get_room_description(&world, r);

    ASSERT_STR_EQ("Default description.", desc, "should return default description");
    PASS();
}

// Test first_visit condition
void test_first_visit_condition(void) {
    TEST("first_visit condition");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default description.");
    Room *r = &world.rooms[room];

    // Add conditional description for first visit
    r->conditional_descs[0].type = COND_FIRST_VISIT;
    r->conditional_descs[0].negate = false;
    r->conditional_descs[0].subject[0] = '\0';
    strncpy(r->conditional_descs[0].description, "First time here!", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;

    // Room description not shown yet - should show first_visit description
    r->description_shown = false;
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("First time here!", desc, "should show first_visit description");

    // After description shown - should show default (description_shown set by world_get_room_description)
    // Note: world_get_room_description() sets description_shown = true, so we get default now
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Default description.", desc, "should show default after first display");

    PASS();
}

// Test visited condition (return visit)
void test_visited_condition(void) {
    TEST("visited condition (return visit)");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default description.");
    Room *r = &world.rooms[room];

    // Add conditional description for return visit
    r->conditional_descs[0].type = COND_VISITED;
    r->conditional_descs[0].negate = false;
    r->conditional_descs[0].subject[0] = '\0';
    strncpy(r->conditional_descs[0].description, "Welcome back!", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;

    // Not visited yet - should show default
    r->visited = false;
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Default description.", desc, "should show default on first visit");

    // After visiting - should show return description
    r->visited = true;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Welcome back!", desc, "should show visited description");

    PASS();
}

// Test has_item condition
void test_has_item_condition(void) {
    TEST("has_item condition");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "A dark room.");
    int lantern = world_add_item(&world, "lantern", "brass lantern", "A lantern.", true);

    Room *r = &world.rooms[room];

    // Add conditional description for having lantern
    r->conditional_descs[0].type = COND_HAS_ITEM;
    r->conditional_descs[0].negate = false;
    strncpy(r->conditional_descs[0].subject, "lantern", sizeof(r->conditional_descs[0].subject));
    strncpy(r->conditional_descs[0].description, "The lantern illuminates the room!", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;
    r->visited = true;

    // Without lantern - should show default
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("A dark room.", desc, "should show default without lantern");

    // With lantern in inventory
    world.inventory[0] = lantern;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("The lantern illuminates the room!", desc, "should show has_item description");

    PASS();
}

// Test negated condition (!has_item)
void test_negated_condition(void) {
    TEST("negated condition (!has_item)");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default description.");
    int key = world_add_item(&world, "key", "rusty key", "A key.", true);

    Room *r = &world.rooms[room];

    // Add conditional description for NOT having key
    r->conditional_descs[0].type = COND_HAS_ITEM;
    r->conditional_descs[0].negate = true;  // Negated!
    strncpy(r->conditional_descs[0].subject, "key", sizeof(r->conditional_descs[0].subject));
    strncpy(r->conditional_descs[0].description, "You need to find the key.", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;
    r->visited = true;

    // Without key - negated condition should match
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("You need to find the key.", desc, "should show !has_item description");

    // With key - negated condition should NOT match
    world.inventory[0] = key;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Default description.", desc, "should show default with key");

    PASS();
}

// Test room_has_item condition
void test_room_has_item_condition(void) {
    TEST("room_has_item condition");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "An empty room.");
    int coin = world_add_item(&world, "coin", "gold coin", "A coin.", true);

    Room *r = &world.rooms[room];

    // Add conditional description for coin in room
    r->conditional_descs[0].type = COND_ROOM_HAS_ITEM;
    r->conditional_descs[0].negate = false;
    strncpy(r->conditional_descs[0].subject, "coin", sizeof(r->conditional_descs[0].subject));
    strncpy(r->conditional_descs[0].description, "Something glints on the floor.", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;
    r->visited = true;

    // Without coin in room - should show default
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("An empty room.", desc, "should show default without coin");

    // With coin in room
    world_place_item(&world, coin, room);
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Something glints on the floor.", desc, "should show room_has_item description");

    PASS();
}

// Test item_used condition
void test_item_used_condition(void) {
    TEST("item_used condition");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Normal room.");
    int scroll = world_add_item(&world, "scroll", "magic scroll", "A scroll.", true);

    Room *r = &world.rooms[room];
    Item *scroll_item = &world.items[scroll];

    // Add conditional description for used scroll
    r->conditional_descs[0].type = COND_ITEM_USED;
    r->conditional_descs[0].negate = false;
    strncpy(r->conditional_descs[0].subject, "scroll", sizeof(r->conditional_descs[0].subject));
    strncpy(r->conditional_descs[0].description, "Arcane symbols glow on the walls.", sizeof(r->conditional_descs[0].description));
    r->conditional_desc_count = 1;

    world.current_room = room;
    r->visited = true;

    // Scroll not used - should show default
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Normal room.", desc, "should show default before scroll used");

    // After using scroll
    scroll_item->used = true;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Arcane symbols glow on the walls.", desc, "should show item_used description");

    PASS();
}

// Test priority: item_used > has_item > room_has_item > visited
void test_condition_priority(void) {
    TEST("Condition priority ordering");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default.");
    int lantern = world_add_item(&world, "lantern", "lantern", "A lantern.", true);
    int scroll = world_add_item(&world, "scroll", "scroll", "A scroll.", true);

    Room *r = &world.rooms[room];

    // Add multiple conditions
    // 1. visited (priority 1)
    r->conditional_descs[0].type = COND_VISITED;
    r->conditional_descs[0].negate = false;
    strncpy(r->conditional_descs[0].description, "Visited desc.", sizeof(r->conditional_descs[0].description));

    // 2. has_item (priority 3)
    r->conditional_descs[1].type = COND_HAS_ITEM;
    r->conditional_descs[1].negate = false;
    strncpy(r->conditional_descs[1].subject, "lantern", sizeof(r->conditional_descs[1].subject));
    strncpy(r->conditional_descs[1].description, "Has item desc.", sizeof(r->conditional_descs[1].description));

    // 3. item_used (priority 4)
    r->conditional_descs[2].type = COND_ITEM_USED;
    r->conditional_descs[2].negate = false;
    strncpy(r->conditional_descs[2].subject, "scroll", sizeof(r->conditional_descs[2].subject));
    strncpy(r->conditional_descs[2].description, "Item used desc.", sizeof(r->conditional_descs[2].description));

    r->conditional_desc_count = 3;

    world.current_room = room;
    r->visited = true;

    // Only visited matches - should show visited
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Visited desc.", desc, "should show visited when only visited matches");

    // Add lantern - has_item should win (higher priority)
    world.inventory[0] = lantern;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Has item desc.", desc, "should show has_item over visited");

    // Mark scroll as used - item_used should win (highest priority)
    world.items[scroll].used = true;
    desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Item used desc.", desc, "should show item_used over has_item");

    PASS();
}

// Test same-priority tie-breaking (first defined wins)
void test_same_priority_tiebreaking(void) {
    TEST("Same priority tie-breaking (first defined wins)");

    World world;
    world_init(&world);

    int room = world_add_room(&world, "test", "Test Room", "Default.");
    int lantern = world_add_item(&world, "lantern", "lantern", "A lantern.", true);
    int torch = world_add_item(&world, "torch", "torch", "A torch.", true);
    (void)torch;  // Suppress unused warning - torch exists but player won't have it

    Room *r = &world.rooms[room];

    // Add two has_item conditions with same priority (priority 3)
    // First: has_item=lantern
    r->conditional_descs[0].type = COND_HAS_ITEM;
    r->conditional_descs[0].negate = false;
    strncpy(r->conditional_descs[0].subject, "lantern", sizeof(r->conditional_descs[0].subject));
    strncpy(r->conditional_descs[0].description, "Lantern desc (first).", sizeof(r->conditional_descs[0].description));

    // Second: !has_item=torch (also priority 3, will match when player doesn't have torch)
    r->conditional_descs[1].type = COND_HAS_ITEM;
    r->conditional_descs[1].negate = true;
    strncpy(r->conditional_descs[1].subject, "torch", sizeof(r->conditional_descs[1].subject));
    strncpy(r->conditional_descs[1].description, "No torch desc (second).", sizeof(r->conditional_descs[1].description));

    r->conditional_desc_count = 2;

    world.current_room = room;
    r->visited = true;
    r->description_shown = true;  // Skip first_visit logic

    // Player has lantern but not torch - both conditions match
    // First one defined (lantern) should win due to tie-breaking
    world.inventory[0] = lantern;
    const char *desc = world_get_room_description(&world, r);
    ASSERT_STR_EQ("Lantern desc (first).", desc, "first defined condition should win on tie");

    PASS();
}

// Test loading conditional descriptions from file
void test_load_conditional_descriptions(void) {
    TEST("Load conditional descriptions from file");

    World world;
    LoadError error;

    bool loaded = world_load_from_file(&world, "worlds/conditional_test.world", &error);

    if (!loaded) {
        char err[256];
        snprintf(err, sizeof(err), "failed to load world: %s", world_loader_get_error(&error));
        FAIL(err);
        return;
    }

    // Find the cellar room
    int cellar_idx = world_find_room(&world, "cellar");
    ASSERT_TRUE(cellar_idx >= 0, "cellar room should exist");

    Room *cellar = &world.rooms[cellar_idx];

    // Should have conditional descriptions
    ASSERT_TRUE(cellar->conditional_desc_count > 0, "cellar should have conditional descriptions");

    // Verify at least one condition was parsed correctly
    bool found_first_visit = false;
    bool found_has_lantern = false;

    for (int i = 0; i < cellar->conditional_desc_count; i++) {
        ConditionalDesc *cond = &cellar->conditional_descs[i];
        if (cond->type == COND_FIRST_VISIT && !cond->negate) {
            found_first_visit = true;
            ASSERT_STR_CONTAINS(cond->description, "first time", "first_visit should mention first time");
        }
        if (cond->type == COND_HAS_ITEM && strcmp(cond->subject, "lantern") == 0 && !cond->negate) {
            found_has_lantern = true;
            ASSERT_STR_CONTAINS(cond->description, "lantern", "has_item=lantern should mention lantern");
        }
    }

    ASSERT_TRUE(found_first_visit, "should have first_visit condition");
    ASSERT_TRUE(found_has_lantern, "should have has_item=lantern condition");

    PASS();
}

// Main test runner
int main(void) {
    printf("\n=== Conditional Description Test Suite ===\n");
    printf("Issue #6: Conditional Room Descriptions\n\n");

    test_default_description();
    test_first_visit_condition();
    test_visited_condition();
    test_has_item_condition();
    test_negated_condition();
    test_room_has_item_condition();
    test_item_used_condition();
    test_condition_priority();
    test_same_priority_tiebreaking();
    test_load_conditional_descriptions();

    printf("\n=== Test Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("\n\xE2\x9C\x93 All tests passed!\n\n");
        return 0;
    } else {
        printf("\n\xE2\x9C\x97 Some tests failed!\n\n");
        return 1;
    }
}
