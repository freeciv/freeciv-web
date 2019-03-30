/***********************************************************************
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "fcintl.h"

/* common */
#include "connection.h"
#include "mapimg.h"

/* server */
#include "commands.h"
#include "voting.h"

/* ai */
#include "difficulty.h"

/* Set the synopsis text to don't be translated. */
#define SYN_ORIG_(String) "*" String
/* Test if the synopsis should be translated or not. */
#define SYN_TRANS_(String) ('*' == String[0] ? String + 1 : _(String))

struct command {
  const char *name;       /* name - will be matched by unique prefix   */
  enum cmdlevel level;    /* access level required to use the command  */
  const char *synopsis;   /* one or few-line summary of usage */
  const char *short_help; /* one line (about 70 chars) description */
  const char *extra_help; /* extra help information; will be line-wrapped */
  char *(*extra_help_func)(const char *cmdname);
                          /* dynamically generated help; if non-NULL,
                           * extra_help is ignored. Must be pre-translated. */
  enum cmd_echo echo;     /* Who will be notified when used. */
  int vote_flags;         /* how to handle votes */
  int vote_percent;       /* percent required, meaning depends on flags */
};

/* Commands must match the values in enum command_id. */
static struct command commands[] = {
  {"start",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("start"),
   N_("Start the game, or restart after loading a savegame."),
   N_("This command starts the game. When starting a new game, "
      "it should be used after all human players have connected, and "
      "AI players have been created (if required), and any desired "
      "changes to initial server options have been made. "
      "After 'start', each human player will be able to "
      "choose their nation, and then the game will begin. "
      "This command is also required after loading a savegame "
      "for the game to recommence. Once the game is running this command "
      "is no longer available, since it would have no effect."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },

  {"help",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("help\n"
      "help commands\n"
      "help options\n"
      "help <command-name>\n"
      "help <option-name>"),
   N_("Show help about server commands and server options."),
   N_("With no arguments gives some introductory help. "
      "With argument \"commands\" or \"options\" gives respectively "
      "a list of all commands or all options. "
      "Otherwise the argument is taken as a command name or option name, "
      "and help is given for that command or option. For options, the help "
      "information includes the current and default values for that option. "
      "The argument may be abbreviated where unambiguous."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },

  {"list",	ALLOW_INFO,
   /* no translatable parameters */
   SYN_ORIG_("list\n"
             "list colors\n"
             "list connections\n"
             "list delegations\n"
             "list ignored users\n"
             "list map image definitions\n"
             "list players\n"
             "list scenarios\n"
             "list nationsets\n"
             "list teams\n"
             "list votes\n"),
   N_("Show a list of various things."),
   N_("Show a list of:\n"
      " - the player colors,\n"
      " - connections to the server,\n"
      " - all player delegations,\n"
      " - your ignore list,\n"
      " - the list of defined map images,\n"
      " - the list of the players in the game,\n"
      " - the available scenarios,\n"
      " - the available nation sets in this ruleset,\n"
      " - the teams of players or\n"
      " - the running votes.\n"
      "The argument may be abbreviated, and defaults to 'players' if "
      "absent."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"quit",	ALLOW_HACK,
   /* no translatable parameters */
   SYN_ORIG_("quit"),
   N_("Quit the game and shutdown the server."), NULL, NULL,
   CMD_ECHO_ALL, VCF_NONE, 0
  },
  {"cut",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cut <connection-name>"),
   N_("Cut a client's connection to server."),
   N_("Cut specified client's connection to the server, removing that client "
      "from the game. If the game has not yet started that client's player "
      "is removed from the game, otherwise there is no effect on the player. "
      "Note that this command now takes connection names, not player names."),
   NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"explain",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("explain\n"
      "explain <option-name>"),
   N_("Explain server options."),
   N_("The 'explain' command gives a subset of the functionality of 'help', "
      "and is included for backward compatibility. With no arguments it "
      "gives a list of options (like 'help options'), and with an argument "
      "it gives help for a particular option (like 'help <option-name>')."),
   NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"show",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("show\n"
      "show <option-name>\n"
      "show <option-prefix>\n"
      "show all\n"
      "show vital\n"
      "show situational\n"
      "show rare\n"
      "show changed\n"
      "show locked\n"
      "show rulesetdir"),
   N_("Show server options."),
   N_("With no arguments, shows vital server options (or available options, "
      "when used by clients). With an option name argument, show only the "
      "named option, or options with that prefix. With \"all\", it shows "
      "all options. With \"vital\", \"situational\" or \"rare\", a set of "
      "options with this level. With \"changed\", it shows only the options "
      "which have been modified, while with \"locked\" all settings locked "
      "by the ruleset will be listed. With \"ruleset\", it will show the "
      "current ruleset directory name."), NULL,
    CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"wall",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("wall <message>"),
   N_("Send message to all connections."),
   N_("For each connected client, pops up a window showing the message "
      "entered."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"connectmsg", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("connectmsg <message>"),
   N_("Set message to show to connecting players."),
   N_("Set message to send to clients when they connect.\n"
      "Empty message means that no message is sent."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"vote",	ALLOW_BASIC,
   /* TRANS: translate text between [] only; "vote" is as a process */
   N_("vote yes|no|abstain [vote number]"),
   /* TRANS: "vote" as an instance of voting */
   N_("Cast a vote."),
      /* xgettext:no-c-format */
   N_("A player with basic level access issuing a control level command "
      "starts a new vote for the command. The /vote command followed by "
      "\"yes\", \"no\", or \"abstain\", and optionally a vote number, "
      "gives your vote. If you do not add a vote number, your vote applies "
      "to the latest vote. You can only suggest one vote at a time. "
      "The vote will pass immediately if more than half of the voters "
      "who have not abstained vote for it, or fail immediately if at "
      "least half of the voters who have not abstained vote against it."),
   NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"debug",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("debug diplomacy <player>\n"
      "debug ferries\n"
      "debug tech <player>\n"
      "debug city <x> <y>\n"
      "debug units <x> <y>\n"
      "debug unit <id>\n"
      "debug timing\n"
      "debug info"),
   N_("Turn on or off AI debugging of given entity."),
   N_("Print AI debug information about given entity and turn continuous "
      "debugging output for this entity on or off."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"set",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("set <option-name> <value>"),
   N_("Set server option."),
   /* TRANS: don't translate text in '' */
   N_("Set an option on the server. The syntax and legal values depend "
      "on the option; see the help for each option. Some options are "
      "\"bitwise\", in that they consist of a choice from a set of values; "
      "separate these with |, for instance, '/set topology wrapx|iso'. For "
      "these options, use syntax like '/set topology \"\"' to set no "
      "values."), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"team",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("team <player> <team>"),
   N_("Change a player's team affiliation."),
   N_("A team is a group of players that start out allied, with shared "
      "vision and embassies, and fight together to achieve team victory "
      "with averaged individual scores. Each player is always a member "
      "of a team (possibly the only member). This command changes which "
      "team a player is a member of. Use \"\" if names contain whitespace."),
   NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"rulesetdir", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("rulesetdir <directory>"),
   N_("Choose new ruleset directory or modpack."),
   NULL, NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"metamessage", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("metamessage <meta-line>"),
   N_("Set metaserver info line."),
   N_("Set user defined metaserver info line. If parameter is omitted, "
      "previously set metamessage will be removed. For most of the time "
      "user defined metamessage will be used instead of automatically "
      "generated messages, if it is available."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"metapatches", ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("metapatches <meta-line>"),
   N_("Set metaserver patches line."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaconnection",	ALLOW_ADMIN,
   /* no translatable parameters */
   SYN_ORIG_("metaconnection u|up\n"
             "metaconnection d|down\n"
             "metaconnection ?"),
   N_("Control metaserver connection."),
   N_("'metaconnection ?' reports on the status of the connection to metaserver. "
      "'metaconnection down' or 'metac d' brings the metaserver connection down. "
      "'metaconnection up' or 'metac u' brings the metaserver connection up. "
      "'metaconnection persistent' or 'metac p' is like 'up', but keeps trying after failures. "),
   NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaserver",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("metaserver <address>"),
   N_("Set address (URL) for metaserver to report to."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"aitoggle",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("aitoggle <player-name>"),
   N_("Toggle AI status of player."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"take",    ALLOW_INFO,
   /* TRANS: translate text between [] and <> only */
   N_("take [connection-name] <player-name>"),
   N_("Take over a player's place in the game."),
   /* TRANS: Don't translate text between '' */
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to take over a player. If you're not one of these, "
      "only the <player-name> argument is allowed. If '-' is given for the "
      "player name and the connection does not already control a player, one "
      "is created and assigned to the connection. The 'allowtake' option "
      "controls which players may be taken and in what circumstances."),
   NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"observe",    ALLOW_INFO,
   /* TRANS: translate text between [] only */
   N_("observe [connection-name] [player-name]"),
   N_("Observe a player or the whole game."),
   /* TRANS: Don't translate text between '' */
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to observe a player. If you're not one of these, "
      "only the [player-name] argument is allowed. If the console gives no "
      "player-name or the connection uses no arguments, then the connection "
      "is attached to a global observer. The 'allowtake' option controls "
      "which players may be observed and in what circumstances."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"detach",    ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("detach <connection-name>"),
   N_("Detach from a player."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to detach from a player."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"create",	ALLOW_CTRL,
   /* TRANS: translate text between <> and [] only */
   N_("create <player-name> [ai type]"),
   N_("Create an AI player with a given name."),
   /* TRANS: don't translate text between single quotes */
   N_("With the 'create' command a new player with the given name is "
      "created.\n"
      "If 'player-name' is empty, a random name will be assigned when the "
      "game begins. Until then the player will be known by a name derived "
      "from its type.\n"
      "The 'ai type' parameter can be used to select which AI module will be "
      "used for the created player. This requires that the respective module "
      "has been loaded or built in to the server.\n"
      "If the game has already started, the new player will have no units or "
      "cities; also, if no free player slots are available, the slot of a "
      "dead player can be reused (removing all record of that player from the "
      "running game)."), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"away",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("away"),
   N_("Set yourself in away mode. The AI will watch your back."),
   NULL, ai_level_help,
   CMD_ECHO_NONE, VCF_NONE, 50
  },
  {"handicapped",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("handicapped\n"
      "handicapped <player-name>"),
   /* TRANS: translate 'Handicapped' as AI skill level */
   N_("Set one or all AI players to 'Handicapped'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"novice",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("novice\n"
      "novice <player-name>"),
   /* TRANS: translate 'Novice' as AI skill level */
   N_("Set one or all AI players to 'Novice'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"easy",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("easy\n"
      "easy <player-name>"),
   /* TRANS: translate 'Easy' as AI skill level */
   N_("Set one or all AI players to 'Easy'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"normal",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("normal\n"
      "normal <player-name>"),
   /* TRANS: translate 'Normal' as AI skill level */
   N_("Set one or all AI players to 'Normal'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"hard",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("hard\n"
      "hard <player-name>"),
   /* TRANS: translate 'Hard' as AI skill level */
   N_("Set one or all AI players to 'Hard'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"cheating",  ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cheating\n"
      "cheating <player-name>"),
   /* TRANS: translate 'Cheating' as AI skill level */
   N_("Set one or all AI players to 'Cheating'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
#ifdef FREECIV_DEBUG
  {"experimental",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("experimental\n"
      "experimental <player-name>"),
   /* TRANS: translate 'Experimental' as AI skill level */
   N_("Set one or all AI players to 'Experimental'."), NULL, ai_level_help,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
#endif /* FREECIV_DEBUG */
  {"cmdlevel",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("cmdlevel\n"
      "cmdlevel <level>\n"
      "cmdlevel <level> new\n"
      "cmdlevel <level> first\n"
      "cmdlevel <level> <connection-name>"),
   N_("Query or set command access level access."),
   N_("The command access level controls which server commands are available "
      "to users via the client chatline. The available levels are:\n"
      "    none  -  no commands\n"
      "    info  -  informational or observer commands only\n"
      "    basic -  commands available to players in the game\n"
      "    ctrl  -  commands that affect the game and users\n"
      "    admin -  commands that affect server operation\n"
      "    hack  -  *all* commands - dangerous!\n"
      "With no arguments, the current command access levels are reported. "
      "With a single argument, the level is set for all existing "
      "connections, and the default is set for future connections. "
      "If 'new' is specified, the level is set for newly connecting clients. "
      "If 'first come' is specified, the 'first come' level is set; it will be "
      "granted to the first client to connect, or if there are connections "
      "already, the first client to issue the 'first' command. "
      "If a connection name is specified, the level is set for that "
      "connection only.\n"
      "Command access levels do not persist if a client disconnects, "
      "because some untrusted person could reconnect with the same name. "
      "Note that this command now takes connection names, not player names."
      ), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"first", ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("first"),
   N_("If there is none, become the game organizer with increased permissions."),
   NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"timeoutincrease", ALLOW_CTRL, 
   /* TRANS: translate text between <> only */
   N_("timeoutincrease <turn> <turninc> <value> <valuemult>"), 
   N_("See \"/help timeoutincrease\"."),
   N_("Every <turn> turns, add <value> to timeout timer, then add <turninc> "
      "to <turn> and multiply <value> by <valuemult>. Use this command in "
      "concert with the option \"timeout\". Defaults are 0 0 0 1"), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"cancelvote", ALLOW_BASIC,
   /* TRANS: translate text between <> only; "vote" is as a process */
   N_("cancelvote\n"
      "cancelvote <vote number>\n"
      "cancelvote all\n"),
   /* TRANS: "vote" as a process */
   N_("Cancel a running vote."),
   /* TRANS: "vote" as a process */
   N_("With no arguments this command removes your own vote. If you have "
      "an admin access level, you can cancel any vote by vote number, or "
      "all votes with the \'all\' argument."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"ignore", ALLOW_INFO,
   /* TRANS: translate text between <> and [] only */
   N_("ignore [type=]<pattern>"),
   N_("Block all messages from users matching the pattern."),
   N_("The given pattern will be added to your ignore list; you will not "
      "receive any messages from users matching this pattern. The type "
      "may be either \"user\", \"host\", or \"ip\". The default type "
      "(if omitted) is to match against the username. The pattern supports "
      "unix glob style wildcards, i.e., * matches zero or more character, ? "
      "exactly one character, [abc] exactly one of 'a' 'b' or 'c', etc. "
      "To access your current ignore list, issue \"/list ignore\"."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"unignore", ALLOW_INFO,
   /* TRANS: translate text between <> */
   N_("unignore <range>"),
   N_("Remove ignore list entries."),
   N_("The ignore list entries in the given range will be removed; "
      "you will be able to receive messages from the respective users. "
      "The range argument may be a single number or a pair of numbers "
      "separated by a dash '-'. If the first number is omitted, it is "
      "assumed to be 1; if the last is omitted, it is assumed to be "
      "the last valid ignore list index. To access your current ignore "
      "list, issue \"/list ignore\"."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"playercolor", ALLOW_ADMIN,
   /* TRANS: translate text between <> */
   N_("playercolor <player-name> <color>\n"
      "playercolor <player-name> reset"),
   N_("Define the color of a player."),
   N_("This command sets the color of a specific player, overriding any color "
      "assigned according to the 'plrcolormode' setting.\n"
      "The color is defined using hexadecimal notation (hex) for the "
      "combination of Red, Green, and Blue color components (RGB), similarly "
      "to HTML. For each component, the lowest (darkest) value is 0 (in "
      "hex: 00), and the highest value is 255 (in hex: FF). The color "
      "definition is simply the three hex values concatenated together "
      "(RRGGBB). For example, the following command sets Caesar to pure red:\n"
      "  playercolor Caesar ff0000\n"
      "Before the game starts, this command can only be used if the "
      "'plrcolormode' setting is set to 'PLR_SET'; a player's color can be "
      "unset again by specifying 'reset'.\n"
      "Once the game has started and colors have been assigned, this command "
      "changes the player color in any mode; 'reset' cannot be used.\n"
      "To list the player colors, use 'list colors'."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"endgame",	ALLOW_ADMIN,
   /* no translatable parameters */
   SYN_ORIG_("endgame"),
   N_("End the game immediately in a draw."), NULL, NULL,
   CMD_ECHO_ALL, VCF_NONE, 0
  },
  {"surrender",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("surrender"),
   N_("Concede the game."),
   N_("This tells everyone else that you concede the game, and if all "
      "but one player (or one team) have conceded the game in this way "
      "then the game ends."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"remove",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("remove <player-name>"),
   N_("Fully remove player from game."),
   N_("This *completely* removes a player from the game, including "
      "all cities and units etc. Use with care!"), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"save",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("save\n"
      "save <file-name>"),
   N_("Save game to file."),
   N_("Save the current game to file <file-name>. If no file-name "
      "argument is given saves to \"<auto-save name prefix><year>m.sav[.gz]\". "
      "To reload a savegame created by 'save', start the server with "
      "the command-line argument:\n"
      "    '--file <filename>' or '-f <filename>'\n"
      "and use the 'start' command once players have reconnected."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"scensave",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("scensave\n"
      "scensave <file-name>"),
   N_("Save game to file as scenario."),
   N_("Save the current game to file <file-name> as scenario. If no file-name "
      "argument is given saves to \"<auto-save name prefix><year>m.sav[.gz]\". "
      "To reload a savegame created by 'scensave', start the server with "
      "the command-line argument:\n"
      "    '--file <filename>' or '-f <filename>'\n"
      "and use the 'start' command once players have reconnected."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"load",      ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("load\n"
      "load <file-name>"),
   N_("Load game from file."),
   N_("Load a game from <file-name>. Any current data including players, "
      "rulesets and server options are lost."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"read",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("read <file-name>"),
   N_("Process server commands from file."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"write",	ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("write <file-name>"),
   N_("Write current settings as server commands to file."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"reset",	ALLOW_CTRL,
   /* no translatable parameters */
   SYN_ORIG_("reset [game|ruleset|script|default]"),
   N_("Reset all server settings."),
   N_("Reset all settings if it is possible. The following levels are "
      "supported:\n"
      "  game     - using the values defined at the game start\n"
      "  ruleset  - using the values defined in the ruleset\n"
      "  script   - using default values and rereading the start script\n"
      "  default  - using default values\n"), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"default",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("default <option name>"),
   N_("Set option to its default value"),
   N_("Reset the option to its default value. If the default ever changes "
      "in a future version, the option's value will follow that change."),
      NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"lua", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("lua cmd <script line>\n"
      "lua unsafe-cmd <script line>\n"
      "lua file <script file>\n"
      "lua unsafe-file <script file>\n"
      "lua <script line> (deprecated)"),
   N_("Evaluate a line of Freeciv script or a Freeciv script file in the "
      "current game."),
   N_("The unsafe prefix runs the script in an instance separate from the "
      "ruleset. This instance doesn't restrict access to Lua functions "
      "that can be used to hack the computer running the Freeciv server. "
      "Access to it is therefore limited to the console and connections "
      "with cmdlevel 'hack'"), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"kick", ALLOW_CTRL,
   /* TRANS: translate text between <> */
    N_("kick <user>"),
    N_("Cut a connection and disallow reconnect."),
    N_("The connection given by the 'user' argument will be cut from the "
       "server and not allowed to reconnect. The time the user wouldn't be "
       "able to reconnect is controlled by the 'kicktime' setting."), NULL,
   CMD_ECHO_ADMINS, VCF_NOPASSALONE, 50
  },
  {"delegate", ALLOW_BASIC,
   /* TRANS: translate only text between [] and <> */
   N_("delegate to <username> [player-name]\n"
      "delegate cancel [player-name]\n"
      "delegate take <player-name>\n"
      "delegate restore\n"
      "delegate show <player-name>"),
   N_("Delegate control to another user."),
   N_("Delegation allows a user to nominate another user who can temporarily "
      "take over control of their player while they are away.\n"
      "'delegate to <username>': allow <username> to 'delegate take' your "
      "player.\n"
      "'delegate cancel': nominated user can no longer take your player.\n"
      "'delegate take <player-name>': take control of a player who has been "
      "delegated to you. (Behaves like 'take', except that the 'allowtake' "
      "restrictions are not enforced.)\n"
      "'delegate restore': relinquish control of a delegated player (opposite "
      "of 'delegate take') and restore your previous view, if any. (This also "
      "happens automatically if the player's owner reconnects.)\n"
      "'delegate show': show who control of your player is currently "
      "delegated to, if anyone.\n"
      "The [player-name] argument can only be used by connections with "
      "cmdlevel 'admin' or above to force the corresponding change of the "
      "delegation status."), NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"aicmd", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("aicmd <player> <command>"),
   N_("Execute AI command"),
   N_("Execute a command in the context of the AI for the given player"),
   NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"fcdb", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("fcdb reload\n"
      "fcdb lua <script>"),
   N_("Manage the authentication database."),
   N_("The argument 'reload' causes the database script file to be re-read "
      "after a change, while the argument 'lua' evaluates a line of Lua "
      "script in the context of the Lua instance for the database."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"mapimg",   ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("mapimg define <mapdef>\n"
      "mapimg show <id>|all\n"
      "mapimg create <id>|all\n"
      "mapimg delete <id>|all\n"
      "mapimg colortest"),
   N_("Create image files of the world/player map."),
   NULL, mapimg_help,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"rfcstyle",	ALLOW_HACK,
   /* no translatable parameters */
   SYN_ORIG_("rfcstyle"),
   N_("Switch server output between 'RFC-style' and normal style."), NULL, NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"serverid",	ALLOW_INFO,
   /* no translatable parameters */
   SYN_ORIG_("serverid"),
   N_("Simply returns the id of the server."), NULL, NULL,
   CMD_ECHO_NONE, VCF_NONE, 0
  }
};


/**********************************************************************//**
  Return command by its number.
**************************************************************************/
const struct command *command_by_number(int i)
{
  fc_assert_ret_val(i >= 0 && i < CMD_NUM, NULL);
  return &commands[i];
}

/**********************************************************************//**
  Return name of the command
**************************************************************************/
const char *command_name(const struct command *pcommand)
{
  return pcommand->name;
}

/**********************************************************************//**
  Return name of the command by commands number.
**************************************************************************/
const char *command_name_by_number(int i)
{
  return command_by_number(i)->name;
}

/**********************************************************************//**
  Returns the synopsis text of the command (translated).
**************************************************************************/
const char *command_synopsis(const struct command *pcommand)
{
  return SYN_TRANS_(pcommand->synopsis);
}

/**********************************************************************//**
  Returns the short help text of the command (translated).
**************************************************************************/
const char *command_short_help(const struct command *pcommand)
{
  return _(pcommand->short_help);
}

/**********************************************************************//**
  Returns the extra help text of the command (translated).
  The caller must free this string.
**************************************************************************/
char *command_extra_help(const struct command *pcommand)
{
  if (pcommand->extra_help_func) {
    fc_assert(pcommand->extra_help == NULL);
    return pcommand->extra_help_func(pcommand->name);
  } else if (pcommand->extra_help) {
    return fc_strdup(_(pcommand->extra_help));
  } else {
    return NULL;
  }
}

/**********************************************************************//**
  What is the permissions level required for running the command?
**************************************************************************/
enum cmdlevel command_level(const struct command *pcommand)
{
  return pcommand->level;
}

/**********************************************************************//**
  Returns the flag of the command to notify the users about its usage.
**************************************************************************/
enum cmd_echo command_echo(const struct command *pcommand)
{
  return pcommand->echo;
}

/**********************************************************************//**
  Returns a bit-wise combination of all vote flags set for this command.
**************************************************************************/
int command_vote_flags(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_flags : 0;
}

/**********************************************************************//**
  Returns the vote percent required for this command to pass in a vote.
**************************************************************************/
int command_vote_percent(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_percent : 0;
}
