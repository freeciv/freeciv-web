# Freeciv Derived Sources

This directory is for files derived from the `freeciv` project. It will
be empty on initial checkout. Running the `freeciv-web` project install
scripts will generate files here during the "Synchronizing Freeciv"
process.

Files generated here should be restricted to resources specifically
derived from Freeciv, e.g. network protocol definitions, images/tilesets
that are copied, etc.

Such files are generated here, instead of within `target` dir, so that
cleaning the maven project does not delete them - as the files are
generated outside of maven, maven should not delete them during its
standard `clean` phase.

**ONLY** This README.md and a .gitignore file should ever be committed
under this directory.

To regenerate files in this directory, run the script:
`${FREECIV_WEB}\scripts\sync-js-hand.sh`.
