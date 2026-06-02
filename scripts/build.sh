#!/bin/bash
set -euo pipefail

# ============================================================
# Build script for Cocos2dxAnimationEditor
# ============================================================

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || echo 8)
JOBS=$((NPROC > 8 ? 8 : NPROC))

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    cat <<EOF
Usage: build.sh [OPTIONS]

Options:
    --all       Build all branches (develop + 6 feature branches)
    --test      Run tests after build
    --sync      Merge develop into all feature branches + init submodules
    --clean     Clean build directories before building
    -j N        Number of parallel jobs (default: $JOBS)

Examples:
    build.sh                  # Build current branch
    build.sh --all            # Build all 7 branches
    build.sh --all --test     # Build all and run tests
    build.sh --sync           # Sync develop → all feature branches
    build.sh --sync --build   # Sync + build all
    build.sh --clean --all    # Clean rebuild all
EOF
    exit 0
}

WORKTREES=(
    "develop:$ROOT"
    "feature/core:$ROOT/.claude/worktrees/feature-core"
    "feature/files:$ROOT/.claude/worktrees/feature-files"
    "feature/nodetree:$ROOT/.claude/worktrees/feature-nodetree"
    "feature/timeline:$ROOT/.claude/worktrees/feature-timeline"
    "feature/preview:$ROOT/.claude/worktrees/feature-preview"
    "feature/animation:$ROOT/.claude/worktrees/feature-animation"
)

FEATURE_WORKTREES=(
    "feature/core:$ROOT/.claude/worktrees/feature-core"
    "feature/files:$ROOT/.claude/worktrees/feature-files"
    "feature/nodetree:$ROOT/.claude/worktrees/feature-nodetree"
    "feature/timeline:$ROOT/.claude/worktrees/feature-timeline"
    "feature/preview:$ROOT/.claude/worktrees/feature-preview"
    "feature/animation:$ROOT/.claude/worktrees/feature-animation"
)

get_branch() { git branch --show-current; }

build_one() {
    local name=$1 path=$2
    mkdir -p "$path/build"
    printf "${YELLOW}[%s]${NC} configuring...\n" "$name"
    cmake -B "$path/build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        > "$path/build/cmake.log" 2>&1 || {
        printf "${RED}[%s] CMake configure FAILED${NC}\n" "$name"
        cat "$path/build/cmake.log" | tail -20
        return 1
    }
    printf "${YELLOW}[%s]${NC} building ( -j$JOBS )...\n" "$name"
    cmake --build "$path/build" -j"$JOBS" > "$path/build/build.log" 2>&1 || {
        printf "${RED}[%s] Build FAILED${NC}\n" "$name"
        tail -20 "$path/build/build.log"
        return 1
    }
    printf "${GREEN}[%s] OK${NC}\n" "$name"
}

test_one() {
    local name=$1 path=$2
    if [ ! -f "$path/build/CTestTestfile.cmake" ]; then
        return 0
    fi
    ctest --test-dir "$path/build" --output-on-failure > "$path/build/test.log" 2>&1 || {
        printf "  ${RED}tests FAILED${NC}\n"
        tail -10 "$path/build/test.log"
        return 1
    }
    local passed=$(grep -c "Passed" "$path/build/test.log" 2>/dev/null || echo "?")
    printf "  tests: ${passed} passed\n"
}

# Ensure third_party is a symlink to the main repo (avoids ~138M duplication per worktree)
ensure_third_party_symlink() {
    local path=$1
    if [ -L "$path/third_party" ]; then
        return 0  # already a symlink
    fi
    if [ -d "$path/third_party" ]; then
        rm -rf "$path/third_party"
    fi
    ln -s ../../../third_party "$path/third_party"
}

sync_branches() {
    echo "=== Syncing develop → feature branches ==="
    for entry in "${FEATURE_WORKTREES[@]}"; do
        local name=${entry%%:*} path=${entry#*:}
        printf "${YELLOW}[%s]${NC} " "$name"
        cd "$path"

        # Clean up any stale cherry-pick or rebase state
        git cherry-pick --abort 2>/dev/null || true
        git rebase --abort 2>/dev/null || true

        # Already in sync?
        if [ "$(git merge-base HEAD develop)" = "$(git rev-parse develop)" ]; then
            echo -n "up to date, "
        else
            git merge develop -m "merge: sync from develop" >/dev/null 2>&1 && \
                echo -n "merged, " || { printf "${RED}merge conflict${NC}\n"; continue; }
        fi
        ensure_third_party_symlink "$path" && \
            echo "third_party OK" || echo "${RED}third_party fail${NC}"
    done
    cd "$ROOT"
}

main() {
    local all=false do_test=false do_sync=false do_clean=false

    while [ $# -gt 0 ]; do
        case "$1" in
            --all)     all=true ;;
            --test)    do_test=true ;;
            --sync)    do_sync=true ;;
            --clean)   do_clean=true ;;
            -j)        shift; JOBS=$1 ;;
            --help|-h) usage ;;
            *)         echo "Unknown: $1"; usage ;;
        esac
        shift
    done

    # Sync first if requested
    if $do_sync; then
        sync_branches
        $all || exit 0
    fi

    # Determine targets
    if $all; then
        targets=("${WORKTREES[@]}")
    else
        local cur_branch=$(get_branch)
        local cur_path=$(git rev-parse --show-toplevel)
        targets=("$cur_branch:$cur_path")
    fi

    # Clean if requested
    if $do_clean; then
        for entry in "${targets[@]}"; do
            local path=${entry#*:}
            rm -rf "$path/build"
        done
    fi

    # Ensure submodules are initialized in main repo
    cd "$ROOT"
    git submodule update --init --recursive >/dev/null 2>&1 || true

    local failed=0
    for entry in "${targets[@]}"; do
        local name=${entry%%:*} path=${entry#*:}
        # Worktrees share third_party via symlink to avoid duplicating ~187M each
        if [ "$name" != "develop" ]; then
            ensure_third_party_symlink "$path"
        fi
        echo ""
        echo "──── ${name} ────"
        build_one "$name" "$path" || { failed=$((failed+1)); continue; }
        if $do_test; then
            test_one "$name" "$path"
        fi
    done

    echo ""
    if [ $failed -eq 0 ]; then
        printf "${GREEN}All builds passed${NC}\n"
    else
        printf "${RED}%d branch(es) failed${NC}\n" $failed
        exit 1
    fi
}

main "$@"
