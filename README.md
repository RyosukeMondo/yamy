# Yamy (Yet Another Mado tsukai no Yuutsu)

**Yamy** is a key binding customization tool for Windows, derived from "Mado tsukai no Yuutsu" (Mayu).

## Overview

The original "Mado tsukai no Yuutsu" (Mayu) used a filter driver to replace key inputs. Yamy changes this approach to use a user-mode hook (`WH_KEYBOARD_LL`) and `SendInput()` API. This allows Yamy to work on modern Windows versions (Vista and later, including 64-bit) without requiring a signed driver, although some low-level replacement capabilities are traded off.

## Status

This repository is a fork of the original Yamy project, reorganized for better maintainability.

- **Original Mayu**: [http://mayu.sourceforge.net/](http://mayu.sourceforge.net/)
- **Original Yamy**: [http://yamy.sourceforge.jp/](http://yamy.sourceforge.jp/)

## Directory Structure

The project structure has been reorganized. Please refer to [README_STRUCTURE.md](README_STRUCTURE.md) for details.

## License and Copyright

### Yamy
**Copyright (C) 2009, KOBAYASHI Yoshiaki <gimy@users.sourceforge.jp>**  
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

(See full license text in [docs/readme.txt](docs/readme.txt))

### Mado tsukai no Yuutsu (Mayu)
**Copyright (C) 1999-2005, TAGA Nayuta <nayuta@users.sourceforge.net>**  
All rights reserved.

(See full license text in [docs/readme.txt](docs/readme.txt))

### Boost C++ Libraries
This software uses Boost C++ Libraries.
**Boost Software License - Version 1.0**

## Acknowledgments

We express our deepest gratitude to **TAGA Nayuta**, the author of "Mado tsukai no Yuutsu", and all contributors who made this software possible.
