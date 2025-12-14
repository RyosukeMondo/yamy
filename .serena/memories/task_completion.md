# Task Completion Checklist
- Update `.spec-workflow/specs/{spec}/tasks.md`: mark task in progress `[-]` before coding and `[x]` after completion and logging.
- Search existing implementation logs under `.spec-workflow/specs/{spec}/Implementation Logs/` to avoid duplicate endpoints/components/functions before coding.
- Implement changes following spec/task prompt and style conventions.
- Run relevant builds/tests (e.g., `cmake --build build_release`, `./quick-test.sh` from build dir, or other targeted checks). Capture issues.
- Record implementation details with `log-implementation` tool: include taskId, summary, files touched, stats, and artifacts (apiEndpoints/components/functions/classes/integrations).
- Commit work in atomic units with `git add` + `git commit -m "..."` as required by user instructions.
- Keep repo clean; do not revert user changes. Coordinate with spec workflow approvals if creating/updating spec documents.