# The "Jules Swarm" Workflow: Parallel AI Coding Guide

This guide documents the effective method for utilizing `jules` (Google's AI coding agent) to execute large-scale refactoring and feature implementation in parallel.

## The Core Philosophy: "Manager & Workers"

The key to this workflow is treating yourself (or the primary agent) as the **Engineering Manager** and the remote `jules` sessions as **Individual Contributors**.

**Success Formula:** `Structured Tasks + Contextual Prompts + CLI Automation = High Velocity`

## Step-by-Step Workflow

### 1. Preparation: The "Spec" (Critical Step)
You cannot parallelize chaos. You need a strict, granular task list.
*   **Format**: A `tasks.md` file (like in `.spec-workflow/specs/`).
*   **Granularity**: Each task should be clear enough that a junior engineer could do it without asking questions.
*   **Dependencies**: Explicitly note dependencies. Group independent tasks together.

### 2. Strategy: Batching & Wave Planning
Identify tasks that touch *different files* or *different modules*.
*   **Wave 1**: Independent backends (e.g., Log Backend, Notification Backend) + Isolated fixes.
*   **Wave 2**: UI components (dependent on Wave 1).
*   **Wave 3**: Integration.

### 3. The Prompt Factory
Don't just say "Do task X". Create a **Self-Contained Prompt** for each session.
A perfect prompt includes:
1.  **Role**: "You are an expert C++ developer..."
2.  **Goal**: "Implement the Logging Backend."
3.  **Context**: "Refer to `tasks.md` for full specs."
4.  **Steps**: Copy/paste the exact checklist from `tasks.md`.
5.  **Constraints**: "Do not implement the UI yet."

**Pro Tip**: Save these as temporary text files (`prompt_session_a.txt`).

### 4. Dispatching (The "Swarm")
Use the CLI to launch sessions rapidly without context switching.

```bash
# Windows PowerShell
type prompt_session_a.txt | jules new
type prompt_session_b.txt | jules new
```

This pipes your prepared instructions directly into a new session.

### 5. Monitoring & Merging
*   **Monitor**: `jules remote list` shows you what's active.
*   **Merge**: When a session is `Done`, review the PR/Changes using `jules remote pull <ID>`.

### 6. Advanced Session Management (The "Manager's Console")
The default `jules remote list` can sometimes truncate output or show cached states.

**Accurate Status Checking:**
*   **Dump to File**: `jules remote list --session > sessions.txt` gives you the full, untruncated output to check status messages like "Awaiting User Feedback".
*   **Check PRs**: `gh pr list` often gives a better indication of "Done" state than the session list.

**Handling "Stuck" Sessions:**
*   **Awaiting Feedback**: If a session is waiting for feedback, you can often just `pull` what it has done so far.
    *   `jules remote pull --session <ID>` (without `--apply`) to inspect.
    *   `jules remote pull --session <ID> --apply` to force apply.
*   **Manual Intervention**: If the AI makes a logic error (e.g., malformed code blocks), plain `git apply` will fail.
    *   Redirect output to a patch: `jules remote pull --session <ID> > session.patch`
    *   Apply with reject: `git apply --reject session.patch`
    *   Manually fix the `.rej` files and the code.

**Closing the Loop:**
*   After manually merging a session, close its PR: `gh pr close <PR_ID> --comment "Manually merged."`
*   This keeps your dashboard clean for the next wave.

## Example Structure

| Session | Focus Area | Files Touched | Risk of Conflict |
| :--- | :--- | :--- | :--- |
| **A** | Core Fixes | `stringtool.cpp`, `input_hook.cpp` | Low |
| **B** | Investigate UI | `dialog_investigate.cpp` | Low |
| **C** | Log Backend | `logger.cpp` (New File) | None |

## Why This Works
*   **Context Isolation**: Each agent focuses on a narrow slice.
*   **No Waiting**: You don't wait for one task to finish before starting the next.
*   **Forced Clarity**: Writing the prompt forces you to clearly define the task, which reduces AI errors.
