# Deep Dive: Boxes-Live Visual Integration

**Status**: Implementation Blueprint
**Complexity**: Medium-High
**Estimated Effort**: 8-12 hours
**Priority**: Medium (visual enhancement)

---

## Overview

[Boxes](http://boxes.thomasjensen.com/) is a powerful command-line tool for creating ASCII art boxes and frames. Integrating it with Adventure Engine will provide:

1. **Professional ASCII art borders** around panels
2. **Themed visual styles** per realm or campaign
3. **Dynamic box generation** from templates
4. **Status indicators** with visual flair
5. **Enhanced readability** through visual separation
6. **Customizable themes** for different moods

---

## Boxes Overview

### What is Boxes?

Boxes draws ASCII art boxes around text. Example:

**Input**:
```
Hello, World!
This is a test.
```

**Output (stone box)**:
```
┌─────────────────┐
│ Hello, World!   │
│ This is a test. │
└─────────────────┘
```

### Box Styles

Boxes supports 50+ built-in styles:

```
stone:      ┌─┐ │ └─┘
double:     ╔═╗ ║ ╚═╝
ada-box:    -+ | +-
columns:    │ │ │
html:       <!-- -->
c-cmt:      /* */
```

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install boxes

# macOS
brew install boxes

# From source
git clone https://github.com/ascii-boxes/boxes
cd boxes && make && sudo make install
```

---

## Integration Architecture

### 1. Box Rendering Pipeline

```
TEXT CONTENT
    │
    ▼
┌─────────────────┐
│ Template Select │  (based on context: narrative, map, stats, etc.)
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ Variable Subst  │  (replace $TITLE, $TIMESTAMP, etc.)
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ Boxes Command   │  boxes -d <design> -p <padding>
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ ANSI Coloring   │  (add colors with escape codes)
└─────────────────┘
    │
    ▼
RENDERED OUTPUT
```

### 2. Component Structure

**File**: `include/visual.h`

```c
#ifndef VISUAL_H
#define VISUAL_H

#include <stdbool.h>

#define MAX_BOX_WIDTH 120
#define MAX_BOX_HEIGHT 50
#define MAX_BOX_DESIGN_NAME 32
#define MAX_BOX_CONTENT 8192

// Box design types
typedef enum {
    BOX_STONE,          // ┌─┐ classic stone box
    BOX_DOUBLE,         // ╔═╗ double-line box
    BOX_THICK,          // ┏━┓ thick box
    BOX_ROUND,          // ╭─╮ rounded corners
    BOX_ASCII,          // +-+ pure ASCII (no unicode)
    BOX_COLUMNS,        // │ │ column separators
    BOX_SPRING,         // Decorative spring design
    BOX_SANTA,          // Christmas theme
    BOX_SCROLL,         // Scroll/parchment
    BOX_DIAMONDS,       // Diamond pattern
    BOX_CUSTOM          // Custom .boxes file
} BoxDesign;

// Box alignment
typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} BoxAlignment;

// Color schemes
typedef enum {
    COLOR_NONE,
    COLOR_NARRATIVE,    // Warm colors for story
    COLOR_MAP,          // Cool colors for map
    COLOR_STATS,        // Neutral for stats
    COLOR_LOG,          // Muted for logs
    COLOR_ERROR,        // Red for errors
    COLOR_SUCCESS,      // Green for success
    COLOR_WARNING,      // Yellow for warnings
    COLOR_CUSTOM        // Custom ANSI codes
} ColorScheme;

// Box configuration
typedef struct {
    BoxDesign design;
    int width;              // 0 = auto-width
    int height;             // 0 = auto-height
    int padding;            // Internal padding
    BoxAlignment alignment;
    bool remove_blank_lines;
    bool kill_blank_lines;
    ColorScheme color_scheme;
    char custom_color[32];  // ANSI escape code
} BoxConfig;

// Rendered box
typedef struct {
    char content[MAX_BOX_CONTENT];
    int width;
    int height;
    int line_count;
} RenderedBox;

// Box templates for different contexts
typedef struct {
    char name[64];
    char title_format[128];  // e.g., "=== %s ==="
    BoxConfig config;
    char header_text[256];
    char footer_text[256];
} BoxTemplate;

// Visual manager
typedef struct {
    BoxTemplate templates[20];
    int template_count;

    char boxes_command[256];  // Path to boxes binary
    bool boxes_available;

    // Cache rendered boxes for performance
    RenderedBox cache[10];
    int cache_count;

} VisualManager;

// Initialization
VisualManager* visual_manager_create(void);
void visual_manager_destroy(VisualManager* manager);
bool visual_check_boxes_installed(VisualManager* manager);

// Box rendering
bool visual_render_box(VisualManager* manager, const char* content,
                      const BoxConfig* config, RenderedBox* out_box);
bool visual_render_with_template(VisualManager* manager, const char* content,
                                 const char* template_name, RenderedBox* out_box);

// Template management
bool visual_add_template(VisualManager* manager, const char* name,
                        BoxDesign design, const char* title_format);
BoxTemplate* visual_get_template(VisualManager* manager, const char* name);

// Utility functions
const char* visual_design_to_string(BoxDesign design);
bool visual_apply_color_scheme(char* content, int max_len, ColorScheme scheme);

// Pre-defined templates
void visual_init_default_templates(VisualManager* manager);

#endif // VISUAL_H
```

---

## Implementation

### 3. Visual Manager Core

**File**: `src/visual.c`

```c
#include "visual.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// ANSI color codes
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"

// Create visual manager
VisualManager* visual_manager_create(void) {
    VisualManager* manager = (VisualManager*)calloc(1, sizeof(VisualManager));
    if (!manager) {
        perror("Failed to allocate visual manager");
        return NULL;
    }

    manager->template_count = 0;
    manager->cache_count = 0;

    // Try to find boxes command
    strcpy(manager->boxes_command, "boxes");

    // Check if boxes is installed
    manager->boxes_available = visual_check_boxes_installed(manager);

    if (manager->boxes_available) {
        printf("[VISUAL] Boxes found: %s\n", manager->boxes_command);
    } else {
        fprintf(stderr, "[VISUAL] Boxes not found - falling back to simple borders\n");
    }

    // Initialize default templates
    visual_init_default_templates(manager);

    return manager;
}

void visual_manager_destroy(VisualManager* manager) {
    if (manager) {
        free(manager);
    }
}

// Check if boxes is installed
bool visual_check_boxes_installed(VisualManager* manager) {
    if (!manager) return false;

    // Try to execute boxes --version
    FILE* fp = popen("boxes --version 2>&1", "r");
    if (!fp) {
        return false;
    }

    char buffer[256];
    bool found = false;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "boxes") != NULL) {
            found = true;
        }
    }

    pclose(fp);
    return found;
}

// Convert design enum to boxes design name
const char* visual_design_to_string(BoxDesign design) {
    switch (design) {
        case BOX_STONE:    return "stone";
        case BOX_DOUBLE:   return "double";
        case BOX_THICK:    return "thick";
        case BOX_ROUND:    return "round";
        case BOX_ASCII:    return "ascii";
        case BOX_COLUMNS:  return "columns";
        case BOX_SPRING:   return "spring";
        case BOX_SANTA:    return "santa";
        case BOX_SCROLL:   return "scroll";
        case BOX_DIAMONDS: return "diamonds";
        default:           return "stone";
    }
}

// Render box using boxes command
bool visual_render_box(VisualManager* manager, const char* content,
                      const BoxConfig* config, RenderedBox* out_box) {
    if (!manager || !content || !out_box) return false;

    // Prepare output buffer
    memset(out_box, 0, sizeof(RenderedBox));

    if (!manager->boxes_available) {
        // Fallback: simple ASCII box
        return visual_render_simple_box(content, config, out_box);
    }

    // Build boxes command
    char command[512];
    snprintf(command, sizeof(command), "boxes -d %s -p a%d",
             visual_design_to_string(config->design),
             config->padding);

    if (config->width > 0) {
        snprintf(command + strlen(command), sizeof(command) - strlen(command),
                " -s %dx%d", config->width, config->height > 0 ? config->height : 1000);
    }

    if (config->alignment != ALIGN_LEFT) {
        const char* align = (config->alignment == ALIGN_CENTER) ? "c" : "r";
        snprintf(command + strlen(command), sizeof(command) - strlen(command),
                " -a %s", align);
    }

    // Create temporary input file
    FILE* input = tmpfile();
    if (!input) {
        perror("Failed to create temp file");
        return false;
    }

    fprintf(input, "%s", content);
    rewind(input);

    // Execute boxes
    char full_command[600];
    snprintf(full_command, sizeof(full_command), "%s", command);

    FILE* output = popen(full_command, "r");
    if (!output) {
        fclose(input);
        return false;
    }

    // Read output
    char line[256];
    int offset = 0;
    int lines = 0;
    while (fgets(line, sizeof(line), output) && offset < MAX_BOX_CONTENT - 256) {
        int len = strlen(line);
        memcpy(out_box->content + offset, line, len);
        offset += len;
        lines++;
    }

    out_box->content[offset] = '\0';
    out_box->line_count = lines;

    pclose(output);
    fclose(input);

    // Apply color scheme
    if (config->color_scheme != COLOR_NONE) {
        visual_apply_color_scheme(out_box->content, MAX_BOX_CONTENT, config->color_scheme);
    }

    return true;
}

// Fallback simple box (when boxes not available)
bool visual_render_simple_box(const char* content, const BoxConfig* config,
                              RenderedBox* out_box) {
    if (!content || !out_box) return false;

    // Calculate width
    int max_line_width = 0;
    const char* line_start = content;
    const char* p = content;

    while (*p) {
        if (*p == '\n') {
            int line_width = p - line_start;
            if (line_width > max_line_width) {
                max_line_width = line_width;
            }
            line_start = p + 1;
        }
        p++;
    }

    int total_width = max_line_width + config->padding * 2 + 2;
    if (config->width > 0 && config->width > total_width) {
        total_width = config->width;
    }

    // Build box
    int offset = 0;

    // Top border
    offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "┌");
    for (int i = 0; i < total_width - 2; i++) {
        offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "─");
    }
    offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "┐\n");

    // Content
    line_start = content;
    p = content;
    while (true) {
        if (*p == '\n' || *p == '\0') {
            // Print line
            int line_width = p - line_start;

            offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "│");

            // Padding
            for (int i = 0; i < config->padding; i++) {
                offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, " ");
            }

            // Content
            for (int i = 0; i < line_width && offset < MAX_BOX_CONTENT - 1; i++) {
                out_box->content[offset++] = line_start[i];
            }

            // Right padding
            int remaining = total_width - 2 - config->padding - line_width;
            for (int i = 0; i < remaining; i++) {
                offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, " ");
            }

            offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "│\n");

            if (*p == '\0') break;
            line_start = p + 1;
        }
        p++;
    }

    // Bottom border
    offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "└");
    for (int i = 0; i < total_width - 2; i++) {
        offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "─");
    }
    offset += snprintf(out_box->content + offset, MAX_BOX_CONTENT - offset, "┘\n");

    return true;
}

// Apply color scheme
bool visual_apply_color_scheme(char* content, int max_len, ColorScheme scheme) {
    if (!content) return false;

    const char* color_code = NULL;

    switch (scheme) {
        case COLOR_NARRATIVE:
            color_code = ANSI_CYAN;
            break;
        case COLOR_MAP:
            color_code = ANSI_GREEN;
            break;
        case COLOR_STATS:
            color_code = ANSI_YELLOW;
            break;
        case COLOR_LOG:
            color_code = ANSI_WHITE;
            break;
        case COLOR_ERROR:
            color_code = ANSI_RED ANSI_BOLD;
            break;
        case COLOR_SUCCESS:
            color_code = ANSI_GREEN ANSI_BOLD;
            break;
        case COLOR_WARNING:
            color_code = ANSI_YELLOW ANSI_BOLD;
            break;
        default:
            return true;  // No color
    }

    // Prepend color code
    int color_len = strlen(color_code);
    int content_len = strlen(content);

    if (color_len + content_len + strlen(ANSI_RESET) >= (size_t)max_len) {
        return false;  // Not enough space
    }

    // Shift content to make room for color code
    memmove(content + color_len, content, content_len + 1);
    memcpy(content, color_code, color_len);

    // Append reset code
    strcat(content, ANSI_RESET);

    return true;
}

// Add template
bool visual_add_template(VisualManager* manager, const char* name,
                        BoxDesign design, const char* title_format) {
    if (!manager || !name || manager->template_count >= 20) {
        return false;
    }

    BoxTemplate* template = &manager->templates[manager->template_count];
    strncpy(template->name, name, 63);
    strncpy(template->title_format, title_format, 127);

    template->config.design = design;
    template->config.width = 0;  // Auto
    template->config.height = 0; // Auto
    template->config.padding = 1;
    template->config.alignment = ALIGN_LEFT;
    template->config.color_scheme = COLOR_NONE;

    manager->template_count++;

    return true;
}

// Get template
BoxTemplate* visual_get_template(VisualManager* manager, const char* name) {
    if (!manager || !name) return NULL;

    for (int i = 0; i < manager->template_count; i++) {
        if (strcmp(manager->templates[i].name, name) == 0) {
            return &manager->templates[i];
        }
    }

    return NULL;
}

// Initialize default templates
void visual_init_default_templates(VisualManager* manager) {
    if (!manager) return;

    // Narrative template
    BoxTemplate* narrative = &manager->templates[manager->template_count++];
    strcpy(narrative->name, "narrative");
    strcpy(narrative->title_format, "=== NARRATIVE ===");
    narrative->config.design = BOX_SCROLL;
    narrative->config.width = 80;
    narrative->config.padding = 2;
    narrative->config.alignment = ALIGN_LEFT;
    narrative->config.color_scheme = COLOR_NARRATIVE;

    // Map template
    BoxTemplate* map = &manager->templates[manager->template_count++];
    strcpy(map->name, "map");
    strcpy(map->title_format, "=== MAP ===");
    map->config.design = BOX_STONE;
    map->config.width = 40;
    map->config.padding = 1;
    map->config.alignment = ALIGN_CENTER;
    map->config.color_scheme = COLOR_MAP;

    // Stats template
    BoxTemplate* stats = &manager->templates[manager->template_count++];
    strcpy(stats->name, "stats");
    strcpy(stats->title_format, "=== TEAM STATUS ===");
    stats->config.design = BOX_DOUBLE;
    stats->config.width = 40;
    stats->config.padding = 1;
    stats->config.alignment = ALIGN_LEFT;
    stats->config.color_scheme = COLOR_STATS;

    // Log template
    BoxTemplate* log = &manager->templates[manager->template_count++];
    strcpy(log->name, "log");
    strcpy(log->title_format, "=== TEAM LOG ===");
    log->config.design = BOX_COLUMNS;
    log->config.width = 40;
    log->config.padding = 1;
    log->config.alignment = ALIGN_LEFT;
    log->config.color_scheme = COLOR_LOG;

    // Error template
    BoxTemplate* error = &manager->templates[manager->template_count++];
    strcpy(error->name, "error");
    strcpy(error->title_format, "!!! ERROR !!!");
    error->config.design = BOX_THICK;
    error->config.width = 60;
    error->config.padding = 2;
    error->config.alignment = ALIGN_CENTER;
    error->config.color_scheme = COLOR_ERROR;
}

// Render with template
bool visual_render_with_template(VisualManager* manager, const char* content,
                                 const char* template_name, RenderedBox* out_box) {
    if (!manager || !content || !template_name || !out_box) {
        return false;
    }

    BoxTemplate* template = visual_get_template(manager, template_name);
    if (!template) {
        fprintf(stderr, "Template not found: %s\n", template_name);
        return false;
    }

    // Add title if specified
    char full_content[MAX_BOX_CONTENT];
    if (template->title_format[0] != '\0') {
        snprintf(full_content, sizeof(full_content), "%s\n\n%s",
                template->title_format, content);
    } else {
        strncpy(full_content, content, MAX_BOX_CONTENT - 1);
    }

    return visual_render_box(manager, full_content, &template->config, out_box);
}
```

---

## Integration with Panels

### 4. Panel Rendering with Boxes

**File**: `panels/narrative.sh` (updated)

```bash
#!/bin/bash
#
# Narrative Panel with Boxes Integration
#

SESSION_ID="$1"
STATE_DIR="/tmp/adventure-campaign-${SESSION_ID}"
PIPE_NARRATIVE="$STATE_DIR/pipe_narrative"

# Check if boxes is available
if command -v boxes &> /dev/null; then
    USE_BOXES=true
else
    USE_BOXES=false
fi

# Function to render with boxes
render_with_boxes() {
    local content="$1"
    local title="$2"

    if [ "$USE_BOXES" = true ]; then
        {
            echo "$title"
            echo ""
            echo "$content"
        } | boxes -d scroll -p a2 -a c
    else
        # Fallback to simple box
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║                      $title                                    ║"
        echo "╠════════════════════════════════════════════════════════════════╣"
        echo "$content" | sed 's/^/║ /' | sed 's/$/║/'
        echo "╚════════════════════════════════════════════════════════════════╝"
    fi
}

# Render narrative content
render_narrative() {
    local title="NARRATIVE PANEL"
    local content=$(cat << 'EOF'
You and your team awaken in a dimly lit stone chamber.
The air is thick with dust and ancient secrets.

A booming voice echoes:
"WELCOME, TRAVELERS. WORK TOGETHER, OR PERISH ALONE."

Three doors appear before you:
  ⚔️  SWORD  - The Path of Valor
  📚 BOOK   - The Path of Wisdom
  🤝 HANDS  - The Path of Unity

Your team must decide...
EOF
)

    render_with_boxes "$content" "$title"
}

# Main loop
clear
exec > "$PIPE_NARRATIVE"

while true; do
    clear
    render_narrative
    sleep 5
done
```

### 5. Custom Box Designs

**File**: `config/custom.boxes`

```
# Adventure Engine Custom Box Designs

BOX adventure
author "Adventure Engine Team"
designer "Adventure Engine Team"
revision "1.0"
created "2025-11-19"
sample
    ⚔═══════════════════════════════⚔
    ║                               ║
    ║   ADVENTURE ENGINE v3.0       ║
    ║                               ║
    ⚔═══════════════════════════════⚔
ends
shapes {
    nw ("⚔") n ("═") ne ("⚔")
    w  ("║") e  ("║")
    sw ("⚔") s ("═") se ("⚔")
}
padding { 1 }
END adventure

BOX quest
sample
    ╭─┤ QUEST ├───────────────────╮
    │                             │
    │  Quest content here...      │
    │                             │
    ╰─────────────────────────────╯
ends
shapes {
    nw ("╭─┤ ") nnw (" ├") n ("─") ne ("╮")
    w  ("│ ") e  (" │")
    sw ("╰") s  ("─") se ("╯")
}
padding { 1 }
END quest

BOX danger
sample
    ⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠
    ⚠  DANGER!          ⚠
    ⚠  Proceed with     ⚠
    ⚠  caution!         ⚠
    ⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠
ends
shapes {
    nw ("⚠") n ("⚠") ne ("⚠")
    w  ("⚠ ") e  (" ⚠")
    sw ("⚠") s ("⚠") se ("⚠")
}
padding { 1 }
END danger
```

To use custom designs:
```bash
boxes -f config/custom.boxes -d adventure < input.txt
```

---

## Advanced Features

### 6. Dynamic Theme Switching

**File**: `src/theme_manager.c`

```c
#include "visual.h"

typedef struct {
    char name[64];
    BoxDesign narrative_design;
    BoxDesign map_design;
    BoxDesign stats_design;
    BoxDesign log_design;
    ColorScheme primary_color;
    ColorScheme secondary_color;
} Theme;

// Pre-defined themes
Theme themes[] = {
    {
        "classic",
        BOX_STONE, BOX_STONE, BOX_DOUBLE, BOX_COLUMNS,
        COLOR_CYAN, COLOR_WHITE
    },
    {
        "fantasy",
        BOX_SCROLL, BOX_ROUND, BOX_DOUBLE, BOX_COLUMNS,
        COLOR_MAGENTA, COLOR_YELLOW
    },
    {
        "scifi",
        BOX_THICK, BOX_ASCII, BOX_ASCII, BOX_COLUMNS,
        COLOR_CYAN, COLOR_GREEN
    },
    {
        "minimal",
        BOX_ASCII, BOX_ASCII, BOX_ASCII, BOX_COLUMNS,
        COLOR_NONE, COLOR_NONE
    },
    {
        "halloween",
        BOX_DIAMONDS, BOX_THICK, BOX_DOUBLE, BOX_THICK,
        COLOR_WARNING, COLOR_ERROR
    }
};

bool theme_apply(VisualManager* manager, const char* theme_name) {
    // Find theme
    Theme* theme = NULL;
    for (size_t i = 0; i < sizeof(themes) / sizeof(themes[0]); i++) {
        if (strcmp(themes[i].name, theme_name) == 0) {
            theme = &themes[i];
            break;
        }
    }

    if (!theme) return false;

    // Update templates
    BoxTemplate* narrative = visual_get_template(manager, "narrative");
    if (narrative) {
        narrative->config.design = theme->narrative_design;
        narrative->config.color_scheme = theme->primary_color;
    }

    BoxTemplate* map = visual_get_template(manager, "map");
    if (map) {
        map->config.design = theme->map_design;
        map->config.color_scheme = theme->secondary_color;
    }

    // ... update other templates

    return true;
}
```

### 7. Animated Boxes

For special effects, boxes can be animated:

```bash
# Pulsing border animation
for i in {1..5}; do
    clear
    echo "CRITICAL ALERT!" | boxes -d thick | \
        sed "s/┃/$(tput setaf 1)┃$(tput sgr0)/g" | \
        sed "s/━/$(tput setaf 1)━$(tput sgr0)/g"
    sleep 0.5
    clear
    echo "CRITICAL ALERT!" | boxes -d stone
    sleep 0.5
done
```

### 8. Box Composition

Combine multiple boxes:

```bash
# Side-by-side boxes
{
    echo "Left Panel" | boxes -d stone -s 30x5
} & {
    echo "Right Panel" | boxes -d double -s 30x5
} | paste - -
```

---

## Integration with Campaign System

### 9. Realm-Specific Themes

**File**: `realms/team_challenge.realm` (additions)

```
[VISUAL]
theme: fantasy
narrative_box: scroll
map_box: stone
stats_box: double
log_box: columns

primary_color: cyan
secondary_color: yellow

title_decoration: ⚔
```

### 10. Command Integration

```c
// New command: theme
bool cmd_theme(MultiplayerWorld* world, int player_number,
              const char* args, char* result, int result_max) {
    VisualManager* visual_mgr = /* get from world */;

    if (theme_apply(visual_mgr, args)) {
        snprintf(result, result_max, "Theme changed to: %s", args);
        return true;
    } else {
        snprintf(result, result_max, "Unknown theme: %s", args);
        return false;
    }
}
```

---

## Performance Considerations

### 11. Caching

```c
// Cache rendered boxes to avoid re-rendering
typedef struct {
    char content_hash[32];
    char template_name[64];
    RenderedBox box;
    time_t cached_at;
} BoxCache;

// Check cache before rendering
bool visual_render_cached(VisualManager* manager, const char* content,
                          const char* template_name, RenderedBox* out_box) {
    // Calculate content hash
    uint32_t hash = calculate_hash(content);

    // Check cache
    for (int i = 0; i < manager->cache_count; i++) {
        if (/* hash matches && template matches */) {
            memcpy(out_box, &manager->cache[i].box, sizeof(RenderedBox));
            return true;
        }
    }

    // Cache miss - render and cache
    if (visual_render_with_template(manager, content, template_name, out_box)) {
        // Add to cache
        return true;
    }

    return false;
}
```

---

## Testing

### 12. Visual Test Suite

**File**: `tests/test_visual.sh`

```bash
#!/bin/bash

echo "=== Visual System Tests ==="

# Test 1: Boxes installation
echo -n "Test 1: Checking boxes installation... "
if command -v boxes &> /dev/null; then
    echo "PASS"
else
    echo "WARN (boxes not found)"
fi

# Test 2: Basic box rendering
echo -n "Test 2: Basic box rendering... "
echo "Test content" | boxes -d stone > /tmp/test_box.txt
if [ -s /tmp/test_box.txt ]; then
    echo "PASS"
else
    echo "FAIL"
fi

# Test 3: Custom design
echo -n "Test 3: Custom design... "
echo "Custom" | boxes -f config/custom.boxes -d adventure > /tmp/test_custom.txt
if [ -s /tmp/test_custom.txt ]; then
    echo "PASS"
else
    echo "WARN (custom design failed)"
fi

# Test 4: Color output
echo -n "Test 4: ANSI colors... "
{
    echo -e "\033[31mRed\033[0m" | boxes -d stone
} > /tmp/test_color.txt
if grep -q "Red" /tmp/test_color.txt; then
    echo "PASS"
else
    echo "FAIL"
fi

# Cleanup
rm -f /tmp/test_*.txt

echo "Tests complete!"
```

---

## Deployment Checklist

- [ ] Boxes installed on system
- [ ] Custom box designs created
- [ ] Panel scripts updated with boxes integration
- [ ] Themes defined and tested
- [ ] Performance acceptable (<100ms per box)
- [ ] Fallback to simple boxes working
- [ ] Color schemes tested on different terminals
- [ ] Documentation updated

---

## Summary

This deep dive provides complete boxes integration:

1. **Visual manager** for box rendering
2. **Template system** for different contexts
3. **Theme support** for mood/realm
4. **Custom box designs** in `.boxes` format
5. **Panel integration** with fallback
6. **Color schemes** for visual hierarchy
7. **Caching** for performance
8. **Dynamic themes** per realm/campaign

**Key benefits**:
- Professional ASCII art appearance
- Consistent visual language
- Theme-able for different moods
- Minimal performance impact
- Graceful fallback without boxes

**Estimated implementation time**: 8-12 hours

**Files to create**:
- `include/visual.h`
- `src/visual.c`
- `src/theme_manager.c`
- `config/custom.boxes`
- `tests/test_visual.sh`
- Updated panel scripts

**Total new code**: ~1,500 lines

**Dependencies**: boxes (optional, with fallback)
