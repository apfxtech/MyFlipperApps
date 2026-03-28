# Contributing
## Commit format
A strict format is used:
```
<type>(<scope>): <message>
```
* `<type>` — exactly 3 characters
* `<scope>` — application / module name
* `<message>` — short and to the point
* all commits are automatically added to the changelog

### Types
* `fix` — Bug fix
* `add` — New feature
* `ref` — Refactor
* `tst` — Tests
* `opt` — Performance
* `sec` — Security
* `rmv` — Remove
* `dep` — Dependencies
* `doc` — Docs
* `bld` — Build
* `cii` — CI/CD
* `cfg` — Config
* `cln` — Cleanup

### Example
```
fix(arduventure): crash on start
add(castleboy): new level system
bld(workflow): improve build speed
```