# Adventure Engine World File Format

Version 1.2

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
- `description_if(condition)` - Conditional descriptions based on game state (optional, max 8 per room)

**Directions:** north, south, east, west, up, down

**Locked Exits:**
When a direction is listed in `locked_exits`, the player must have the specified item in their inventory to pass through. The door auto-unlocks when the player has the key and attempts to move through, and stays unlocked for the rest of the game.

**Conditional Descriptions:**
Rooms can have multiple conditional descriptions that display based on game state. When a condition matches, its description replaces the default. Conditions are evaluated in priority order:

1. `item_used=item_id` - Highest priority. Shows when the specified item has been used.
2. `has_item=item_id` - Shows when player has item in inventory.
3. `room_has_item=item_id` - Shows when item is present in this room.
4. `first_visit` - Shows only on first visit (room not yet marked as visited).
5. `visited` - Shows on return visits (room has been visited before).

Conditions can be negated with `!` prefix (e.g., `!has_item=lantern`).

**Example:**

```
[ROOM:entrance]
name: Tower Entrance
description: You stand before a massive dark tower. Its stone walls are cold and ancient. A heavy wooden door stands ajar to the north.
exits: north=hall

[ROOM:hall]
name: Grand Hall
description: A vast hall with high vaulted ceilings. Torches flicker on the walls.
exits: south=entrance, east=chamber, up=tower_top
locked_exits: east=chamber_key

[ROOM:cellar]
name: Dark Cellar
description: A damp, dark cellar. You can barely see anything.
description_if(first_visit): You descend into the cellar for the first time. The darkness is oppressive.
description_if(visited): The familiar musty smell greets your return.
description_if(has_item=lantern): Your lantern illuminates dusty wine racks and cobwebs.
description_if(!has_item=lantern): It's quite dim here without a light source.
description_if(room_has_item=coin): Something glints on the floor.
exits: up=hall
```

In the example above:

- First visit shows the special "first time" description
- Return visits show "familiar musty smell" unless the player has the lantern
- Having the lantern overrides the visit-based descriptions
- If a coin is in the room, that description takes priority over visit-based ones

#### [ITEM:id] Section

Defines an item. Can appear multiple times.

**Properties:**

- `name` - Item name (required, max 64 chars)
- `description` - Full description shown with "examine" (required, max 256 chars)
- `takeable` - Can be picked up? (required: yes/no/true/false/1/0)
- `location` - Room ID where item starts (required)
- `use_message` - Message shown when item is used (optional, max 256 chars)
- `use_consumable` - Is item consumed after use? (optional: yes/no, default: no)

**Usable Items:**
Items with a `use_message` can be used by the player with the "use" command. If `use_consumable` is set to `yes`, the item is removed from inventory after use. Items without a `use_message` cannot be used.

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

[ITEM:healing_potion]
name: healing potion
description: A red potion that glows with restorative magic.
takeable: yes
location: entrance
use_message: You drink the healing potion. Warmth flows through your body!
use_consumable: yes

[ITEM:magic_torch]
name: magic torch
description: A torch with an eternal flame.
takeable: yes
location: entrance
use_message: You hold up the torch. Its light reveals hidden details in the walls.
use_consumable: no
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
- ~~**Locked Exits**: Doors requiring keys or conditions~~ (Implemented in v1.1)
- ~~**Item Usage**: Scripts for item interactions~~ (Implemented in v1.1)
- ~~**Conditional Descriptions**: Room descriptions that change based on state~~ (Implemented in v1.2)
- **Triggers**: Events that change world state
- **Variables**: Global and local state tracking

## Best Practices

1. **Use descriptive IDs**: `rusty_key` not `key1`
2. **Write vivid descriptions**: Engage player imagination
3. **Test exit consistency**: Every exit should be bidirectional (unless one-way by design)
4. **Balance item distribution**: Spread items across rooms
5. **Comment your world**: Use # comments to organize sections
6. **Version your worlds**: Update version number when making changes
