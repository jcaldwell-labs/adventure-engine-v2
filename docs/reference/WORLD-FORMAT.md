# Adventure Engine World File Format

Version 1.0

## Overview

World files define the rooms, items, and connections for an adventure game. They use a simple, human-readable text format that's easy to write and parse.

## File Extension

World files use the `.world` extension (e.g., `dark_tower.world`)

## Format Specification

### Basic Structure

```
# Comments start with #
# Blank lines are ignored

[WORLD]
name: World Name
author: Author Name
version: 1.0

[ROOM:room_id]
name: Room Name
description: Room description text
exits: direction1=room_id1, direction2=room_id2

[ITEM:item_id]
name: Item Name
description: Item description text
takeable: yes/no
location: room_id
```

### Sections

#### [WORLD] Section (Required)

Defines metadata about the world. Must appear once at the beginning.

**Properties:**

- `name` - Display name of the world (required)
- `author` - Creator name (optional)
- `version` - Version string (optional)
- `start` - Starting room ID (optional, defaults to first room)

**Example:**

```
[WORLD]
name: The Dark Tower
author: Unknown
version: 1.0
start: entrance
```

#### [ROOM:id] Section

Defines a room. Can appear multiple times.

**Properties:**

- `name` - Short room name (required, max 64 chars)
- `description` - Full room description (required, max 512 chars)
- `exits` - Comma-separated list of direction=room_id pairs (optional)
- `locked_exits` - Comma-separated list of direction=item_id pairs for locked doors (optional)

**Directions:** north, south, east, west, up, down

**Locked Exits:**
When a direction is listed in `locked_exits`, the player must have the specified item in their inventory to pass through. The door auto-unlocks when the player has the key and attempts to move through, and stays unlocked for the rest of the game.

**Example:**

```
[ROOM:entrance]
name: Tower Entrance
description: You stand before a massive dark tower. Its stone walls are cold and ancient. A heavy wooden door stands ajar to the north.
exits: north=hall

[ROOM:hall]
name: Grand Hall
description: A vast hall with high vaulted ceilings. Torches flicker on the walls, casting dancing shadows.
exits: south=entrance, east=chamber, up=tower_top
locked_exits: east=chamber_key
```

In this example, the east exit to the chamber is locked and requires the `chamber_key` item to unlock.

#### [ITEM:id] Section

Defines an item. Can appear multiple times.

**Properties:**

- `name` - Item name (required, max 64 chars)
- `description` - Full description shown with "examine" (required, max 256 chars)
- `takeable` - Can be picked up? (required: yes/no/true/false/1/0)
- `location` - Room ID where item starts (required)

**Example:**

```
[ITEM:key]
name: rusty key
description: An old iron key, covered in rust but still functional.
takeable: yes
location: entrance

[ITEM:statue]
name: stone statue
description: A heavy stone statue of a forgotten king. Too heavy to move.
takeable: no
location: hall
```

## Multi-word Item IDs

Item IDs should be single words (no spaces), but item names can be multi-word:

```
[ITEM:rusty_key]
name: rusty key
description: An old key covered in rust.
takeable: yes
location: entrance
```

Players can refer to it as "key", "rusty key", or "rusty_key".

## Validation Rules

1. **Unique IDs**: All room and item IDs must be unique
2. **Exit Consistency**: Exits should reference valid room IDs
3. **Item Locations**: Items must reference valid room IDs
4. **Required Fields**: All required properties must be present
5. **Start Room**: Must be a valid room ID (if specified)

## Error Reporting

The world loader reports errors with:

- File name
- Line number
- Error description

Example:

```
ERROR: dark_tower.world:15: Room 'hall' has invalid exit to 'nowhere'
```

## Example Complete World

```
# The Dark Tower - A simple demo adventure

[WORLD]
name: The Dark Tower
author: Adventure Engine Team
version: 1.0
start: entrance

[ROOM:entrance]
name: Tower Entrance
description: You stand before a massive dark tower. Its stone walls are cold and ancient. A heavy wooden door stands ajar to the north.
exits: north=hall

[ROOM:hall]
name: Grand Hall
description: A vast hall with high vaulted ceilings. Torches flicker on the walls, casting dancing shadows. Stone stairs lead up to the east. The exit is to the south.
exits: south=entrance, east=chamber

[ROOM:chamber]
name: Treasure Chamber
description: A small chamber filled with ancient artifacts. Dust covers everything. Stairs lead down to the west.
exits: west=hall

[ITEM:key]
name: rusty key
description: An old iron key, covered in rust but still functional.
takeable: yes
location: entrance

[ITEM:torch]
name: burning torch
description: A wooden torch with flames that never seem to die.
takeable: yes
location: hall

[ITEM:statue]
name: stone statue
description: A heavy stone statue of a forgotten king. Too heavy to move.
takeable: no
location: hall

[ITEM:gem]
name: glowing gem
description: A brilliant blue gem that pulses with inner light.
takeable: yes
location: chamber
```

## Future Extensions

Planned features for future versions:

- **NPCs**: Character definitions with dialogue
- **Locked Exits**: Doors requiring keys or conditions
- **Item Usage**: Scripts for item interactions
- **Triggers**: Events that change world state
- **Variables**: Global and local state tracking

## Best Practices

1. **Use descriptive IDs**: `rusty_key` not `key1`
2. **Write vivid descriptions**: Engage player imagination
3. **Test exit consistency**: Every exit should be bidirectional (unless one-way by design)
4. **Balance item distribution**: Spread items across rooms
5. **Comment your world**: Use # comments to organize sections
6. **Version your worlds**: Update version number when making changes
