/*
 * Test Suite for Save/Load Module
 * Tests game state persistence and recovery
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/world.h"
#include "../include/save_load.h"

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

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) \
    do { \
        if (cond) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            char err[256]; \
            snprintf(err, sizeof(err), "%s (expected: %d, got: %d)", msg, expected, actual); \
            FAIL(err); \
            return; \
        } \
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

// Helper: Create a test world
World create_test_world(void) {
    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "entrance", "Entrance", "The entrance.");
    int room2 = world_add_room(&world, "hall", "Hall", "A great hall.");
    int room3 = world_add_room(&world, "chamber", "Chamber", "A dark chamber.");

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_connect_rooms(&world, room2, DIR_SOUTH, room1);
    world_connect_rooms(&world, room2, DIR_EAST, room3);
    world_connect_rooms(&world, room3, DIR_WEST, room2);

    int key = world_add_item(&world, "key", "rusty key", "An old key.", true);
    int sword = world_add_item(&world, "sword", "iron sword", "A sword.", true);
    int statue = world_add_item(&world, "statue", "statue", "A heavy statue.", false);

    world_place_item(&world, key, room1);
    world_place_item(&world, sword, room2);
    world_place_item(&world, statue, room3);

    world.current_room = room1;

    return world;
}

// Test basic save
void test_basic_save(void) {
    TEST("Basic save");

    World world = create_test_world();
    const char *slot = "test_slot_1";

    ASSERT_TRUE(game_save(&world, slot, "test_world"), "save should succeed");

    // Verify file exists
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/%s.sav",
             getenv("HOME"), slot);

    struct stat st;
    ASSERT_EQ(0, stat(save_path, &st), "save file should exist");

    // Cleanup
    unlink(save_path);

    PASS();
}

// Test save and load
void test_save_and_load(void) {
    TEST("Save and load");

    World world1 = create_test_world();
    const char *slot = "test_slot_2";

    // Modify world state
    // Take key first while in entrance (room 0) before moving
    world_take_item(&world1, "key");  // Take key from current room
    world1.current_room = 1;  // Move to hall
    world1.rooms[0].visited = true;
    world1.rooms[1].visited = true;

    // Save
    ASSERT_TRUE(game_save(&world1, slot, "test_world"), "save should succeed");

    // Create new world and load
    World world2;
    world_init(&world2);
    char loaded_world_name[256];

    ASSERT_TRUE(game_load(&world2, slot, loaded_world_name, sizeof(loaded_world_name)),
                "load should succeed");

    // Verify state
    ASSERT_EQ(world1.current_room, world2.current_room, "current room should match");
    ASSERT_STR_EQ("test_world", loaded_world_name, "world name should match");
    ASSERT_EQ(world1.rooms[0].visited, world2.rooms[0].visited, "room 0 visited should match");
    ASSERT_EQ(world1.rooms[1].visited, world2.rooms[1].visited, "room 1 visited should match");

    // Verify inventory
    ASSERT_EQ(world1.inventory[0], world2.inventory[0], "inventory should match");
    ASSERT_TRUE(world2.inventory[0] >= 0, "should have key in inventory");

    // Cleanup
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/%s.sav",
             getenv("HOME"), slot);
    unlink(save_path);

    PASS();
}

// Test multiple saves
void test_multiple_saves(void) {
    TEST("Multiple save slots");

    World world = create_test_world();

    // Save to multiple slots
    ASSERT_TRUE(game_save(&world, "slot_a", "world_a"), "save slot_a");

    world.current_room = 1;
    ASSERT_TRUE(game_save(&world, "slot_b", "world_b"), "save slot_b");

    world.current_room = 2;
    ASSERT_TRUE(game_save(&world, "slot_c", "world_c"), "save slot_c");

    // Load from different slots
    World loaded;
    char world_name[256];

    world_init(&loaded);
    game_load(&loaded, "slot_a", world_name, sizeof(world_name));
    ASSERT_EQ(0, loaded.current_room, "slot_a current room");
    ASSERT_STR_EQ("world_a", world_name, "slot_a world name");

    world_init(&loaded);
    game_load(&loaded, "slot_b", world_name, sizeof(world_name));
    ASSERT_EQ(1, loaded.current_room, "slot_b current room");
    ASSERT_STR_EQ("world_b", world_name, "slot_b world name");

    world_init(&loaded);
    game_load(&loaded, "slot_c", world_name, sizeof(world_name));
    ASSERT_EQ(2, loaded.current_room, "slot_c current room");
    ASSERT_STR_EQ("world_c", world_name, "slot_c world name");

    // Cleanup
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/slot_a.sav", getenv("HOME"));
    unlink(save_path);
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/slot_b.sav", getenv("HOME"));
    unlink(save_path);
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/slot_c.sav", getenv("HOME"));
    unlink(save_path);

    PASS();
}

// Test load nonexistent save
void test_load_nonexistent(void) {
    TEST("Load nonexistent save");

    World world;
    world_init(&world);
    char world_name[256];

    ASSERT_FALSE(game_load(&world, "nonexistent_slot_xyz", world_name, sizeof(world_name)),
                 "load should fail for nonexistent slot");

    PASS();
}

// Test save directory creation
void test_save_directory_creation(void) {
    TEST("Save directory creation");

    World world = create_test_world();

    // Remove save directory if it exists
    char save_dir[512];
    snprintf(save_dir, sizeof(save_dir), "%s/.adventure-saves", getenv("HOME"));

    // Save should create directory if needed
    ASSERT_TRUE(game_save(&world, "test_dir_slot", "test_world"),
                "save should create directory");

    // Verify directory exists
    struct stat st;
    ASSERT_EQ(0, stat(save_dir, &st), "save directory should exist");
    ASSERT_TRUE(S_ISDIR(st.st_mode), "should be a directory");

    // Cleanup
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/test_dir_slot.sav", save_dir);
    unlink(save_path);

    PASS();
}

// Test inventory persistence
void test_inventory_persistence(void) {
    TEST("Inventory persistence");

    World world = create_test_world();
    const char *slot = "test_inventory";

    // Take multiple items
    world.current_room = 0;
    world_take_item(&world, "key");

    world.current_room = 1;
    world_take_item(&world, "sword");

    // Save
    game_save(&world, slot, "test_world");

    // Load into new world
    World loaded;
    world_init(&loaded);
    char world_name[256];
    game_load(&loaded, slot, world_name, sizeof(world_name));

    // Verify inventory has 2 items
    int item_count = 0;
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (loaded.inventory[i] != -1) {
            item_count++;
        }
    }
    ASSERT_EQ(2, item_count, "should have 2 items in inventory");

    // Cleanup
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/%s.sav",
             getenv("HOME"), slot);
    unlink(save_path);

    PASS();
}

// Test visited rooms persistence
void test_visited_rooms_persistence(void) {
    TEST("Visited rooms persistence");

    World world = create_test_world();
    const char *slot = "test_visited";

    // Visit rooms
    world.rooms[0].visited = true;
    world.rooms[1].visited = true;
    world.rooms[2].visited = false;

    // Save
    game_save(&world, slot, "test_world");

    // Load
    World loaded;
    world_init(&loaded);
    char world_name[256];
    game_load(&loaded, slot, world_name, sizeof(world_name));

    // Verify visited state
    ASSERT_TRUE(loaded.rooms[0].visited, "room 0 should be visited");
    ASSERT_TRUE(loaded.rooms[1].visited, "room 1 should be visited");
    ASSERT_FALSE(loaded.rooms[2].visited, "room 2 should not be visited");

    // Cleanup
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s/.adventure-saves/%s.sav",
             getenv("HOME"), slot);
    unlink(save_path);

    PASS();
}

// Main test runner
int main(void) {
    printf("\n=== Save/Load System Test Suite ===\n\n");

    test_basic_save();
    test_save_and_load();
    test_multiple_saves();
    test_load_nonexistent();
    test_save_directory_creation();
    test_inventory_persistence();
    test_visited_rooms_persistence();

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
