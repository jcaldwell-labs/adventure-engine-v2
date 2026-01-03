# Organization Guidelines

This document outlines development standards and guidelines for the Adventure Engine v2 project and related repositories in the jcaldwell-labs organization.

## Code Quality Standards

### General Principles
- ✅ Write clean, readable, maintainable code
- ✅ Follow language-specific best practices
- ✅ Prioritize simplicity over cleverness
- ✅ Document complex logic and design decisions
- ✅ Keep functions focused and testable

### C/C++ Projects
- **Standard**: C11 or C++17 minimum
- **Warnings**: Zero tolerance - compile with `-Wall -Wextra -Werror`
- **Style**: Consistent indentation (4 spaces), descriptive names
- **Memory**: Always check allocations, no leaks (verify with valgrind)
- **Safety**: Bounds checking, input validation, error handling

### Documentation Requirements
- **README.md**: Badges, quick start, features, examples
- **CLAUDE.md**: AI development guide for repositories
- **CONTRIBUTING.md**: Contribution guidelines
- **Architecture docs**: For projects > 1,000 LOC
- **Inline comments**: Complex algorithms, edge cases, TODOs

## Testing Standards

### Minimum Requirements
- ✅ Unit tests for core functionality
- ✅ Integration tests for system interactions
- ✅ Target: >80% code coverage
- ✅ All tests pass before merge
- ✅ CI/CD pipeline automation

### Test Structure
```
tests/
├── unit/           # Unit tests
├── integration/    # Integration tests
└── fixtures/       # Test data
```

### Testing Tools
- **C/C++**: Custom test frameworks or Google Test
- **CI/CD**: GitHub Actions (mandatory)
- **Coverage**: gcov/lcov for C, coverage.py for Python

## Git Workflow

### Branch Strategy
- **main**: Production-ready code only
- **develop**: Integration branch for features
- **feature/**: Feature branches (feature/add-npc-system)
- **fix/**: Bug fix branches (fix/parser-crash)
- **claude/**: AI assistant working branches (claude/*)

### Commit Messages
```
<type>: <subject>

<body>

<footer>
```

**Types**: feat, fix, docs, test, refactor, chore, perf

**Example**:
```
feat: add NPC dialogue system

Implement basic NPC interaction with scripted responses.
Supports multiple dialogue options and state tracking.

- Added NPC data structure
- Implemented dialogue parser
- Added 3 example NPCs
- Added test coverage

Closes #42
```

### Pull Request Guidelines
1. **Title**: Clear, descriptive
2. **Description**: What, why, how
3. **Testing**: How tested, results
4. **Checklist**: Code review checklist completed
5. **CI/CD**: All checks must pass
6. **Review**: At least one approval required

## Documentation Structure

### Repository Organization
```
repository/
├── .github/
│   ├── workflows/          # CI/CD workflows
│   ├── ISSUE_TEMPLATE/     # Issue templates
│   ├── PULL_REQUEST_TEMPLATE.md
│   ├── ORG-GUIDELINES.md   # This file
│   └── planning/           # Roadmaps, backlogs
├── docs/                   # Documentation
│   ├── README.md           # Documentation index
│   ├── guides/             # User/developer guides
│   ├── tutorials/          # Step-by-step tutorials
│   └── examples/           # Code examples
├── src/                    # Source code
├── tests/                  # Test suite
├── README.md               # Project overview
├── CLAUDE.md               # AI development guide
├── CONTRIBUTING.md         # Contribution guide
├── LICENSE                 # License file
└── llms.txt                # AI discoverability
```

### Documentation Priorities
1. **User-facing**: README, quick starts, tutorials
2. **Developer-facing**: CLAUDE.md, CONTRIBUTING.md, architecture docs
3. **API/Reference**: Function documentation, format specs
4. **AI-facing**: llms.txt for AI tool discoverability

## CI/CD Requirements

### GitHub Actions Workflow
Every repository should include:

```yaml
name: CI Build and Test

on:
  push:
    branches: [ main, develop, claude/* ]
  pull_request:
    branches: [ main, develop ]

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
    steps:
      - uses: actions/checkout@v4
      - name: Build
        run: make all
      - name: Test
        run: make test
```

### Required Checks
- ✅ Build succeeds on all platforms
- ✅ All tests pass
- ✅ Zero compiler warnings
- ✅ No memory leaks (C/C++)
- ✅ Code quality checks (linters)

## Security Guidelines

### Code Security
- ✅ No hardcoded secrets or credentials
- ✅ Input validation on all user input
- ✅ Bounds checking on all buffers
- ✅ Use secure functions (strncpy, not strcpy)
- ✅ Sanitize file paths and system calls

### Dependency Management
- ✅ Keep dependencies up to date
- ✅ Review security advisories
- ✅ Use Dependabot for automated updates
- ✅ Document all dependencies

### Vulnerability Response
1. Report via GitHub Security Advisories
2. Fix within 7 days for critical issues
3. Update dependencies promptly
4. Document in CHANGELOG

## Release Process

### Versioning (Semantic Versioning)
- **MAJOR**: Breaking changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

**Example**: 2.3.1 → 2.4.0 (new feature)

### Release Checklist
- [ ] All tests pass
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Version bumped
- [ ] Tagged in git
- [ ] Release notes written
- [ ] Binaries built (if applicable)

### Release Notes Template
```markdown
## v2.4.0 - Feature Name

### Features
- Add NPC dialogue system
- Implement quest tracking

### Improvements
- Better error messages
- Performance optimizations

### Bug Fixes
- Fix parser crash on empty input
- Fix save corruption issue

### Breaking Changes
None

### Migration Guide
No changes required
```

## Code Review Guidelines

### Reviewer Responsibilities
- ✅ Check code quality and style
- ✅ Verify tests exist and pass
- ✅ Review documentation changes
- ✅ Test locally if possible
- ✅ Provide constructive feedback
- ✅ Approve only when ready to merge

### Author Responsibilities
- ✅ Self-review before requesting review
- ✅ Respond to feedback promptly
- ✅ Make requested changes
- ✅ Keep PR focused (single purpose)
- ✅ Rebase on develop if needed

### Review Checklist
- [ ] Code follows style guidelines
- [ ] Tests added for new features
- [ ] Documentation updated
- [ ] No compiler warnings
- [ ] CI/CD checks pass
- [ ] No security issues
- [ ] Performance acceptable

## Project Management

### Issue Labels
- **Type**: bug, feature, enhancement, documentation
- **Priority**: critical, high, medium, low
- **Status**: in-progress, blocked, ready-for-review
- **Good First Issue**: For newcomers
- **Help Wanted**: Community help needed

### Milestones
- Use for version releases (v2.4.0, v3.0.0)
- Group related issues/PRs
- Track progress toward release

### Project Boards
- **Backlog**: Future work
- **Ready**: Ready to start
- **In Progress**: Active work
- **Review**: Awaiting review
- **Done**: Completed

## Community Guidelines

### Code of Conduct
- ✅ Be respectful and professional
- ✅ Welcome newcomers
- ✅ Provide constructive criticism
- ✅ Focus on what's best for the project
- ✅ Show empathy and kindness

### Communication Channels
- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: Q&A, ideas, general discussion
- **Pull Requests**: Code contributions
- **Email**: Security issues only

### Recognition
- Contributors listed in README
- Major contributors in ACKNOWLEDGMENTS
- Monthly "Contributor Spotlight" (if active community)

## AI Assistant Guidelines

### Using AI Tools (Claude, Copilot, etc.)
- ✅ Review all AI-generated code
- ✅ Test thoroughly
- ✅ Ensure code quality standards
- ✅ Add human review before merge
- ✅ Document AI assistance in commit messages (optional)

### CLAUDE.md Requirements
Every repository should include CLAUDE.md with:
- Project overview
- Codebase structure
- Component descriptions
- Common tasks
- Known limitations
- Quick reference

### AI-Friendly Practices
- ✅ Clear, descriptive comments
- ✅ Consistent naming conventions
- ✅ Well-organized file structure
- ✅ Comprehensive documentation
- ✅ llms.txt for AI discoverability

## Performance Guidelines

### Optimization Principles
- ✅ Measure before optimizing
- ✅ Focus on algorithmic improvements first
- ✅ Profile to find bottlenecks
- ✅ Don't sacrifice readability for minor gains
- ✅ Document performance-critical sections

### Benchmarking
- Include benchmarks for performance-critical code
- Run benchmarks in CI/CD
- Track performance over time
- Document performance requirements

## Accessibility

### Terminal Applications
- ✅ Support standard terminal sizes (80x24 minimum)
- ✅ Graceful degradation on small terminals
- ✅ Color schemes that work on light/dark backgrounds
- ✅ Support for screen readers (where applicable)

### Documentation
- ✅ Clear, concise writing
- ✅ Examples for complex concepts
- ✅ Alternative text for images
- ✅ Accessible code examples

## Maintenance

### Regular Tasks
- **Monthly**: Review open issues, update dependencies
- **Quarterly**: Review documentation, security audit
- **Annually**: Roadmap review, major version planning

### Deprecated Code
- Mark as deprecated with comments
- Add deprecation timeline
- Provide migration path
- Remove after reasonable transition period

## References

- [Semantic Versioning](https://semver.org/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [GitHub Flow](https://guides.github.com/introduction/flow/)

---

**Last Updated**: December 25, 2025
**Applies To**: All jcaldwell-labs repositories
**Maintainer**: jcaldwell1066
