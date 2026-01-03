#!/bin/bash
# code-review.sh - Automated code quality review for adventure-engine-v2
# Uses Claude Code with specialized agents for comprehensive code analysis
#
# Usage:
#   ./scripts/code-review.sh              # Run quick review (code + security)
#   ./scripts/code-review.sh --full       # Run all agents
#   ./scripts/code-review.sh --security   # Security audit only
#   ./scripts/code-review.sh --memory     # C memory safety focus

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PROJECT_NAME="adventure-engine-v2"
REVIEWS_DIR="$PROJECT_ROOT/state/reviews"
DATE=$(date +%Y-%m-%d)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Available review types
declare -A REVIEW_TYPES=(
    ["code-quality"]="Code Quality Review"
    ["security"]="Security Audit"
    ["memory-safety"]="C Memory Safety Analysis"
    ["test-coverage"]="Test Coverage Assessment"
)

usage() {
    cat <<EOF
Adventure Engine v2 - Code Review Script

Usage: $0 [OPTIONS]

Options:
  --full        Run all review agents (code, security, memory, tests)
  --quick       Run code quality and security only (default)
  --security    Run security audit only
  --memory      Run C memory safety analysis only
  --tests       Run test coverage assessment only
  --output DIR  Custom output directory (default: state/reviews/YYYY-MM-DD)
  --help        Show this help message

Examples:
  $0                    # Quick review
  $0 --full             # Complete review
  $0 --security         # Security-focused review
  $0 --memory           # Memory safety audit (good for C code)

Review Output:
  Reports are saved to: $REVIEWS_DIR/YYYY-MM-DD/
EOF
    exit 0
}

check_prerequisites() {
    if ! command -v claude &> /dev/null; then
        echo -e "${RED}Error: Claude Code CLI not found${NC}"
        echo "Install from: https://claude.ai/download"
        exit 1
    fi
}

run_review() {
    local review_type="$1"
    local output_file="$2"
    local description="${REVIEW_TYPES[$review_type]}"

    echo -e "${CYAN}Running: $description${NC}"
    echo "Output: $output_file"
    echo ""

    local prompt
    case $review_type in
        code-quality)
            prompt="Review all C code in this project for quality issues. Focus on:
- Correctness and edge cases
- Error handling completeness
- Code clarity and maintainability
- API design and consistency
- Dead code and unused variables

Organize findings by severity: Critical, Warning, Suggestion.
Include file paths and line numbers for all issues.
For this C project, pay special attention to:
- Proper use of standard library functions
- Return value checking
- Resource cleanup"
            ;;
        security)
            prompt="Perform a comprehensive security audit of this C project. Check for:
- Buffer overflows (strcpy, strcat, sprintf, gets)
- Format string vulnerabilities
- Integer overflows in size calculations
- Path traversal vulnerabilities
- Command injection risks
- Memory corruption (use-after-free, double-free)
- Null pointer dereferences
- Race conditions in file operations

Provide severity ratings (Critical/High/Medium/Low) and remediation steps.
Reference OWASP and CWE identifiers where applicable."
            ;;
        memory-safety)
            prompt="Analyze this C project specifically for memory safety issues. Examine:
- All malloc/calloc/realloc/free calls
- Array bounds checking
- strncpy null termination
- fread/fwrite return value validation
- File handle leaks
- Uninitialized variables
- Stack buffer overflows

For each issue, provide:
1. Exact file and line number
2. Vulnerable code pattern
3. Secure replacement pattern
4. Test case to verify fix

This is critical for a text adventure game that parses user input."
            ;;
        test-coverage)
            prompt="Assess the test coverage of this C project. Analyze:
- Current test files in tests/ directory
- Functions without test coverage
- Edge cases not tested
- Security-critical code paths needing tests
- Integration test gaps

Recommend specific tests to add, including:
- Test function names
- Input/output expectations
- Mock requirements"
            ;;
        *)
            echo -e "${RED}Unknown review type: $review_type${NC}"
            return 1
            ;;
    esac

    # Run the review
    cd "$PROJECT_ROOT"

    # Execute Claude with the review prompt
    if claude -p "You are reviewing the adventure-engine-v2 C project.

$prompt

Project structure:
- src/: Core C source files (main.c, parser.c, world.c, session.c, player.c, ipc.c)
- include/: Header files
- tests/: Test suites
- worlds/: Example .world files

Format your response as a markdown report with clear sections and code examples." > "$output_file" 2>&1; then
        echo -e "${GREEN}✓ Complete${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠ Completed with warnings${NC}"
        return 0
    fi
}

generate_summary() {
    local reviews_run="$1"
    local summary_file="$OUTPUT_DIR/${PROJECT_NAME}-SUMMARY.md"

    echo -e "${BLUE}Generating summary...${NC}"

    cat > "$summary_file" <<EOF
# Code Review Summary: $PROJECT_NAME

**Date:** $(date '+%Y-%m-%d %H:%M:%S')
**Review Directory:** \`$OUTPUT_DIR\`

## Reviews Completed

EOF

    for review in $reviews_run; do
        local description="${REVIEW_TYPES[$review]}"
        local report_file="${PROJECT_NAME}-${review}.md"
        if [ -f "$OUTPUT_DIR/$report_file" ]; then
            echo "- **$description** → [\`$report_file\`](./$report_file)" >> "$summary_file"
        fi
    done

    cat >> "$summary_file" <<EOF

## Quick Actions

\`\`\`bash
# Build with sanitizers
make DEBUG=1 all

# Run all tests
make DEBUG=1 run-tests

# Check specific report
cat $OUTPUT_DIR/${PROJECT_NAME}-security.md

# Create issue from findings
gh issue create --title "Security: <finding>" --body "..."
\`\`\`

## Files Generated

EOF

    ls -1 "$OUTPUT_DIR"/${PROJECT_NAME}-*.md 2>/dev/null | while read -r file; do
        basename "$file" | sed 's/^/- /' >> "$summary_file"
    done

    echo -e "${GREEN}✓ Summary: $summary_file${NC}"
}

# Parse arguments
OUTPUT_DIR=""
REVIEWS_TO_RUN=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --full)
            REVIEWS_TO_RUN="code-quality security memory-safety test-coverage"
            shift
            ;;
        --quick)
            REVIEWS_TO_RUN="code-quality security"
            shift
            ;;
        --security)
            REVIEWS_TO_RUN="security"
            shift
            ;;
        --memory)
            REVIEWS_TO_RUN="memory-safety"
            shift
            ;;
        --tests)
            REVIEWS_TO_RUN="test-coverage"
            shift
            ;;
        --output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            ;;
    esac
done

# Default to quick review
[ -z "$REVIEWS_TO_RUN" ] && REVIEWS_TO_RUN="code-quality security"

# Set output directory
[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR="$REVIEWS_DIR/$DATE"
mkdir -p "$OUTPUT_DIR"

# Check prerequisites
check_prerequisites

# Display header
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Code Review: $PROJECT_NAME${NC}"
echo -e "${CYAN}Path: $PROJECT_ROOT${NC}"
echo -e "${CYAN}Output: $OUTPUT_DIR${NC}"
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Run reviews
for review in $REVIEWS_TO_RUN; do
    if [ -z "${REVIEW_TYPES[$review]}" ]; then
        echo -e "${YELLOW}Warning: Unknown review type '$review', skipping...${NC}"
        continue
    fi

    output_file="$OUTPUT_DIR/${PROJECT_NAME}-${review}.md"

    echo -e "${CYAN}─────────────────────────────────────────────────────────${NC}"
    run_review "$review" "$output_file"
    echo ""
done

# Generate summary
echo -e "${CYAN}─────────────────────────────────────────────────────────${NC}"
generate_summary "$REVIEWS_TO_RUN"

# Completion message
echo ""
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Code Review Complete!${NC}"
echo ""
echo -e "Summary: ${CYAN}$OUTPUT_DIR/${PROJECT_NAME}-SUMMARY.md${NC}"
echo ""
echo -e "${CYAN}Next Steps:${NC}"
echo "  1. Review findings: less $OUTPUT_DIR/${PROJECT_NAME}-*.md"
echo "  2. Run tests: make DEBUG=1 run-tests"
echo "  3. Create issues: gh issue create"
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════${NC}"
