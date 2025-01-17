# Release process

This document describes the steps required to make a new node.gl release.

1. Make sure the [Changelog](/CHANGELOG.md) is not missing any entry
2. Run `./scripts/make_release.py` with no argument from the root directory in a
   clean and up-to-date git state
3. Indicate the new `libnodegl` version as prompted, following the semantic
   versioning convention (the version can remain identical)
4. Check the last commit and tag
5. Run `git push -n && git push -n --tags`
6. If everything looks sane and the release commit has been approved by another
   developer, run `git push && git push --tags`
