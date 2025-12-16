import os
import shutil
import re
from pathlib import Path

# Configuration
SOURCE_ROOT = "/home/rmondo/repos/yamy/docs"
BACKUP_DIR = "/home/rmondo/repos/yamy/docs_backup_pre_reorg"
LOG_FILE = os.path.join(SOURCE_ROOT, "file_organization_log.md")

# Categories
DIRS = {
    "user-manual": ["user-manual", "features", "legacy-ja"],
    "developer-guide": ["developer-guide", "setup", "testing", "architecture", "practices"],
    "project-records": ["project-records", "investigations", "milestones", "status", "legacy-archive"],
    "specs": ["specs", "design-concepts", "implementation-plans"],
    "_review_required": ["_review_required"]
}

# Mapping Logic (Regex Pattern -> Target Path Format)
# Note: Order matters. First match wins.
MAPPINGS = [
    # --- USER MANUAL ---
    (r"user/guide\.md", "user-manual/index.md"),
    (r"user/quick-start\.md", "user-manual/quick-start.md"),
    (r"user/configuration\.md", "user-manual/configuration.md"),
    (r"user/troubleshooting\.md", "user-manual/troubleshooting.md"),
    (r"user/MODAL_MODIFIER_GUIDE\.md", "user-manual/features/modal-modifiers.md"),
    (r"user/NUMBER_MODIFIER_USER_GUIDE\.md", "user-manual/features/number-modifiers.md"),
    (r"user/LINUX-QT-GUI-MANUAL\.md", "user-manual/linux-gui-manual.md"),
    (r"user/(.*)-ja\.html", r"user-manual/legacy-ja/\1-ja.html"),
    (r"user/(.*)-ja\.(png|gif|jpg)", r"user-manual/legacy-ja/\1-ja.\2"),
    (r"user/.*\.css", "user-manual/legacy-ja/styles.css"), # Assuming css is for legacy html
    (r"user/.*\.png", "user-manual/legacy-ja/images/"), # Generic images in user mostly for legacy

    # --- DEVELOPER GUIDE ---
    (r"dev/guide\.md", "developer-guide/index.md"),
    (r"dev/setup/(.*)", r"developer-guide/setup/\1"),
    (r"dev/testing/(.*)", r"developer-guide/testing/\1"),
    (r"dev/architecture/(.*)", r"developer-guide/architecture/\1"),
    (r"dev/guides/(.*)", r"developer-guide/practices/\1"),
    (r"dev/IPC_PROTOCOL\.md", "developer-guide/architecture/ipc-protocol.md"),
    (r"dev/agents\.md", "developer-guide/practices/ai-agents.md"),

    # --- SPECS ---
    (r"dev/design/(.*)", r"specs/design-concepts/\1"),
    (r"archive/JULES-IMPLEMENTATION-GUIDE\.md", "specs/implementation-plans/jules-agent-plan.md"),
    (r"archive/LINUX-GUI-IMPLEMENTATION-PLAN\.md", "specs/implementation-plans/linux-gui-plan.md"),
    (r"archive/SYSTEMATIC_INVESTIGATION_SPEC\.md", "specs/investigation-specs/systematic-investigation.md"),
    
    # --- PROJECT RECORDS (Catch-all for archive) ---
    (r"archive/M00_ROOT_CAUSE_ANALYSIS\.md", "project-records/investigations/issue-m00-hold-detection.md"),
    (r"archive/.*INVESTIGATION.*\.md", "project-records/investigations/"),
    (r"archive/.*ROOT_CAUSE.*\.md", "project-records/investigations/"),
    (r"archive/.*BUGFIX.*\.md", "project-records/investigations/"),
    (r"archive/PHASE.*\.md", "project-records/milestones/phase-reports/"),
    (r"archive/BUILD_STATUS\.md", "project-records/status/build-status-log.md"),
    (r"archive/.*SUMMARY\.md", "project-records/status/summaries/"),
    (r"archive/.*REPORT\.md", "project-records/status/reports/"),
    (r"archive/.*METRICS.*\.md", "project-records/status/metrics/"),
    (r"archive/RELEASE-NOTES.*\.md", "project-records/milestones/release-notes/"),
    (r"reports/(.*)", r"project-records/status/active-reports/\1"),
    
    # Fallback for docs/archive -> project-records/legacy-archive
    (r"archive/(.*)", r"project-records/legacy-archive/\1"),
    
    # Fallback for remaining user docs
    (r"user/(.*)", r"user-manual/misc/\1"),
    
    # Fallback for remaining dev docs
    (r"dev/(.*)", r"developer-guide/misc/\1"),
]

def to_kebab_case(filename):
    # Remove file extension for processing
    name, ext = os.path.splitext(filename)
    # Convert to lower case
    name = name.lower()
    # Replace spaces and underscores with hyphens
    name = re.sub(r'[_\s]+', '-', name)
    return name + ext

def normalize_path_str(path_str):
    if path_str.endswith('/'):
         return path_str # Needs filename appended
    return path_str

def get_target_path(rel_path):
    for pattern, target in MAPPINGS:
        match = re.match(pattern, rel_path)
        if match:
            if target.endswith('/'):
                # It's a directory target, keep original filename (kebab-cased)
                filename = os.path.basename(rel_path)
                return os.path.join(target, to_kebab_case(filename))
            elif '\\1' in target:
                # Regex substitution
                new_path = match.expand(target)
                # Kebab-case the filename part
                directory = os.path.dirname(new_path)
                filename = os.path.basename(new_path)
                return os.path.join(directory, to_kebab_case(filename))
            else:
                 return target
    return None

def main():
    print(f"Starting reorganization of {SOURCE_ROOT}")
    
    # 0. Backup
    # if os.path.exists(BACKUP_DIR):
    #     shutil.rmtree(BACKUP_DIR)
    # shutil.copytree(SOURCE_ROOT, BACKUP_DIR)
    # print(f"Backup created at {BACKUP_DIR}")

    log_entries = []
    
    # 1. Scan and Calculate Moves
    moves = []
    
    for root, _, files in os.walk(SOURCE_ROOT):
        for file in files:
            abs_path = os.path.join(root, file)
            rel_path = os.path.relpath(abs_path, SOURCE_ROOT)
            
            # Skip the script itself or existing new structure if running partially
            if file == "reorg_docs.py" or file == "file_organization_log.md":
                continue
                
            target_rel_path = get_target_path(rel_path)
            
            if target_rel_path:
                target_abs_path = os.path.join(SOURCE_ROOT, target_rel_path)
            else:
                # Default to _review_required
                target_rel_path = os.path.join("_review_required", rel_path.replace("/", "_"))
                target_abs_path = os.path.join(SOURCE_ROOT, target_rel_path)

            moves.append((abs_path, target_abs_path, rel_path, target_rel_path))

    # 2. Execute Moves
    print(f"Computed {len(moves)} moves. Executing...")
    
    for src, dst, rel_src, rel_dst in moves:
        if src == dst:
            continue
            
        dst_dir = os.path.dirname(dst)
        os.makedirs(dst_dir, exist_ok=True)
        
        try:
            shutil.move(src, dst)
            log_entries.append(f"| `{rel_src}` | `{rel_dst}` |")
        except Exception as e:
            print(f"Error moving {src}: {e}")
            log_entries.append(f"| `{rel_src}` | **ERROR: {e}** |")

    # 3. Clean Empty Directories
    for root, dirs, files in os.walk(SOURCE_ROOT, topdown=False):
        for name in dirs:
            try:
                os.rmdir(os.path.join(root, name))
            except OSError:
                pass # Directory not empty

    # 4. Write Log
    with open(LOG_FILE, "w") as f:
        f.write("# File Organization Log\n\n")
        f.write("| Original Path | New Path |\n")
        f.write("| :--- | :--- |\n")
        f.write("\n".join(sorted(log_entries)))
    
    print(f"Reorganization complete. Log written to {LOG_FILE}")

if __name__ == "__main__":
    main()
