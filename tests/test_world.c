/*
 * Test Suite for World Module
 * Tests world creation, room navigation, item management, and inventory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/world.h"

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
            char err[512]; \
            snprintf(err, sizeof(err), "%s (expected: %d, got: %d)", msg, expected, actual); \
            FAIL(err); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual, msg) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            char err[512]; \
            snprintf(err, sizeof(err), "%s (expected: '%.128s', got: '%.128s')", msg, expected, actual); \
            FAIL(err); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, msg) \
    do { \
        if (ptr == NULL) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr, msg) \
    do { \
        if (ptr != NULL) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

// Test world initialization
void test_world_init(void) {
    TEST("World initialization");

    World world;
    world_init(&world);

    ASSERT_EQ(0, world.room_count, "room count should be 0");
    ASSERT_EQ(0, world.item_count, "item count should be 0");
    ASSERT_EQ(0, world.current_room, "current room should be 0");

    // Check inventory is empty
    for (int i = 0; i < MAX_INVENTORY; i++) {
        ASSERT_EQ(-1, world.inventory[i], "inventory slot should be -1");
    }

    PASS();
}

// Test room creation
void test_room_creation(void) {
    TEST("Room creation");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "entrance", "Entrance Hall", "A grand entrance hall.");
    ASSERT_EQ(0, room1, "first room should be index 0");
    ASSERT_EQ(1, world.room_count, "room count should be 1");

    int room2 = world_add_room(&world, "hall", "Great Hall", "A massive hall.");
    ASSERT_EQ(1, room2, "second room should be index 1");
    ASSERT_EQ(2, world.room_count, "room count should be 2");

    // Verify room data
    ASSERT_STR_EQ("entrance", world.rooms[0].id, "room 0 id");
    ASSERT_STR_EQ("Entrance Hall", world.rooms[0].name, "room 0 name");
    ASSERT_STR_EQ("A grand entrance hall.", world.rooms[0].description, "room 0 description");

    PASS();
}

// Test item creation
void test_item_creation(void) {
    TEST("Item creation");

    World world;
    world_init(&world);

    int item1 = world_add_item(&world, "key", "rusty key", "An old rusty key.", true);
    ASSERT_EQ(0, item1, "first item should be index 0");
    ASSERT_EQ(1, world.item_count, "item count should be 1");

    int item2 = world_add_item(&world, "statue", "stone statue", "A heavy stone statue.", false);
    ASSERT_EQ(1, item2, "second item should be index 1");
    ASSERT_EQ(2, world.item_count, "item count should be 2");

    // Verify item data
    ASSERT_STR_EQ("key", world.items[0].id, "item 0 id");
    ASSERT_STR_EQ("rusty key", world.items[0].name, "item 0 name");
    ASSERT_TRUE(world.items[0].takeable, "item 0 should be takeable");
    ASSERT_FALSE(world.items[1].takeable, "item 1 should not be takeable");

    PASS();
}

// Test room connections
void test_room_connections(void) {
    TEST("Room connections");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");

    // Connect rooms
    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_connect_rooms(&world, room2, DIR_SOUTH, room1);

    // Verify connections
    ASSERT_EQ(room2, world.rooms[room1].exits[DIR_NORTH], "room1 north exit");
    ASSERT_EQ(-1, world.rooms[room1].exits[DIR_SOUTH], "room1 south exit");
    ASSERT_EQ(room1, world.rooms[room2].exits[DIR_SOUTH], "room2 south exit");
    ASSERT_EQ(-1, world.rooms[room2].exits[DIR_NORTH], "room2 north exit");

    PASS();
}

// Test navigation
void test_navigation(void) {
    TEST("Navigation");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_connect_rooms(&world, room2, DIR_SOUTH, room1);

    world.current_room = room1;

    // Move north (should succeed)
    ASSERT_TRUE(world_move(&world, DIR_NORTH), "move north should succeed");
    ASSERT_EQ(room2, world.current_room, "should be in room2");

    // Move north again (should fail - no exit)
    ASSERT_FALSE(world_move(&world, DIR_NORTH), "move north should fail");
    ASSERT_EQ(room2, world.current_room, "should still be in room2");

    // Move south (should succeed)
    ASSERT_TRUE(world_move(&world, DIR_SOUTH), "move south should succeed");
    ASSERT_EQ(room1, world.current_room, "should be back in room1");

    PASS();
}

// Test item placement and discovery
void test_item_placement(void) {
    TEST("Item placement");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int item1 = world_add_item(&world, "key", "rusty key", "An old key.", true);

    // Place item in room
    world_place_item(&world, item1, room1);

    // Verify item is in room
    ASSERT_EQ(item1, world.rooms[room1].items[0], "item should be in room");
    ASSERT_TRUE(world.items[item1].visible, "item should be visible");

    PASS();
}

// Test taking items
void test_take_items(void) {
    TEST("Taking items");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int item1 = world_add_item(&world, "key", "rusty key", "An old key.", true);
    int item2 = world_add_item(&world, "statue", "stone statue", "Heavy statue.", false);

    world.current_room = room1;
    world_place_item(&world, item1, room1);
    world_place_item(&world, item2, room1);

    // Take takeable item (should succeed)
    ASSERT_TRUE(world_take_item(&world, "key"), "take key should succeed");
    ASSERT_EQ(item1, world.inventory[0], "key should be in inventory");

    // Try to take non-takeable item (should fail)
    ASSERT_FALSE(world_take_item(&world, "statue"), "take statue should fail");

    // Try to take item not in room
    ASSERT_FALSE(world_take_item(&world, "sword"), "take nonexistent item should fail");

    PASS();
}

// Test dropping items
void test_drop_items(void) {
    TEST("Dropping items");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int item1 = world_add_item(&world, "key", "rusty key", "An old key.", true);

    world.current_room = room1;
    world.inventory[0] = item1;

    // Drop item (should succeed)
    ASSERT_TRUE(world_drop_item(&world, "key"), "drop key should succeed");
    ASSERT_EQ(-1, world.inventory[0], "inventory should be empty");

    // Verify item is in room
    bool found = false;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (world.rooms[room1].items[i] == item1) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found, "item should be in room");

    // Try to drop item not in inventory
    ASSERT_FALSE(world_drop_item(&world, "sword"), "drop nonexistent item should fail");

    PASS();
}

// Test inventory management
void test_inventory_management(void) {
    TEST("Inventory management");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int key = world_add_item(&world, "key", "rusty key", "An old key.", true);
    int sword = world_add_item(&world, "sword", "iron sword", "A sharp sword.", true);

    world.current_room = room1;
    world_place_item(&world, key, room1);
    world_place_item(&world, sword, room1);

    // Initially should not have items
    ASSERT_FALSE(world_has_item(&world, "key"), "should not have key initially");
    ASSERT_FALSE(world_has_item(&world, "sword"), "should not have sword initially");

    // Take items
    world_take_item(&world, "key");
    world_take_item(&world, "sword");

    // Should now have items
    ASSERT_TRUE(world_has_item(&world, "key"), "should have key");
    ASSERT_TRUE(world_has_item(&world, "sword"), "should have sword");

    // Get items from inventory
    Item *key_item = world_get_inventory_item(&world, "key");
    ASSERT_NOT_NULL(key_item, "should get key from inventory");
    ASSERT_STR_EQ("rusty key", key_item->name, "key name should match");

    PASS();
}

// Test direction string conversion
void test_direction_conversion(void) {
    TEST("Direction string conversion");

    // Test valid directions
    ASSERT_EQ(DIR_NORTH, str_to_direction("north"), "north");
    ASSERT_EQ(DIR_NORTH, str_to_direction("n"), "n");
    ASSERT_EQ(DIR_SOUTH, str_to_direction("south"), "south");
    ASSERT_EQ(DIR_SOUTH, str_to_direction("s"), "s");
    ASSERT_EQ(DIR_EAST, str_to_direction("east"), "east");
    ASSERT_EQ(DIR_EAST, str_to_direction("e"), "e");
    ASSERT_EQ(DIR_WEST, str_to_direction("west"), "west");
    ASSERT_EQ(DIR_WEST, str_to_direction("w"), "w");
    ASSERT_EQ(DIR_UP, str_to_direction("up"), "up");
    ASSERT_EQ(DIR_UP, str_to_direction("u"), "u");
    ASSERT_EQ(DIR_DOWN, str_to_direction("down"), "down");
    ASSERT_EQ(DIR_DOWN, str_to_direction("d"), "d");

    // Test invalid direction
    ASSERT_EQ(-1, str_to_direction("invalid"), "invalid direction");
    ASSERT_EQ(-1, str_to_direction(""), "empty direction");

    // Test direction to string
    ASSERT_STR_EQ("north", direction_to_str(DIR_NORTH), "DIR_NORTH to string");
    ASSERT_STR_EQ("south", direction_to_str(DIR_SOUTH), "DIR_SOUTH to string");
    ASSERT_STR_EQ("east", direction_to_str(DIR_EAST), "DIR_EAST to string");
    ASSERT_STR_EQ("west", direction_to_str(DIR_WEST), "DIR_WEST to string");
    ASSERT_STR_EQ("up", direction_to_str(DIR_UP), "DIR_UP to string");
    ASSERT_STR_EQ("down", direction_to_str(DIR_DOWN), "DIR_DOWN to string");

    PASS();
}

// Test room visited tracking
void test_room_visited(void) {
    TEST("Room visited tracking");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");

    // Initially not visited
    ASSERT_FALSE(world.rooms[room1].visited, "room1 not visited");
    ASSERT_FALSE(world.rooms[room2].visited, "room2 not visited");

    // Mark as visited
    world.rooms[room1].visited = true;
    ASSERT_TRUE(world.rooms[room1].visited, "room1 visited");
    ASSERT_FALSE(world.rooms[room2].visited, "room2 still not visited");

    PASS();
}

// Main test runner
int main(void) {
    printf("\n=== World System Test Suite ===\n\n");

    test_world_init();
    test_room_creation();
    test_item_creation();
    test_room_connections();
    test_navigation();
    test_item_placement();
    test_take_items();
    test_drop_items();
    test_inventory_management();
    test_direction_conversion();
    test_room_visited();

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
