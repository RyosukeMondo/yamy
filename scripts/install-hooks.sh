#!/usr/bin/env bash
#
# Install Git hooks for YAMY development
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

echo "Installing Git hooks for YAMY..."

# Check if we're in a git repository
if [ ! -d "$REPO_ROOT/.git" ]; then
    echo "Error: Not in a git repository"
    exit 1
fi

# Create hooks directory if it doesn't exist
mkdir -p "$HOOKS_DIR"

# Install pre-commit hook for code metrics
echo "Installing pre-commit hook for code metrics..."
cat > "$HOOKS_DIR/pre-commit" << 'EOF'
#!/usr/bin/env bash
# Pre-commit hook: Run code metrics check
exec "$(git rev-parse --show-toplevel)/scripts/pre-commit-metrics.sh"
EOF

chmod +x "$HOOKS_DIR/pre-commit"

echo ""
echo "✓ Git hooks installed successfully!"
echo ""
echo "Installed hooks:"
echo "  - pre-commit: Code metrics enforcement (lizard)"
echo ""
echo "To bypass hooks temporarily, use: git commit --no-verify"
echo ""
echo "Note: Make sure lizard is installed: pip install lizard"
