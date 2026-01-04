/*
 * Adventure Engine - Locked Exits Test Suite
 * Tests for Issue #5: Locked doors and key mechanics
 */

#include <stdio.h>
#include <string.h>
#include "world.h"

// Test macros
#define TEST(name) printf("  Testing: %s ... ", name)
#define PASS() printf("\033[32m✓ PASS\033[0m\n"); passed++
#define FAIL(msg) printf("\033[31m✗ FAIL: %s\033[0m\n", msg); failed++

static int passed = 0;
static int failed = 0;

// Test: Room locked exits initialize empty
void test_locked_exits_init(void) {
    TEST("Locked exits initialization");

    World world;
    world_init(&world);

    int room_idx = world_add_room(&world, "test", "Test Room", "A test room.");

    // All locked_exits should be empty strings
    for (int i = 0; i < DIR_COUNT; i++) {
        if (world.rooms[room_idx].locked_exits[i][0] != '\0') {
            FAIL("locked_exits should initialize to empty");
            return;
        }
        if (world.rooms[room_idx].exit_unlocked[i] != false) {
            FAIL("exit_unlocked should initialize to false");
            return;
        }
    }

    PASS();
}

// Test: Lock an exit
void test_lock_exit(void) {
    TEST("Lock exit with key");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");
    int key = world_add_item(&world, "iron_key", "iron key", "A heavy iron key.", true);

    // Connect rooms
    world_connect_rooms(&world, room1, DIR_NORTH, room2);

    // Lock the exit
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    // Verify lock was set
    if (strcmp(world.rooms[room1].locked_exits[DIR_NORTH], "iron_key") != 0) {
        FAIL("locked_exits should contain key ID");
        return;
    }

    (void)key; // Unused variable
    PASS();
}

// Test: Locked exit blocks movement without key
void test_locked_exit_blocks_movement(void) {
    TEST("Locked exit blocks movement");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");
    world_add_item(&world, "iron_key", "iron key", "A heavy iron key.", true);

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    world.current_room = room1;

    // Try to move without key - should fail
    char key_needed[32];
    MoveResult result = world_move_ex(&world, DIR_NORTH, key_needed, sizeof(key_needed));

    if (result != MOVE_LOCKED) {
        FAIL("Should return MOVE_LOCKED when key not in inventory");
        return;
    }

    if (strcmp(key_needed, "iron_key") != 0) {
        FAIL("Should return required key ID");
        return;
    }

    // Player should still be in room1
    if (world.current_room != room1) {
        FAIL("Player should not have moved");
        return;
    }

    PASS();
}

// Test: Locked exit allows movement with key
void test_locked_exit_with_key(void) {
    TEST("Locked exit allows movement with key");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");
    int key = world_add_item(&world, "iron_key", "iron key", "A heavy iron key.", true);

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    world.current_room = room1;

    // Put key in inventory
    world.inventory[0] = key;

    // Try to move with key - should succeed
    char key_needed[32];
    MoveResult result = world_move_ex(&world, DIR_NORTH, key_needed, sizeof(key_needed));

    if (result != MOVE_SUCCESS) {
        FAIL("Should return MOVE_SUCCESS when key in inventory");
        return;
    }

    // Player should be in room2
    if (world.current_room != room2) {
        FAIL("Player should have moved to room2");
        return;
    }

    PASS();
}

// Test: Unlocked exit stays unlocked
void test_exit_stays_unlocked(void) {
    TEST("Exit stays unlocked after first use");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");
    int key = world_add_item(&world, "iron_key", "iron key", "A heavy iron key.", true);

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_connect_rooms(&world, room2, DIR_SOUTH, room1);
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    world.current_room = room1;
    world.inventory[0] = key;

    // First move - unlocks the door
    world_move_ex(&world, DIR_NORTH, NULL, 0);

    // Go back
    world_move(&world, DIR_SOUTH);

    // Drop the key
    world.inventory[0] = -1;

    // Try to go north again without key - should still work
    char key_needed[32];
    MoveResult result = world_move_ex(&world, DIR_NORTH, key_needed, sizeof(key_needed));

    if (result != MOVE_SUCCESS) {
        FAIL("Should still be able to pass after exit unlocked");
        return;
    }

    PASS();
}

// Test: world_exit_is_locked function
void test_exit_is_locked(void) {
    TEST("world_exit_is_locked function");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world.current_room = room1;

    // Not locked initially
    if (world_exit_is_locked(&world, DIR_NORTH)) {
        FAIL("Exit should not be locked initially");
        return;
    }

    // Lock it
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    if (!world_exit_is_locked(&world, DIR_NORTH)) {
        FAIL("Exit should be locked after world_lock_exit");
        return;
    }

    // Unlock it
    world_unlock_exit(&world, room1, DIR_NORTH);

    if (world_exit_is_locked(&world, DIR_NORTH)) {
        FAIL("Exit should not be locked after world_unlock_exit");
        return;
    }

    PASS();
}

// Test: world_get_required_key function
void test_get_required_key(void) {
    TEST("world_get_required_key function");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_lock_exit(&world, room1, DIR_NORTH, "golden_key");
    world.current_room = room1;

    const char *key = world_get_required_key(&world, DIR_NORTH);

    if (key == NULL) {
        FAIL("Should return key ID");
        return;
    }

    if (strcmp(key, "golden_key") != 0) {
        FAIL("Should return correct key ID");
        return;
    }

    // No key for east
    const char *no_key = world_get_required_key(&world, DIR_EAST);
    if (no_key != NULL) {
        FAIL("Should return NULL for unlocked direction");
        return;
    }

    PASS();
}

// Test: Regular world_move still works
void test_world_move_compatibility(void) {
    TEST("world_move backward compatibility");

    World world;
    world_init(&world);

    int room1 = world_add_room(&world, "room1", "Room 1", "First room.");
    int room2 = world_add_room(&world, "room2", "Room 2", "Second room.");
    int key = world_add_item(&world, "iron_key", "iron key", "A heavy iron key.", true);

    world_connect_rooms(&world, room1, DIR_NORTH, room2);
    world_lock_exit(&world, room1, DIR_NORTH, "iron_key");

    world.current_room = room1;

    // Without key - should fail
    if (world_move(&world, DIR_NORTH)) {
        FAIL("world_move should return false when locked");
        return;
    }

    // With key - should succeed
    world.inventory[0] = key;
    if (!world_move(&world, DIR_NORTH)) {
        FAIL("world_move should return true when key present");
        return;
    }

    PASS();
}

int main(void) {
    printf("\n=== Locked Exits Test Suite (Issue #5) ===\n\n");

    test_locked_exits_init();
    test_lock_exit();
    test_locked_exit_blocks_movement();
    test_locked_exit_with_key();
    test_exit_stays_unlocked();
    test_exit_is_locked();
    test_get_required_key();
    test_world_move_compatibility();

    printf("\n=== Test Results ===\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("\n");

    if (failed > 0) {
        printf("\033[31m✗ Some tests failed!\033[0m\n\n");
        return 1;
    }

    printf("\033[32m✓ All tests passed!\033[0m\n\n");
    return 0;
}
