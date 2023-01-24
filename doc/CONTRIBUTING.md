# Contributing to Freeciv-Web

Thank you for your interest in contributing to Freeciv-web!

## Getting Started

Freeciv is an open source project where contributions from the open source community is encouraged.
There are multiple versions of Freeciv-web, and Freeciv-web can be hosted on multiple server instances.
To get involved in one of these Freeciv-web instances, you may either contribute directly to the
github.com/freeciv/freeciv-web repository by submitting a pull-request there, or contact one of the fork projects
of Freeciv-web on GitHub, IRC, e-mailing lists, Discord or similar.

## Multiple Freeciv-web servers
We need to recognize that there are multiple competing Freeciv-web servers, and that the main
freeciv/freeciv-web github repository must be generic and not specific to one site.
We will get a better Freeciv-web service if we allow multiple servers to exists and compete.

## Pull Request Guidelines

- All changes to Freeciv-web must be done using Pull Requests on GitHub. Don't commit changes directly to the master or develop branch.
- Pull requests must be approved by two other Freeciv developers. Minor changes can be approved by one Freeciv developer. If there's no objections within 72h to the pull request, it's ok to merge it.
- Only freeciv maintainers team members have rights to merge commits.
- Don't break the build. All tests run by GitHub actions must pass successfully.
- The repository github.com/freeciv/freeciv-web should be generic, not specific to one particular Freeciv-web server instance or Freeciv-web fork.
- Try not to exceed 200 LOC +- in a single PR. Larger pull-requests are allowed when required.
- Avoid whitespace-only or other trivial changes that do not change logic.

The best thing you can do is to keep commits and PRs contextual. Small PRs
are great, as they're easy to review and approve. Large, multi-faceted PRs
are more difficult to review, and may get rejected for one portion,though
other portions would have been accepted if they were independent.

A PR should generally have a purpose other than "fixing code"; changing
lines just for the sake of style or syntax obscures the commit history of a
file. In other words, the why of a line of code's existence can usually be
determined by the comments in the previous commit for that line and those
around it - git blame is used to see why a change was made, understand the
context, and determine if, and how it can/should be modified. To change
code simply for the sake of whitespace, or re-arranging lines is frowned
upon, because it unnecessarily hides/buries history.
