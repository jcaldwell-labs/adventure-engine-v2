/*
 * Unit Tests for Issue #8: Use Command
 * Tests item use_message and use_consumable functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

// Test macros
#define TEST(name) printf("  Testing: %s ... ", name)
#define PASS() do { printf("\033[32m✓ PASS\033[0m\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("\033[31m✗ FAIL: %s\033[0m\n", msg); tests_failed++; } while(0)

#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_FALSE(cond, msg) do { if (cond) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        char err[256]; \
        snprintf(err, sizeof(err), "%s (expected: %d, got: %d)", msg, (int)(expected), (int)(actual)); \
        FAIL(err); \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        char err[256]; \
        snprintf(err, sizeof(err), "%s (expected: '%s', got: '%s')", msg, expected, actual); \
        FAIL(err); \
        return; \
    } \
} while(0)

// Test: Item use_message initialization
void test_use_message_init(void) {
    TEST("Item use_message initialization");

    World world;
    world_init(&world);

    int item_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    ASSERT_TRUE(item_idx >= 0, "should create item");

    Item *item = &world.items[item_idx];
    ASSERT_EQ('\0', item->use_message[0], "use_message should be empty by default");
    ASSERT_FALSE(item->use_consumable, "use_consumable should be false by default");

    PASS();
}

// Test: Set use_message on item
void test_set_use_message(void) {
    TEST("Setting use_message on item");

    World world;
    world_init(&world);

    int item_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    ASSERT_TRUE(item_idx >= 0, "should create item");

    Item *item = &world.items[item_idx];
    strncpy(item->use_message, "You drink the potion and feel refreshed!", sizeof(item->use_message) - 1);
    item->use_consumable = true;

    ASSERT_STR_EQ("You drink the potion and feel refreshed!", item->use_message, "use_message should be set");
    ASSERT_TRUE(item->use_consumable, "use_consumable should be true");

    PASS();
}

// Test: Check if item is usable (has use_message)
void test_item_is_usable(void) {
    TEST("Item usability check");

    World world;
    world_init(&world);

    // Non-usable item (empty use_message)
    int key_idx = world_add_item(&world, "key", "rusty key", "An old key.", true);
    Item *key = &world.items[key_idx];
    ASSERT_EQ('\0', key->use_message[0], "key should not be usable");

    // Usable item
    int potion_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    Item *potion = &world.items[potion_idx];
    strncpy(potion->use_message, "You drink the potion.", sizeof(potion->use_message) - 1);
    ASSERT_TRUE(potion->use_message[0] != '\0', "potion should be usable");

    PASS();
}

// Test: world_remove_from_inventory function
void test_remove_from_inventory(void) {
    TEST("Remove item from inventory");

    World world;
    world_init(&world);

    // Set up room and item
    world_add_room(&world, "room1", "Test Room", "A test room.");
    int item_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    world_place_item(&world, item_idx, 0);

    // Take item
    ASSERT_TRUE(world_take_item(&world, "potion"), "should take potion");
    ASSERT_TRUE(world_has_item(&world, "potion"), "should have potion in inventory");

    // Remove from inventory
    ASSERT_TRUE(world_remove_from_inventory(&world, "potion"), "should remove potion");
    ASSERT_FALSE(world_has_item(&world, "potion"), "should not have potion after removal");

    PASS();
}

// Test: Remove non-existent item from inventory
void test_remove_nonexistent_item(void) {
    TEST("Remove non-existent item returns false");

    World world;
    world_init(&world);

    ASSERT_FALSE(world_remove_from_inventory(&world, "nonexistent"), "should return false for non-existent item");

    PASS();
}

// Test: Consumable item removed after use
void test_consumable_flag(void) {
    TEST("Consumable flag behavior");

    World world;
    world_init(&world);

    // Create two items: consumable and non-consumable
    int potion_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    Item *potion = &world.items[potion_idx];
    strncpy(potion->use_message, "You drink the potion.", sizeof(potion->use_message) - 1);
    potion->use_consumable = true;

    int torch_idx = world_add_item(&world, "torch", "burning torch", "A torch.", true);
    Item *torch = &world.items[torch_idx];
    strncpy(torch->use_message, "The torch illuminates the area.", sizeof(torch->use_message) - 1);
    torch->use_consumable = false;

    ASSERT_TRUE(potion->use_consumable, "potion should be consumable");
    ASSERT_FALSE(torch->use_consumable, "torch should not be consumable");

    PASS();
}

// Test: Get inventory item by ID
void test_get_inventory_item(void) {
    TEST("Get inventory item by ID");

    World world;
    world_init(&world);

    // Set up room and item
    world_add_room(&world, "room1", "Test Room", "A test room.");
    int item_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    world_place_item(&world, item_idx, 0);

    // Before taking, should not be in inventory
    ASSERT_TRUE(world_get_inventory_item(&world, "potion") == NULL, "potion should not be in inventory yet");

    // Take item
    world_take_item(&world, "potion");

    // Now should be in inventory
    Item *item = world_get_inventory_item(&world, "potion");
    ASSERT_TRUE(item != NULL, "should find potion in inventory");
    ASSERT_STR_EQ("potion", item->id, "item id should match");

    PASS();
}

// Test: Multiple items with different use properties
void test_multiple_usable_items(void) {
    TEST("Multiple items with different use properties");

    World world;
    world_init(&world);

    world_add_room(&world, "room1", "Test Room", "A test room.");

    // Create various items
    int key_idx = world_add_item(&world, "key", "rusty key", "An old key.", true);
    int potion_idx = world_add_item(&world, "potion", "healing potion", "A red potion.", true);
    int scroll_idx = world_add_item(&world, "scroll", "magic scroll", "A glowing scroll.", true);
    int torch_idx = world_add_item(&world, "torch", "burning torch", "A torch.", true);

    Item *key = &world.items[key_idx];
    Item *potion = &world.items[potion_idx];
    Item *scroll = &world.items[scroll_idx];
    Item *torch = &world.items[torch_idx];

    // Key: not usable
    // (default - no use_message)

    // Potion: usable and consumable
    strncpy(potion->use_message, "You drink the potion.", sizeof(potion->use_message) - 1);
    potion->use_consumable = true;

    // Scroll: usable and consumable
    strncpy(scroll->use_message, "You read the scroll. It crumbles to dust.", sizeof(scroll->use_message) - 1);
    scroll->use_consumable = true;

    // Torch: usable but not consumable
    strncpy(torch->use_message, "The torch illuminates the darkness.", sizeof(torch->use_message) - 1);
    torch->use_consumable = false;

    // Verify
    ASSERT_EQ('\0', key->use_message[0], "key should not be usable");
    ASSERT_TRUE(potion->use_message[0] != '\0', "potion should be usable");
    ASSERT_TRUE(potion->use_consumable, "potion should be consumable");
    ASSERT_TRUE(scroll->use_message[0] != '\0', "scroll should be usable");
    ASSERT_TRUE(scroll->use_consumable, "scroll should be consumable");
    ASSERT_TRUE(torch->use_message[0] != '\0', "torch should be usable");
    ASSERT_FALSE(torch->use_consumable, "torch should not be consumable");

    PASS();
}

int main(void) {
    printf("\n=== Use Command Test Suite (Issue #8) ===\n\n");

    test_use_message_init();
    test_set_use_message();
    test_item_is_usable();
    test_remove_from_inventory();
    test_remove_nonexistent_item();
    test_consumable_flag();
    test_get_inventory_item();
    test_multiple_usable_items();

    printf("\n=== Test Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("\n");

    if (tests_failed == 0) {
        printf("\033[32m✓ All tests passed!\033[0m\n\n");
        return 0;
    } else {
        printf("\033[31m✗ Some tests failed!\033[0m\n\n");
        return 1;
    }
}
