# Release Process

1. Create a PR "Preparing for release X.Y.Z" against master branch
    * Alter CHANGELOG.md from `<placeholder_version> (unreleased)` to `<X.Y.Z> (YYYY-MM-DD)`
2. Create a release "Release X.Y.Z" on Github
    * Create Tag `vX.Y.Z`
    * Copy CHANGELOG.md into the release notes
3. Create a PR "Back to development" against master branch
    * Add `<next_version> (unreleased)` to CHANGELOG.md
    * Update project version in CMakeLists.txt to the `<next version>`