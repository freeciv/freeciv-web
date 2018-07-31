Migration scripts
-----------------

When changes in Freeciv-web may make current installations stop working, a
migration script should be provided to help users find and understand the
changeset, and hopefully migrate their installation with no issues if it's
a default setup.

The filenames start with a sequential four-digit number. When `migrate.sh`
is executed it runs each of them in order, following from where it stopped
in its last execution.

The scripts may still fail on purpose if manual action is required (or safer
than trying to script it).
