# ⚡ Jules Swarm: Parallel Session Tips & Best Practices

**Goal**: Maximize throughput by running multiple AI coding agents (Jules) in parallel, while minimizing the "Management Tax" (conflicts, monitoring, merging).

---

## 🏗️ The "Architect" Mindset
When running a swarm, **you are not the coder**. You are the **Architect**.
*   **Your Job**: Design the components, define the "contracts" (interfaces), and merge the results.
*   **Jules' Job**: Write the boilerplate, implementation details, and tests for *isolated* components.

### ✅ The Golden Rule: STRICT ISOLATION
**Never** assign two concurrent sessions to edit the same file.
*   **Bad**: "Session A: Add method to `engine.cpp`", "Session B: Add variable to `engine.cpp`".
*   **Good**: "Session A: Create `src/core/audio/sound_manager.cpp`", "Session B: Create `src/ui/qt/status_widget.cpp`".
*   **Why**: Merge conflicts in C++ are painful. It is faster to write glue code manually than to resolve 3-way git conflicts.

### 💀 New Best Practice: "The Skeleton-First Strategy" (Approved for Wave 3)
To eliminate "Jules broke the build refactoring include files" issues:
1.  **Architect (You)**: Create the **empty files** (`.h`, `.cpp`) yourself first.
2.  **Architect (You)**: Add them to `CMakeLists.txt` and verify the build system sees them.
3.  **Jules**: Assign the specific prompt: *"Implement login logic inside the existing empty file `src/auth/login.cpp`. Do NOT modify CMakeLists."*
**Result**: Zero build system conflicts. Zero "accidental refactoring" of shared headers.

---

## 🚀 Workflow & Lifecycle

### 1. Preparation & Prompting
*   **Script Generation**: Use scripts (like `scripts/generate_wave2_prompts.ps1`) to generate unique prompt files for each task.
*   **Explicit Instructions**:
    *   "CREATE new files only."
    *   "Do NOT edit core engine files. Mock dependencies if needed."
    *   "Output ONLY the relevant source code."

### 2. Monitoring & Unblocking (The "Stuck" State)
Jules sessions often get stuck in `Awaiting User Feedback` or `Awaiting Plan A`.
**The CLI is limited here. Use the Web UI.**

| Status | Likely Cause | Solution |
| :--- | :--- | :--- |
| **Awaiting Plan A** | Needs approval to start. | **Click "Approve Plan"** in Web UI. |
| **Awaiting Feedback** | "File already exists" or "Need clarification". | **Reply** "Proceed. Overwrite if necessary." |
| **Completed** | Done! | Run `jules remote pull --session <ID> --apply`. |

### 3. Merging Strategies
*   **The Happy Path**: `jules remote pull --session <ID> --apply` works perfectly.
*   **The Conflict Path**:
    1.  Save the patch: `jules remote pull --session <ID> > patch_name.diff`
    2.  Check content: `view_file patch_name.diff`
    3.  **Manual Cherry-Pick**: If the patch is messy, just read the implementation in the patch file and manually creating the clean source files (`write_to_file`) is often faster and cleaner than fighting `git apply`.

---

## 🛠️ Troubleshooting & "Gotchas"

### ⚠️ Binary Files
**Jules cannot handle binary files (images, sounds, .wav, .png) well in patches.**
*   **Symptom**: Patch fails with "binary file differs" or garbage characters.
*   **Fix**: Do not ask Jules to generate binaries. Ask for placeholders or empty files, then add the real assets yourself.

### ⚠️ "File Already Exists"
*   **Scenario**: You asked for a new file, but a previous session or legacy code created it.
*   **Fix**: Reply: "Refactor the existing file to meet the requirements." or "Overwrite it."

### ⚠️ Redundant Sessions
*   **Scenario**: Two sessions end up doing similar things.
*   **Decision**: **Do not kill them.** Let them finish.
*   **Why**: It's cheap to let them run. You can audit both solutions and pick the best one, or merge the unique features of each (e.g., Session A's Logic + Session B's UI).

---

## 📝 Copy-Paste Replies
Keep these handy for the Web UI to unblock sessions quickly:

**For "Should I create files?":**
> Proceed. Create the new files as requested.

**For "File exists / Conflict":**
> Proceed. Overwrite the existing file or refactor it to meet the requirements.

**For "Missing Dependency (Engine/Manager)":**
> Create a local stub or checking class. Do not modifying the core Engine files.
