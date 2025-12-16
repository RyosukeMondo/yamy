# The "Jules Swarm" Workflow: Parallel AI Coding Guide

This guide documents the effective method for utilizing `jules` (Google's AI coding agent) to execute large-scale refactoring and feature implementation in parallel.

## The Core Philosophy: "Architect & Builders"

**Lesson from Wave 1**: You must shift your role from "Manager" to **"Architect"**.
*   **The Architect (You)**: Designs the components, defines the interfaces, and creates the empty files/hooks.
*   **The Builders (Jules)**: Fill in the implementations of specific, isolated components.

> [!IMPORTANT]
> **Rule of Thumb**: Never assign two sessions to edit the same existing file (e.g., `engine.cpp`) simultaneously. It guarantees merge conflicts.

## Step-by-Step Workflow

### 1. Preparation: Component Isolation
Break features down into **new files**.
*   *Bad*: "Session A: Add logging to Engine. Session B: Add notifications to Engine." (Both edit `engine.cpp`)
*   *Good*: "Session A: Create `Logger` class. Session B: Create `NotificationManager` class." (No overlap)

### 2. The Prompt Factory
Create a **Self-Contained Prompt** for each session.
1.  **Role**: "You are an expert C++ developer..."
2.  **Goal**: "Implement the [Component Name] class."
3.  **Context**: "Refer to `tasks.md`."
4.  **Constraint**: "Create new files `src/foo/bar.cpp`. Do NOT edit `main.cpp` or `engine.cpp` without explicit instruction."

### 3. Dispatching (The "Swarm")
Use the CLI to launch sessions rapidly.

```bash
# Windows PowerShell
type prompt_session_a.txt | jules new
type prompt_session_b.txt | jules new
```

### 4. Monitoring & Advanced Management
The default `jules remote list` can act inconsistently. Use these "Pro" commands:

**Accurate Status Checking:**
*   **Dump to File**: `jules remote list --session > sessions.txt` (Untruncated view).
*   **Check PRs**: `gh pr list` (Best indicator of completion).

**Handling "Stuck" Sessions:**
*   **Inspecting**: `jules remote pull --session <ID>` (No `--apply`).
*   **Forcing**: `jules remote pull --session <ID> --apply`.
*   **Fixing Rejections**:
    1.  `jules remote pull --session <ID> > session.patch`
    2.  `git apply --reject session.patch`
    3.  Manually fix `.rej` files.

### 5. Closing the Loop
*   Review the code (AI makes logic errors!).
*   Manually merge if needed.
*   Close the PR: `gh pr close <PR_ID> --comment "Merged manually."`

## Wave Strategy

| Wave | Focus | Strategy | Risk |
| :--- | :--- | :--- | :--- |
| **1** | Core Backends | Independent Classes | Low |
| **2** | UI Components | Isolated Widgets (`LogDialog`, `ConfigPanel`) | Medium |
| **3** | Integration | **Serial Execution** (You or single agent) | High (Shared Glue) |

## Troubleshooting

### Session Identification: "Which session is which?"
When running many sessions, `jules remote list` can be confusing.
**Solution**: Use a mapping script or manually record IDs when launching.
*   **Best Practice**: When dispatching, log the output: `type prompt.txt | jules new >> session_log.txt`
*   **Recovery**: Use `scripts/track_sessions.ps1` to match active sessions back to their prompt files based on the Task description.

### Conflict: "File Already Exists"
If Jules complains that a file exists when you asked for "NEW FILES ONLY":
1.  **Verify**: Check if the existing file is legacy code or a previous wave.
2.  **Pivot**: Reply to the session (via web UI):
    > "Refactor the existing file to meet the requirements. Ignore the 'Create New Files' constraint."
3.  **Do Not Force**: Do not try to overwrite blindly.
