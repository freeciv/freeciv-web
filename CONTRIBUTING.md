# Contributing to Freeciv-Web

Thank you for your interest in contributing!

## Getting Started

The developers of this project can be found in freenode IRC #freeciv-dev
and in this discord [guild](https://discord.gg/X8bKbry), channel #dev. If
you want an idea for a good starting project, poke someone in there first.

## Pull Request Guidelines

- Try not to exceed 200 LOC +- in a single PR. Larger pull-requests are also 
  welcome, if the improvement or refactoring requires larger changes.
- Avoid whitespace-only or other trivial changes that do not change logic.
- Always test your changes properly.

The best thing you can do is to keep commits and PRs contextual. Small PRs
are great, as they're easy to review and approve. Large, multi-faceted PRs
are more difficult to review.

A PR should generally have a purpose other than "fixing code"; changing
lines just for the sake of style or syntax obscures the commit history of a
file. In other words, the why of a line of code's existence can usually be
determined by the comments in the previous commit for that line and those
around it - git blame is used to see why a change was made, understand the
context, and determine if, and how it can/should be modified. To change
code simply for the sake of whitespace, or re-arranging lines is frowned
upon, because it unnecessarily hides/buries history.
