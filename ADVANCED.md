# About this file

This file contains various advanced hints and documentation for developers.
A section may assume that its readers already has some familiarity with its
subject.
Don't expect to be able to understand everything here even after reading the
[friendlier documentation](README.md).

# Freeciv-web and Freeciv

Freeciv-web uses a [patched](freeciv/apply_patches.sh) version of Freeciv's
[trunk branch](http://freeciv.wikia.com/wiki/Freeciv_source_code_repository).
This makes it easier to get Freeciv-web related changes into Freeciv.
It also makes it easier for Freeciv-web to take advantage of the newest
Freeciv features.

Graphics, static help texts, rulesets and scenarios are extracted from
Freeciv.
Parts of The Freeciv-web client's Freeciv protocol support is auto generated
from Freeciv's [common/networking/packets.def](http://svn.gna.org/viewcvs/freeciv/trunk/common/networking/packets.def).

The Freeciv server is responsible for running the game.
It makes sure that the rules are followed.
It handles the player's orders.
It "rolls the dice" when randomness is involved.
It keeps track of the game state and what the player is allowed to see.

The Freeciv server has other responsibilities too.
It creates, starts and manages the AI players.
It is responsible for the beginning of saving and the end of loading a game.

# The AI

Freeciv-web uses the Freeciv AI.
Most of it lives in the [ai](http://repo.or.cz/freeciv.git/tree/HEAD:/ai)
folder of Freeciv.
More information about the Freeciv AI can be found at the
[Freeciv wiki](http://freeciv.wikia.com/wiki/Category:AI).
Freeciv has AI related
[introduction tasks](http://freeciv.wikia.com/wiki/Introduction_tasks).

# Upgrading Freeciv-web's Freeciv version

Freeciv-web's Freeciv version is set in
[freeciv/version.txt](freeciv/version.txt).
The variable FCREV is set to the SVN revision it should use.
The comment above it should contain the previous SVN revision.
The latest version of Freeciv's trunk branch can be found in the
[Freeciv SVN repository](svn://svn.gna.org/svn/freeciv/trunk).

## Freeciv version upgrade checklist

It is easy to break Freeciv-web by changing its Freeciv version.
The difference between the two versions should therefore be carefully
checked.

### Summary

The new Freeciv version could have new bugs.

Freeciv-web [patches](freeciv/apply_patches.sh) may have been accepted.

Be careful when Freeciv changes
`bootstrap/freeciv.project configure.ac m4/ data/ common/networking/dataio_json.* common/networking/packets.def fc_version server/save*`

### Check: Freeciv commit log messages

Was a [patch](freeciv/patches/) accepted into the new Freeciv version?
Remove the patch file.
Update [apply_patches.sh](freeciv/apply_patches.sh).

### Check: [Freeciv's bug tracker](https://gna.org/bugs/?group=freeciv)

Was (known) bugs introduced?
Have a look at fixed bugs too if you upgrade to a version that isn't the
most recent.

### Check: bootstrap/freeciv.project

Does the [project definition](freeciv/freeciv-web.project) need an update?

### Check: configure.ac m4/

Should the arguments [prepare_freeciv.sh](freeciv/prepare_freeciv.sh) uses
when it calls autogen.sh be updated?

### Check: data/helpdata.txt

Did [helpdata_gen.py](scripts/helpdata_gen/helpdata_gen.py) successfully
extract the new version?

### Check: data/scenarios/

Has the bundled scenarios changed?
Does the change conflict with Freeciv-web's modifications?

### Check: data/civ2civ3

Was the ruleset format changed?
Did the civciv3 rules change?
Is Freeciv-web able to handle the modifications?
Be aware that the client may have hard coded the old rule.
(The classic ruleset and the webperimental ruleset are supposed to be
web-compatible)

### Check: data/

Was the tileset format changed?
Does [img-extract.py](scripts/freeciv-img-extract/img-extract.py) need an
update?

### Check: data/

Was any tag removed from a tileset?
Does Freeciv-web use it?

### Check: server/save*

Has there been changes to save game handling?
Is there development version save game compatibility for format changes?

### Check: common/networking/packets.def

Did a packet change?
Does Freeciv-web send it?
Should the sender change?

Does the server send it?
Is it handled in [packhand.js](freeciv-web/src/main/webapp/javascript/packhand.js)?
Who uses the packet after it is handled?

### Check: common/dataio_json.*

Does this change a field in a packet Freeciv-web sends or receives?
Find its Freeciv-web users.

### Check: fc_version

Has an enum gained or lost a value?
Update the corresponding variables in Freeciv-web if they exist.

Is there a change to the meaning of a packet?
Find its Freeciv-web users.
