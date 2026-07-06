#!/bin/sh
# Copyright (c) 2012-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
if [ $# -gt 1 ]; then
    cd "$2" || exit 1
fi
if [ $# -gt 0 ]; then
    FILE="$1"
    shift
    if [ -f "$FILE" ]; then
        INFO="$(head -n 1 "$FILE")"
    fi
else
    echo "Usage: $0 <filename> <srcroot>"
    exit 1
fi

GIT_TAG=""
GIT_COMMIT=""
GIT_BRANCH=""
if [ "${BITCOIN_GENBUILD_NO_GIT}" != "1" ] && [ -e "$(command -v git)" ] && [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ]; then
    # clean 'dirty' status of touched files that haven't been modified
    git diff >/dev/null 2>/dev/null

    # Get current branch name (filter out HEAD if detached)
    BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
    # Only include branch name if it's not "HEAD" (detached HEAD state) and not a release branch
    if [ -n "$BRANCH_NAME" ] && [ "$BRANCH_NAME" != "HEAD" ]; then
        # Filter out common release branch patterns - don't include for tags/releases
        case "$BRANCH_NAME" in
            master|main|release/*|v[0-9]*)
                # Don't include branch name for release branches
                ;;
            *)
                # Sanitize branch name for inclusion in version string
                # Remove common prefixes (feature/, fix/, hotfix/, etc.)
                SANITIZED_BRANCH=$(echo "$BRANCH_NAME" | sed -e 's|^feature/||' -e 's|^fix/||' -e 's|^hotfix/||' -e 's|^bugfix/||')
                # Limit to 20 characters max to avoid excessively long version strings
                if [ ${#SANITIZED_BRANCH} -le 20 ]; then
                    GIT_BRANCH="$SANITIZED_BRANCH"
                else
                    # If too long, just use first 17 chars + "..."
                    GIT_BRANCH="${SANITIZED_BRANCH:0:17}..."
                fi
                ;;
        esac
    fi

    # if latest commit is tagged and not dirty, then override using the tag name
    RAWDESC=$(git describe --abbrev=0 2>/dev/null)
    if [ "$(git rev-parse HEAD)" = "$(git rev-list -1 $RAWDESC 2>/dev/null)" ]; then
        git diff-index --quiet HEAD -- && GIT_TAG=$RAWDESC
    fi

    # otherwise generate suffix from git, i.e. string like "59887e8-dirty"
    if [ -z "$GIT_TAG" ]; then
        GIT_COMMIT=$(git rev-parse --short=12 HEAD)
        git diff-index --quiet HEAD -- || GIT_COMMIT="$GIT_COMMIT-dirty"
    fi
fi

if [ -n "$GIT_TAG" ]; then
    NEWINFO="#define BUILD_GIT_TAG \"$GIT_TAG\""
elif [ -n "$GIT_COMMIT" ]; then
    if [ -n "$GIT_BRANCH" ]; then
        NEWINFO="#define BUILD_GIT_COMMIT \"$GIT_BRANCH-$GIT_COMMIT\""
    else
        NEWINFO="#define BUILD_GIT_COMMIT \"$GIT_COMMIT\""
    fi
else
    NEWINFO="// No build information available"
fi

# only update build.h if necessary
if [ "$INFO" != "$NEWINFO" ]; then
    echo "$NEWINFO" >"$FILE"
fi
