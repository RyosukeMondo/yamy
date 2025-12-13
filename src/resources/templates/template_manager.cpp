#include "template_manager.h"

#include <fstream>
#include <map>
#include <vector>

namespace yamy
{
namespace resources
{
namespace templates
{

namespace
{

const std::string DEFAULT_TEMPLATE = R"mayu_template(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - default.mayu
# Default configuration template with common keyboard remappings
#
# This template provides essential keyboard customizations that most users
# find helpful. It serves as a good starting point for further customization.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Keyboard Type Detection
# Automatically detects whether you have a 104-key (US) or 109-key (JP) layout
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if ( !KBD109 ) and ( !KBD104 )
  # Default to 109-key Japanese keyboard layout if not specified
  # Change this if you have a different keyboard layout
endif


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Control
# One of the most popular remappings - makes CapsLock act as Control
# This reduces strain on your pinky and makes Ctrl combinations easier
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

# Remap CapsLock to Left Control
mod control += CapsLock
key *CapsLock = *LControl

# Also handle E0-prefixed CapsLock (some keyboards send this)
mod control += E0CapsLock
key *E0CapsLock = *LControl


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Window Management Shortcuts
# Useful keyboard shortcuts for managing windows
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Move window with Ctrl+Shift+Arrow keys
key C-S-Left   = &WindowMove(-16, 0)   # Move window left
key C-S-Right  = &WindowMove(16, 0)    # Move window right
key C-S-Up     = &WindowMove(0, -16)   # Move window up
key C-S-Down   = &WindowMove(0, 16)    # Move window down

# Fine-grained window movement with Ctrl+Shift+Alt+Arrow
key C-S-A-Left  = &WindowMove(-1, 0)   # Move window left (1 pixel)
key C-S-A-Right = &WindowMove(1, 0)    # Move window right (1 pixel)
key C-S-A-Up    = &WindowMove(0, -1)   # Move window up (1 pixel)
key C-S-A-Down  = &WindowMove(0, 1)    # Move window down (1 pixel)

# Window state shortcuts
key C-S-Z = &WindowMaximize     # Maximize window
key C-S-I = &WindowMinimize     # Minimize window
key C-S-X = &WindowVMaximize    # Maximize window vertically
key C-S-C = &WindowHMaximize    # Maximize window horizontally


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Mouse Movement via Keyboard
# Use Win+Arrow keys to move the mouse cursor
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

key W-Left  = &MouseMove(-16, 0)   # Move mouse left
key W-Right = &MouseMove(16, 0)    # Move mouse right
key W-Up    = &MouseMove(0, -16)   # Move mouse up
key W-Down  = &MouseMove(0, 16)    # Move mouse down


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Utility Shortcuts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Reload configuration (Ctrl+Shift+S)
key C-S-S = &LoadSetting

# Show window information (Ctrl+Shift+D) - useful for debugging
key C-S-D = &WindowIdentify


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dialog Box Handling
# Make Escape and Ctrl+G close dialog boxes
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window DialogBox /:#32770:/ : Global
  key C-G = Escape
)mayu_template";

const std::string EMACS_TEMPLATE = R"mayu_template(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - emacs.mayu
# Emacs-style keybindings template
#
# This template provides Emacs-like keybindings across all applications.
# If you're familiar with Emacs, this will make other apps feel more natural.
#
# Key conventions:
#   C- = Control
#   M- = Alt (Meta)
#   S- = Shift
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Control
# Essential for comfortable Emacs usage - CapsLock becomes Control
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

mod control += CapsLock
key *CapsLock = *LControl
mod control += E0CapsLock
key *E0CapsLock = *LControl


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Emacs Movement Commands
# These work in text fields across most applications
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap EmacsMove : Global

# Basic cursor movement
key C-F = Right               # Forward one character
key C-B = Left                # Backward one character
key C-N = Down                # Next line
key C-P = Up                  # Previous line
key C-A = Home                # Beginning of line
key C-E = End                 # End of line

# Word movement (Alt+arrow equivalent)
key M-F = C-Right             # Forward one word
key M-B = C-Left              # Backward one word

# Page movement
key C-V = Next                # Scroll down (Page Down)
key M-V = Prior               # Scroll up (Page Up)

# Document navigation
key Home = C-Home             # Beginning of document
key End = C-End               # End of document
key S-M-Comma = C-Home        # M-< (Beginning of buffer)
key S-M-Period = C-End        # M-> (End of buffer)

# Scrolling without moving cursor
key C-L = &WindowRedraw       # Recenter/redraw

# Cancel command
key C-G = Escape              # Cancel current operation

# Search
key C-S = C-F                 # Incremental search forward


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Emacs Editing Commands
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap EmacsEdit : EmacsMove

# Deletion
key C-D = Delete              # Delete character forward
key C-H = BackSpace           # Delete character backward (backspace)
key M-D = S-C-Right C-X       # Kill word forward
key M-BackSpace = S-C-Left C-X  # Kill word backward

# Line operations
key C-K = S-End C-X           # Kill to end of line

# Character transpose
key C-T = S-Right C-X Left C-V Right  # Transpose characters

# Enter/newline
key C-J = Return              # Newline
key C-M = Return              # Carriage return (same as Enter)
key C-O = Return Left         # Open line (insert newline, stay in place)

# Cut, Copy, Paste (Emacs style)
key C-W = C-X                 # Kill region (Cut)
key M-W = C-C                 # Copy region
key C-Y = C-V                 # Yank (Paste)

# Undo
key C-Slash = C-Z             # Undo
key C-Underscore = C-Z        # Undo (alternative)

# Case conversion
# Note: These require clipboard manipulation
# key M-U = (uppercase word)
# key M-L = (lowercase word)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# C-x Prefix Commands
# Emacs uses C-x as a prefix for many commands
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 EmacsC-X : EmacsEdit
  event prefixed = &HelpMessage("C-x", "C-x prefix active")
  event before-key-down = &HelpMessage

  key C-S = C-S               # Save file
  key C-W = LAlt F A          # Save As (Write file)
  key C-F = C-O               # Open file (Find file)
  key K = C-N                 # New file (Kill buffer, then new)
  key C-C = A-F4              # Exit application
  key U = C-Z                 # Undo

keymap EmacsEdit
  key C-X = &Prefix(EmacsC-X)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Mark and Selection
# C-Space sets the mark for text selection
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 EmacsMark : EmacsEdit
  # Movement with selection (extends selection)
  key C-F = S-Right &Prefix(EmacsMark)
  key C-B = S-Left &Prefix(EmacsMark)
  key C-N = S-Down &Prefix(EmacsMark)
  key C-P = S-Up &Prefix(EmacsMark)
  key C-A = S-Home &Prefix(EmacsMark)
  key C-E = S-End &Prefix(EmacsMark)
  key M-F = S-C-Right &Prefix(EmacsMark)
  key M-B = S-C-Left &Prefix(EmacsMark)
  key C-V = S-Next &Prefix(EmacsMark)
  key M-V = S-Prior &Prefix(EmacsMark)
  key Home = S-C-Home &Prefix(EmacsMark)
  key End = S-C-End &Prefix(EmacsMark)

  # Arrow keys with selection
  key Left = S-Left &Prefix(EmacsMark)
  key Right = S-Right &Prefix(EmacsMark)
  key Up = S-Up &Prefix(EmacsMark)
  key Down = S-Down &Prefix(EmacsMark)

  # Cut and copy end mark mode
  key C-W = C-X Left Right    # Kill region
  key M-W = C-C Left Right    # Copy region

  # Cancel mark
  key C-G = Left Right &Undefined

keymap EmacsEdit
  key C-Space = &Prefix(EmacsMark)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Application-Specific Settings
# Apply EmacsEdit keymap to text input controls
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Standard Windows edit controls
window EditControl /:(Edit|TEdit|RichEdit(20[AW])?)$/ : EmacsEdit

# Combo boxes (dropdown with text input)
window ComboBox /:ComboBox(:Edit)?$/ : EmacsEdit

# List views (for navigation)
window SysListView32 /:SysListView32$/ : EmacsMove

# Tree views (for navigation)
window SysTreeView32 /:SysTreeView32$/ : EmacsMove


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dialog Box Handling
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window DialogBox /:#32770:/ : Global
  key C-G = Escape            # Cancel dialog with C-g


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Exclude Real Emacs
# Don't apply these remappings in actual Emacs
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Emacsen : Global
  # Pass through all special keys in real Emacs

window Meadow /:Meadow$/ : Emacsen
window Emacs /:Emacs$/ : Emacsen


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Utility Shortcuts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global
  # Reload configuration
  key C-S-S = &LoadSetting
)mayu_template";

const std::string VIM_TEMPLATE = R"mayu_template(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - vim.mayu
# Vim-style keybindings template
#
# This template provides Vim-like keybindings for navigation and editing
# across applications. Useful for Vim users who want consistent keybindings.
#
# Note: This provides basic Vim motions, not full Vim emulation.
# For complete Vim behavior, consider a dedicated Vim emulator.
#
# Escape is used to enter "normal mode" where h/j/k/l become movement keys.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Escape
# Many Vim users prefer CapsLock as Escape for faster mode switching
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

# Remap CapsLock to Escape (common Vim user preference)
key *CapsLock = *Escape
key *E0CapsLock = *Escape


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Normal Mode
# Press Escape to enter this mode where h/j/k/l become movement keys
# Press i, a, or other insert commands to return to insert mode
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimNormal : Global
  event prefixed = &HelpMessage("VIM", "-- NORMAL --")
  event before-key-down = &HelpMessage

  # Basic movement (h/j/k/l)
  key H = Left                # Move left
  key J = Down                # Move down
  key K = Up                  # Move up
  key L = Right               # Move right

  # Word movement
  key W = C-Right             # Forward to start of next word
  key B = C-Left              # Backward to start of word
  key E = C-Right Left        # Forward to end of word

  # Line movement
  key _0 = Home               # Beginning of line
  key S-_4 = End              # End of line ($)
  key S-_6 = Home             # First non-blank character (^)

  # Document movement
  key G G = C-Home            # Go to start of document
  key S-G = C-End             # Go to end of document

  # Page movement
  key C-F = Next              # Page forward (Page Down)
  key C-B = Prior             # Page backward (Page Up)
  key C-D = Next              # Half page down (simplified)
  key C-U = Prior             # Half page up (simplified)

  # Insert mode transitions
  key I = &Undefined          # Insert before cursor (exit normal mode)
  key A = Right &Undefined    # Append after cursor
  key S-I = Home &Undefined   # Insert at beginning of line
  key S-A = End &Undefined    # Append at end of line
  key O = End Return &Undefined  # Open line below
  key S-O = Home Return Up &Undefined  # Open line above

  # Editing in normal mode
  key X = Delete              # Delete character under cursor
  key S-X = BackSpace         # Delete character before cursor
  key R = &Prefix(VimReplace) # Replace single character

  # Delete operations
  key D D = Home S-End C-X    # Delete entire line
  key D W = S-C-Right C-X     # Delete word
  key D S-_4 = S-End C-X      # Delete to end of line (d$)
  key S-D = S-End C-X         # Delete to end of line (D)

  # Yank (copy) operations
  key Y Y = Home S-End C-C Right  # Yank entire line
  key Y W = S-C-Right C-C Left    # Yank word
  key Y S-_4 = S-End C-C      # Yank to end of line

  # Put (paste)
  key P = C-V                 # Put after cursor
  key S-P = C-V Left          # Put before cursor

  # Change operations (delete and enter insert mode)
  key C C = Home S-End C-X &Undefined  # Change entire line
  key C W = S-C-Right C-X &Undefined   # Change word
  key S-C = S-End C-X &Undefined       # Change to end of line

  # Undo/Redo
  key U = C-Z                 # Undo
  key C-R = C-Y               # Redo

  # Search
  key Slash = C-F             # Search forward
  key N = F3                  # Next search result
  key S-N = S-F3              # Previous search result

  # Join lines
  key S-J = End Delete Space  # Join lines

  # Cancel/Escape stays in normal mode
  key Escape = &Prefix(VimNormal)

keymap Global
  # Enter normal mode with Escape
  key Escape = &Prefix(VimNormal)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Replace Mode
# Replace a single character, then return to normal mode
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimReplace : VimNormal
  event prefixed = &HelpMessage("VIM", "-- REPLACE --")
  event before-key-down = &HelpMessage

  # Any key replaces current character
  # After replacement, go back to normal mode
  # (This is simplified - true Vim replace is more complex)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Visual Mode (Selection)
# Press v in normal mode to start visual selection
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimVisual : Global
  event prefixed = &HelpMessage("VIM", "-- VISUAL --")
  event before-key-down = &HelpMessage

  # Movement with selection
  key H = S-Left &Prefix(VimVisual)
  key J = S-Down &Prefix(VimVisual)
  key K = S-Up &Prefix(VimVisual)
  key L = S-Right &Prefix(VimVisual)
  key W = S-C-Right &Prefix(VimVisual)
  key B = S-C-Left &Prefix(VimVisual)
  key _0 = S-Home &Prefix(VimVisual)
  key S-_4 = S-End &Prefix(VimVisual)

  # Visual mode operations
  key Y = C-C &Prefix(VimNormal)  # Yank selection
  key D = C-X &Prefix(VimNormal)  # Delete selection
  key X = C-X &Prefix(VimNormal)  # Delete selection
  key C = C-X &Undefined          # Change selection (delete and insert)

  # Exit visual mode
  key Escape = Right &Prefix(VimNormal)
  key V = Right &Prefix(VimNormal)

keymap2 VimNormal
  # Enter visual mode
  key V = &Prefix(VimVisual)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Command Line Mode (simplified)
# : commands for common operations
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimCommand : Global
  event prefixed = &HelpMessage("VIM", ":")
  event before-key-down = &HelpMessage

  key W Return = C-S &Prefix(VimNormal)   # :w - save
  key Q Return = A-F4                      # :q - quit
  key Q S-_1 Return = A-F4                # :q! - force quit
  key W Q Return = C-S A-F4               # :wq - save and quit
  key X Return = C-S A-F4                 # :x - save and quit

  # Cancel command
  key Escape = &Prefix(VimNormal)

keymap2 VimNormal
  # Enter command mode
  key S-Semicolon = &Prefix(VimCommand)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Application-Specific Settings
# Only enable Vim keys in text editing contexts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window EditControl /:(Edit|TEdit|RichEdit(20[AW])?)$/ : Global
  key Escape = &Prefix(VimNormal)

window ComboBox /:ComboBox(:Edit)?$/ : Global
  key Escape = &Prefix(VimNormal)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Exclude Real Vim/Terminal Applications
# Don't apply these remappings in actual Vim or terminal emulators
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap VimExclude : Global
  # Pass through all keys - don't remap in real Vim

window GVim /gvim.*:Vim$/ : VimExclude
window Vim /vim:/ : VimExclude
window Terminal /:(ConsoleWindowClass|mintty|Terminal)$/ : VimExclude


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Global Shortcuts (always available)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global
  # Reload configuration
  key C-S-S = &LoadSetting

  # Window management (Vim-inspired)
  key C-W H = &WindowMove(-16, 0)   # Move window left
  key C-W J = &WindowMove(0, 16)    # Move window down
  key C-W K = &WindowMove(0, -16)   # Move window up
  key C-W L = &MoveMove(16, 0)    # Move window right
)mayu_template";

const std::map<std::string, const std::string&> TEMPLATES = {
    {"Default", DEFAULT_TEMPLATE},
    {"Emacs", EMACS_TEMPLATE},
    {"Vim", VIM_TEMPLATE}};

} // namespace

std::vector<std::string> TemplateManager::listTemplates() const
{
  std::vector<std::string> names;
  for (const auto& pair : TEMPLATES)
  {
    names.push_back(pair.first);
  }
  return names;
}

bool TemplateManager::createFromTemplate(const std::string& templateName,
                                         const std::string& targetPath) const
{
  auto it = TEMPLATES.find(templateName);
  if (it == TEMPLATES.end())
  {
    return false;
  }

  std::ifstream f(targetPath.c_str());
  if (f.good())
  {
    return false; // File already exists
  }

  std::ofstream outFile(targetPath);
  if (!outFile)
  {
    return false;
  }

  outFile << it->second;
  return true;
}

} // namespace templates
} // namespace resources
} // namespace yamy
