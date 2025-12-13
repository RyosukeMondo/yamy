#!/bin/bash
# Verify Scan Code Mappings - Compare what we mapped vs what 109.mayu defines

echo "=============================================="
echo "Scan Code Mapping Verification"
echo "=============================================="
echo ""
echo "Comparing g_evdevToYamyMap vs 109.mayu definitions"
echo ""

# Function to get scan code from 109.mayu
get_109_scancode() {
    local keyname="$1"
    # Handle different key name variations
    case "$keyname" in
        "A"|"B"|"C"|"D"|"E"|"F"|"G"|"H"|"I"|"J"|"K"|"L"|"M"|"N"|"O"|"P"|"Q"|"R"|"S"|"T"|"U"|"V"|"W"|"X"|"Y"|"Z")
            grep -E "^def key $keyname\s+" ../keymaps/109.mayu | awk '{print $5}' | head -1
            ;;
        "Tab")
            grep -E "^def key Tab\s+" ../keymaps/109.mayu | awk '{print $3}' | head -1
            ;;
        "Enter"|"Return")
            grep -E "^def key Enter" ../keymaps/109.mayu | awk '{print $4}' | head -1
            ;;
        "Backspace"|"BS")
            grep -E "^def key BackSpace" ../keymaps/109.mayu | awk '{print $5}' | head -1
            ;;
        "Esc"|"Escape")
            grep -E "^def key Esc" ../keymaps/109.mayu | awk '{print $4}' | head -1
            ;;
        "Space")
            grep -E "^def key Space\s+" ../keymaps/109.mayu | awk '{print $3}' | head -1
            ;;
        "_"*)
            local num="${keyname#_}"
            grep -E "^def key _$num\s+" ../keymaps/109.mayu | awk '{print $3}' | head -1
            ;;
        "F"[0-9]*)
            grep -E "^def key $keyname\s+" ../keymaps/109.mayu | awk '{print $3}' | head -1
            ;;
        *)
            echo "???"
            ;;
    esac
}

# Function to get our mapped scan code from keycode_mapping.cpp
get_mapped_scancode() {
    local evdev="$1"
    grep "{KEY_${evdev}," ../src/platform/linux/keycode_mapping.cpp | \
        grep "g_evdevToYamyMap" -A 100 | \
        grep "KEY_${evdev}" | \
        head -1 | \
        sed 's/.*0x/0x/' | \
        awk '{print $1}' | \
        sed 's/},//'
}

# Test key letters
echo "Letters:"
echo "Key | evdev_name | Our_Scan | 109_Scan | Match"
echo "----|------------|----------|----------|------"

for key in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; do
    our_scan=$(get_mapped_scancode "$key")
    expected_scan=$(get_109_scancode "$key")

    if [ "$our_scan" = "$expected_scan" ]; then
        match="✓"
    else
        match="✗"
    fi

    printf "%-3s | KEY_%-6s | %-8s | %-8s | %s\n" "$key" "$key" "$our_scan" "$expected_scan" "$match"
done

echo ""
echo "Numbers:"
echo "Key | evdev_name | Our_Scan | 109_Scan | Match"
echo "----|------------|----------|----------|------"

for i in 0 1 2 3 4 5 6 7 8 9; do
    our_scan=$(get_mapped_scancode "$i")
    expected_scan=$(get_109_scancode "_$i")

    if [ "$our_scan" = "$expected_scan" ]; then
        match="✓"
    else
        match="✗"
    fi

    printf "%-3s | KEY_%-6s | %-8s | %-8s | %s\n" "$i" "$i" "$our_scan" "$expected_scan" "$match"
done

echo ""
echo "Special Keys:"
echo "Key       | evdev_name | Our_Scan | 109_Scan | Match"
echo "----------|------------|----------|----------|------"

declare -A special_keys=(
    ["TAB"]="Tab"
    ["ENTER"]="Enter"
    ["BACKSPACE"]="BackSpace"
    ["ESC"]="Esc"
    ["SPACE"]="Space"
)

for evdev_name in "${!special_keys[@]}"; do
    key_109="${special_keys[$evdev_name]}"
    our_scan=$(get_mapped_scancode "$evdev_name")
    expected_scan=$(get_109_scancode "$key_109")

    if [ "$our_scan" = "$expected_scan" ]; then
        match="✓"
    else
        match="✗"
    fi

    printf "%-9s | KEY_%-6s | %-8s | %-8s | %s\n" "$key_109" "$evdev_name" "$our_scan" "$expected_scan" "$match"
done

echo ""
echo "Function Keys:"
echo "Key  | evdev_name | Our_Scan | 109_Scan | Match"
echo "-----|------------|----------|----------|------"

for i in 1 2 3 4 5 6 7 8 9 10 11 12; do
    our_scan=$(get_mapped_scancode "F$i")
    expected_scan=$(get_109_scancode "F$i")

    if [ "$our_scan" = "$expected_scan" ]; then
        match="✓"
    else
        match="✗"
    fi

    printf "F%-3s | KEY_F%-5s | %-8s | %-8s | %s\n" "$i" "$i" "$our_scan" "$expected_scan" "$match"
done
