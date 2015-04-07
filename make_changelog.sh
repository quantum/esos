#! /bin/sh

# Get the last commit message
if ! msg="$(git log -1 --pretty=%B)"; then
    exit 1
fi

# Generate the new ChangeLog file
./gitlog-to-changelog --ignore-matching="updated changelog" > ChangeLog

# Stage the new file and commit it
git add ChangeLog || exit 1
git commit --amend -m "${msg}" || exit 1

