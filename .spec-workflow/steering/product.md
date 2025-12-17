# YAMY Product Vision

## Product Overview

**YAMY (Yet Another Mado tsukai no Yuutsu)** is a powerful keyboard remapping utility that transforms any keyboard into a fully customizable input device. Originally Windows-only, YAMY is expanding to Linux to serve the cross-platform developer community.

### Vision Statement

Enable developers and power users to achieve peak productivity by providing a unified, cross-platform keyboard customization experience that eliminates the friction between different operating systems.

### Mission

Bring enterprise-grade keyboard remapping to Linux while maintaining 100% feature parity with the mature Windows implementation, ensuring users never compromise their workflow when switching platforms.

---

## Target Users

### Primary: Cross-Platform Developers
- **Demographics**: Software engineers, DevOps, system administrators
- **Pain Points**:
  - Muscle memory breaks when switching between Windows/Linux
  - Linux lacks mature keyboard remapping tools
  - Inconsistent key bindings across platforms
- **Goals**: Identical keyboard behavior everywhere, programmable layouts

### Secondary: Emacs/Vim Power Users
- **Demographics**: Long-time Unix users, text editor enthusiasts
- **Pain Points**:
  - Limited system-wide key remapping on Linux
  - xcape/xmodmap too primitive
  - Want Emacs bindings everywhere (browser, IDE, terminal)
- **Goals**: System-wide modal editing, prefix keys, chording

### Tertiary: International Keyboard Users
- **Demographics**: Japanese, Korean, European users
- **Pain Points**:
  - IME key conflicts
  - Non-US keyboard layouts poorly supported
  - Want to remap language-switching keys
- **Goals**: Seamless multilingual input, custom IME triggers

---

## Product Goals

### Immediate (Q1 2025) - JSON Refactoring **[IN PROGRESS]**
1. **Simplify Configuration** - Replace complex .mayu parser with JSON-based config
2. **Focus on Core Remapping** - M00-MFF virtual modifiers + basic key remapping
3. **Remove Complexity** - Drop window matching, thread tracking (~5,000 LOC removed)
4. **Improve Performance** - 50% faster event processing, <10ms config load time
5. **Better Maintainability** - Codebase reduced by ~30%, cleaner architecture

**Rationale**: The current .mayu parser and window/focus system are overly complex for most users' needs. By focusing on core key remapping functionality with a simple JSON config, we can deliver a more maintainable product that's easier to extend and debug.

### Short-Term (Q2 2025)
1. **Documentation** - JSON schema docs, migration guide from .mayu
2. **Example Configs** - Vim-mode, Emacs-mode, basic remapping templates
3. **Testing** - Property-based tests for M00-MFF, comprehensive E2E coverage
4. **Build System** - <5 second incremental builds with Mold/LLD + ccache

### Medium-Term (Q3-Q4 2025)
1. **Optional Features** - Add back per-window keymaps if there's user demand
2. **Wayland Support** - Native Wayland input capture (no X11 dependency)
3. **Visual Configurator** - GUI-based JSON keymap editor
4. **Advanced Modifiers** - Lock keys (L00-LFF) support if needed

### Long-Term (2026+)
1. **macOS Port** - Complete cross-platform trinity
2. **Plugin System** - Lua scripting for custom key actions
3. **Configuration Sharing** - Cloud sync across devices
4. **Marketplace** - Community keymap sharing platform

---

## Success Metrics

### Adoption Metrics
- **Target**: 10,000 Linux installs by end of 2025
- **Measurement**: GitHub releases download count, package manager stats
- **Indicator**: 50/50 Windows/Linux user split

### Quality Metrics
- **Target**: <5 P0 bugs at any time
- **Measurement**: GitHub issues labeled "P0" or "critical"
- **Indicator**: 90% of bugs closed within 7 days

### Engagement Metrics
- **Target**: 100 active community contributors
- **Measurement**: GitHub discussions, PR submissions, wiki edits
- **Indicator**: 20 community-created keymaps in 6 months

### Performance Metrics
- **Target**: <1ms input latency (99th percentile)
- **Measurement**: Built-in latency profiler with zero-overhead logging
- **Indicator**: No user complaints about lag/ghosting

### Developer Velocity Metrics
- **Target**: <5 second incremental builds
- **Measurement**: CI build time tracking, local developer measurements
- **Indicator**: Contributors report fast iteration cycles

---

## User Outcomes

### For Developers
- **Before**: "I avoid Linux because my keyboard shortcuts don't work"
- **After**: "I use the same .mayu file on all my machines - seamless"

### For Power Users
- **Before**: "xmodmap breaks every X update, xcape is buggy"
- **After**: "YAMY just works. Set it and forget it"

### For International Users
- **Before**: "IME keys conflict with app shortcuts, can't fix it"
- **After**: "I remapped CapsLock to toggle IME - perfect"

---

## Competitive Landscape

### Linux Alternatives
| Tool | Strengths | Weaknesses | YAMY Advantage |
|------|-----------|------------|----------------|
| **xmodmap** | Built-in, simple | Breaks on X updates, no layers | Stable, advanced features |
| **xcape** | Dual-role keys | Single-purpose, unmaintained | Full programming, active dev |
| **kmonad** | Powerful, Haskell-based | Complex config, niche | Familiar .mayu syntax |
| **keyd** | Modern, systemd | Limited scripting | Rich function library |

### Windows Alternatives
| Tool | Strengths | Weaknesses | YAMY Advantage |
|------|-----------|------------|----------------|
| **AutoHotkey** | Popular, scripting | Heavy, security concerns | Lightweight, input-focused |
| **PowerToys** | Microsoft official | Basic features | Advanced remapping |
| **SharpKeys** | Simple registry editor | Static, no layers | Dynamic, programmable |

**YAMY Differentiator**: **Cross-platform consistency** - same config file, same behavior, Windows and Linux.

---

## Product Principles

### 1. **Zero Compromise Cross-Platform**
- Feature parity or don't ship
- Identical .mayu file syntax
- Same keyboard layout on all systems

### 2. **Performance First**
- Sub-millisecond latency mandatory
- No frame drops, no input lag
- Background service, not an app

### 3. **Power User Focused**
- Text-based configuration (programmers love this)
- Full Turing-complete scripting
- No dumbing down for "ease of use"

### 4. **Respectful of Legacy**
- Backward compatible with all .mayu files
- Migration path from Windows registry
- Don't break user workflows

### 5. **Open and Extensible**
- MIT license, forever free
- Plugin API for custom functions
- Community-driven development

### 6. **Developer Velocity as a Feature**
- Sub-5 second incremental builds mandatory
- AI-agent compatible project structure
- Automated code quality enforcement
- Zero-friction contribution experience

---

## Roadmap Risks

### Technical Risks
1. **Wayland Input Capture** - API still immature, may need workarounds
2. **Multi-Monitor Edge Cases** - X11/Wayland behavior differs
3. **Performance on ARM** - Raspberry Pi users may see lag
4. **Build System Complexity** - Modern toolchain may increase learning curve

**Mitigations**: Early prototyping, community testing, performance profiling, comprehensive documentation

### Market Risks
1. **keyd/kmonad adoption** - Competitors gaining traction
2. **Wayland fragmentation** - Different compositors behave differently
3. **User expectations** - Windows users expect instant perfection

**Mitigations**: Highlight cross-platform USP, clear beta labels, gradual rollout

### Resource Risks
1. **Volunteer dependency** - Core team is 1 person + contributors
2. **Platform testing** - Need maintainers on GNOME, KDE, XFCE
3. **Documentation debt** - Windows docs exist, Linux docs sparse

**Mitigations**: Recruit co-maintainers, automated testing, docs-first development

---

## Non-Goals (What YAMY Won't Do)

### Out of Scope
- ❌ **Mouse remapping** - Use xbindkeys or similar
- ❌ **Macro recording** - Too complex, use scripting instead
- ❌ **Gaming profiles** - Not target audience
- ❌ **Cloud-based AI** - Privacy first, local only
- ❌ **Mobile keyboards** - Physical keyboards only

### Explicitly Rejected
- ❌ **Electron GUI** - Qt is lighter and native
- ❌ **.mayu text format** - Replaced by JSON for simplicity and maintainability
- ❌ **Per-window keymaps (Phase 1)** - Focusing on global keymap first, can add later if needed
- ❌ **Backward compatibility with .mayu** - Clean break for better future
- ❌ **Paid tiers** - Free forever, donations only
- ❌ **Telemetry** - Zero data collection (structured logging is for development/debugging only)

---

## Go-to-Market Strategy

### Distribution Channels
1. **Package Managers** (Primary)
   - AUR (Arch User Repository) - community-driven
   - PPA (Ubuntu/Debian) - official maintained
   - Copr (Fedora) - community-driven
   - Flatpak/Snap - universal packages

2. **GitHub Releases** (Secondary)
   - Pre-built .deb, .rpm, .tar.gz
   - Install scripts for manual builds

3. **Word of Mouth** (Growth Engine)
   - Reddit (r/mechanicalkeyboards, r/linux)
   - Hacker News launch post
   - YouTube demos (targeted at devs)

### Marketing Messages
- **For Windows Users**: "Your .mayu file works on Linux, zero changes"
- **For Linux Users**: "AutoHotkey for Linux, but better"
- **For Emacs Users**: "System-wide Emacs bindings, finally"
- **For Developers**: "Build it in 5 seconds, run it in 1 millisecond"

---

## Version 1.0 Definition

**Minimum Shippable Product (Linux):**
- ✅ Core engine compiles and runs
- ✅ Input capture from all keyboards
- ✅ .mayu file parsing (100% Windows-compatible)
- ✅ Qt GUI with tray icon
- ✅ All Windows functions implemented
- ✅ Documentation and examples
- ✅ Zero P0 bugs

**Release Criteria:**
1. Passes 100% of Windows integration tests
2. Community beta testing (20+ testers, 2 weeks)
3. Benchmarks: <1ms latency, <5MB RAM, <1% CPU
4. Build benchmarks: <5s incremental, <2min clean
5. All GitHub issues labeled "v1.0-blocker" closed

**Launch Plan:**
- Date: Q2 2025 (April-June)
- Channels: HN post, r/linux announcement, blog post
- Support: Discord server, GitHub discussions
- Follow-up: AMA session, video tutorial series

---

## Long-Term Product Vision (2026+)

### The Grand Vision: **Universal Input Transformation Platform**

YAMY evolves from "keyboard remapper" to "input transformation layer" - any input device (keyboard, mouse, touchpad, game controller) can trigger any action (key press, mouse move, shell command, API call, cross-device sync).

**Example Future Capabilities:**
- Foot pedal triggers clipboard paste
- Mouse gesture runs git commit
- Keyboard chord sends Slack message
- Touchpad swipe switches workspace
- Voice command types pre-defined text

**Core Principle**: "If it sends input events, YAMY can transform them"

This positions YAMY as the **operating system's missing input abstraction layer** - what systemd did for init, YAMY does for input.

---

**Document Version**: 2.0
**Last Updated**: 2025-12-15
**Reviewed By**: (Pending approval)
