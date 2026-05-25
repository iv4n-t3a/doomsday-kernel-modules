#!/usr/bin/env bats

bats_require_minimum_version 1.5.0

MODULE="penguin_fat16"
PROJ_DIR="$(cd "$(dirname "$BATS_TEST_FILENAME")/.." && pwd)"
KO="$PROJ_DIR/kmodule/$MODULE.ko"

img()  { cat "$BATS_FILE_TMPDIR/img";  }
loop() { cat "$BATS_FILE_TMPDIR/loop"; }
mnt()  { cat "$BATS_FILE_TMPDIR/mnt";  }

setup_file() {
    if [[ $EUID -ne 0 ]]; then
        echo "Tests must run as root (use sudo)" >&2
        exit 1
    fi

    [[ -f "$KO" ]] || { echo "$KO not found — run make first" >&2; exit 1; }
    command -v mkfs.fat >/dev/null || { echo "mkfs.fat not found (install dosfstools)" >&2; exit 1; }

    local img; img=$(mktemp /tmp/fat16_XXXXXX.img)
    local mnt; mnt=$(mktemp -d /tmp/fat16_mnt_XXXXXX)
    echo "$img" > "$BATS_FILE_TMPDIR/img"
    echo "$mnt" > "$BATS_FILE_TMPDIR/mnt"

    dd if=/dev/zero of="$img" bs=1M count=16 2>/dev/null
    mkfs.fat -F 16 "$img" >/dev/null

    local loop; loop=$(losetup -f --show "$img")
    echo "$loop" > "$BATS_FILE_TMPDIR/loop"

    local setup_mnt; setup_mnt=$(mktemp -d /tmp/fat16_setup_XXXXXX)
    mount -t vfat "$loop" "$setup_mnt"

    printf 'hello'          > "$setup_mnt/HELLO.TXT"
    touch                     "$setup_mnt/EMPTY.TXT"
    mkdir                     "$setup_mnt/SUBDIR"
    printf 'inner'          > "$setup_mnt/SUBDIR/INNER.TXT"

    dd if=/dev/urandom of=/tmp/fat16_ref.bin bs=520 count=1 2>/dev/null
    cp /tmp/fat16_ref.bin     "$setup_mnt/BIG.BIN"
    echo "$RANDOM$RANDOM"   > "$setup_mnt/SUBDIR/NESTED.TXT"

    umount "$setup_mnt"
    rmdir  "$setup_mnt"

    insmod "$KO"
    mount -t "$MODULE" "$loop" "$mnt"
}

teardown_file() {
    umount "$(mnt)"  2>/dev/null || true
    rmdir  "$(mnt)"  2>/dev/null || true
    rmmod  "$MODULE" 2>/dev/null || true
    losetup -d "$(loop)" 2>/dev/null || true
    rm -f "$(img)" /tmp/fat16_ref.bin
}

@test "module is loaded after insmod" {
    run lsmod
    [[ "$output" == *"$MODULE"* ]]
}

@test "mountpoint is accessible" {
    run stat "$(mnt)"
    [ "$status" -eq 0 ]
}

@test "filesystem type is reported correctly by findmnt" {
    run findmnt -n -o FSTYPE "$(mnt)"
    [ "$status" -eq 0 ]
    [ "$output" = "$MODULE" ]
}

@test "root listing succeeds" {
    run ls "$(mnt)"
    [ "$status" -eq 0 ]
}

@test "root contains HELLO.TXT" {
    run ls "$(mnt)"
    [[ "$output" == *"HELLO.TXT"* ]]
}

@test "root contains EMPTY.TXT" {
    run ls "$(mnt)"
    [[ "$output" == *"EMPTY.TXT"* ]]
}

@test "root contains SUBDIR" {
    run ls "$(mnt)"
    [[ "$output" == *"SUBDIR"* ]]
}

@test "root contains BIG.BIN" {
    run ls "$(mnt)"
    [[ "$output" == *"BIG.BIN"* ]]
}

@test "HELLO.TXT has correct size" {
    run stat -c '%s' "$(mnt)/HELLO.TXT"
    [ "$status" -eq 0 ]
    [ "$output" -eq 5 ]
}

@test "EMPTY.TXT has zero size" {
    run stat -c '%s' "$(mnt)/EMPTY.TXT"
    [ "$status" -eq 0 ]
    [ "$output" -eq 0 ]
}

@test "BIG.BIN has correct size" {
    run stat -c '%s' "$(mnt)/BIG.BIN"
    [ "$status" -eq 0 ]
    [ "$output" -eq 520 ]
}

@test "HELLO.TXT is a regular file" {
    run stat -c '%F' "$(mnt)/HELLO.TXT"
    [ "$status" -eq 0 ]
    [ "$output" = "regular file" ]
}

@test "SUBDIR is a directory" {
    run stat -c '%F' "$(mnt)/SUBDIR"
    [ "$status" -eq 0 ]
    [ "$output" = "directory" ]
}

@test "lookup nonexistent file returns error" {
    run stat "$(mnt)/NOSUCH.TXT"
    [ "$status" -ne 0 ]
}

@test "inode numbers are nonzero" {
    run stat -c '%i' "$(mnt)/HELLO.TXT"
    [ "$status" -eq 0 ]
    [ "$output" -gt 0 ]
}

@test "inode numbers are unique across root entries" {
    local i1 i2 i3
    i1=$(stat -c '%i' "$(mnt)/HELLO.TXT")
    i2=$(stat -c '%i' "$(mnt)/EMPTY.TXT")
    i3=$(stat -c '%i' "$(mnt)/SUBDIR")
    [ "$i1" -ne "$i2" ]
    [ "$i1" -ne "$i3" ]
    [ "$i2" -ne "$i3" ]
}

@test "filesystem is mounted read-only" {
    run touch "$(mnt)/NEWFILE.TXT"
    [ "$status" -ne 0 ]
}

@test "SUBDIR listing succeeds" {
    run ls "$(mnt)/SUBDIR"
    [ "$status" -eq 0 ]
}

@test "SUBDIR contains INNER.TXT" {
    run ls "$(mnt)/SUBDIR"
    [[ "$output" == *"INNER.TXT"* ]]
}

@test "SUBDIR contains NESTED.TXT" {
    run ls "$(mnt)/SUBDIR"
    [[ "$output" == *"NESTED.TXT"* ]]
}

@test "INNER.TXT has correct size" {
    run stat -c '%s' "$(mnt)/SUBDIR/INNER.TXT"
    [ "$status" -eq 0 ]
    [ "$output" -eq 5 ]
}

@test "nested lookup for nonexistent file returns error" {
    run stat "$(mnt)/SUBDIR/NOSUCH.TXT"
    [ "$status" -ne 0 ]
}

@test "inode numbers are unique between root and subdir entries" {
    local i1 i2
    i1=$(stat -c '%i' "$(mnt)/HELLO.TXT")
    i2=$(stat -c '%i' "$(mnt)/SUBDIR/INNER.TXT")
    [ "$i1" -ne "$i2" ]
}

@test "reading HELLO.TXT returns correct content" {
    run cat "$(mnt)/HELLO.TXT"
    [ "$status" -eq 0 ]
    [ "$output" = "hello" ]
}

@test "reading INNER.TXT returns correct content" {
    run cat "$(mnt)/SUBDIR/INNER.TXT"
    [ "$status" -eq 0 ]
    [ "$output" = "inner" ]
}

@test "reading BIG.BIN matches original bytes" {
    run cmp "$(mnt)/BIG.BIN" /tmp/fat16_ref.bin
    [ "$status" -eq 0 ]
}

