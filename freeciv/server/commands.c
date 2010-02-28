/********************************************************************** 
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
#include <config.h>
#endif

#include "fcintl.h"

#include "connection.h"

#include "commands.h"
#include "voting.h"

struct command {
  const char *name;       /* name - will be matched by unique prefix   */
  enum cmdlevel_id level; /* access level required to use the command  */
  const char *synopsis;	  /* one or few-line summary of usage */
  const char *short_help; /* one line (about 70 chars) description */
  const char *extra_help; /* extra help information; will be line-wrapped */
  int vote_flags;         /* how to handle votes */
  int vote_percent;       /* percent required, meaning depends on flags */
};

/* Commands must match the values in enum command_id. */
static struct command commands[] = {
  {"start",	ALLOW_BASIC,
   /* no translatable parameters */
   "start",
   N_("Start the game, or restart after loading a savegame."),
   N_("This command starts the game.  When starting a new game, "
      "it should be used after all human players have connected, and "
      "AI players have been created (if required), and any desired "
      "changes to initial server options have been made.  "
      "After 'start', each human player will be able to "
      "choose their nation, and then the game will begin.  "
      "This command is also required after loading a savegame "
      "for the game to recommence.  Once the game is running this command "
      "is no longer available, since it would have no effect."),
   VCF_NONE, 0
  },

  {"help",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("help\n"
      "help commands\n"
      "help options\n"
      "help <command-name>\n"
      "help <option-name>"),
   N_("Show help about server commands and server options."),
   N_("With no arguments gives some introductory help.  "
      "With argument \"commands\" or \"options\" gives respectively "
      "a list of all commands or all options.  "
      "Otherwise the argument is taken as a command name or option name, "
      "and help is given for that command or option.  For options, the help "
      "information includes the current and default values for that option.  "
      "The argument may be abbreviated where unambiguous."),
   VCF_NONE, 0
  },

  {"list",	ALLOW_INFO,
   /* no translatable parameters */
   "list\n"
   "list players\n"
   "list teams\n"
   "list connections\n"
   "list scenarios",
   N_("Show a list of players, teams, connections, or scenarios."),
   N_("Show a list of players in the game, teams of players, connections to "
      "the server, or available scenarios.  The argument may be abbreviated,"
      " and defaults to 'players' if absent."),
   VCF_NONE, 0
  },
  {"quit",	ALLOW_HACK,
   /* no translatable parameters */
   "quit",
   N_("Quit the game and shutdown the server."), NULL,
   VCF_NONE, 0
  },
  {"cut",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cut <connection-name>"),
   N_("Cut a client's connection to server."),
   N_("Cut specified client's connection to the server, removing that client "
      "from the game.  If the game has not yet started that client's player "
      "is removed from the game, otherwise there is no effect on the player.  "
      "Note that this command now takes connection names, not player names."),
   VCF_NONE, 50
  },
  {"explain",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("explain\n"
      "explain <option-name>"),
   N_("Explain server options."),
   N_("The 'explain' command gives a subset of the functionality of 'help', "
      "and is included for backward compatibility.  With no arguments it "
      "gives a list of options (like 'help options'), and with an argument "
      "it gives help for a particular option (like 'help <option-name>')."),
   VCF_NONE, 0
  },
  {"show",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("show\n"
      "show <option-name>\n"
      "show <option-prefix>"),
   N_("Show server options."),
   N_("With no arguments, shows all server options (or available options, when "
      "used by clients).  With an argument, show only the named option, "
      "or options with that prefix.")
  },
  {"wall",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("wall <message>"),
   N_("Send message to all connections."),
   N_("For each connected client, pops up a window showing the message "
      "entered."),
   VCF_NONE, 0
  },
  {"connectmsg", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("connectmsg <message>"),
   N_("Set message to show to connecting players."),
   N_("Set message to sends to clients when they connect.\n"
      "Empty message means that no message is sent.")
  },
  {"vote",	ALLOW_BASIC,
   /* TRANS: translate text between [] only */
   N_("vote yes|no|abstain [vote number]"),
   N_("Cast a vote."),
      /* xgettext:no-c-format */
   N_("A player with info level access issuing a control level command "
      "starts a new vote for the command.  The /vote command followed by "
      "\"yes\", \"no\", or \"abstain\", and optionally a vote number, "
      "gives your vote.  If you do not add a vote number, your vote applies "
      "to the latest vote.  You can only suggest one vote at a time.  "
      "The vote will pass immediately if more than half of the voters "
      "who have not abstained vote for it, or fail immediately if at "
      "least half of the voters who have not abstained vote against it."),
   VCF_NONE, 0
  },
  {"debug",	ALLOW_CTRL,
   /* no translatable parameters */
   "debug [ diplomacy | ferries | player <player> | tech <player>"
   " | city <x> <y> | units <x> <y> | unit <id> "
   " | timing | info ]",
   N_("Turn on or off AI debugging of given entity."),
   N_("Print AI debug information about given entity and turn continous "
      "debugging output for this entity on or off."),
   VCF_NONE, 0
  },
  {"set",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("set <option-name> <value>"),
   N_("Set server option."), NULL,
   VCF_NONE, 50
  },
  {"team",	ALLOW_CTRL,
   /* TRANS: translate text between <> and [] only */
   N_("team <player> [team]"),
   N_("Change, add or remove a player's team affiliation."),
   N_("Sets a player as member of a team. If no team specified, the "
      "player is set teamless. Use \"\" if names contain whitespace. "
      "A team is a group of players that start out allied, with shared "
      "vision and embassies, and fight together to achieve team victory "
      "with averaged individual scores."),
   VCF_NONE, 50
  },
  {"rulesetdir", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("rulesetdir <directory>"),
   N_("Choose new ruleset directory or modpack."),
   N_("Choose new ruleset directory or modpack. Calling this\n "
      "without any arguments will show you the currently selected "
      "ruleset."),
   VCF_NONE, 50
  },
  {"metamessage", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("metainfo <meta-line>"),
   N_("Set metaserver info line."),
   N_("Set user defined metaserver info line. If parameter is omitted,\n"
      "previously set metamessage will be removed. For most of the time\n"
      "user defined metamessage will be used instead of automatically\n"
      "generated messages, if it is available."),
   VCF_NONE, 50
  },
  {"metapatches", ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("metapatch <meta-line>"),
   N_("Set metaserver patches line."), NULL,
   VCF_NONE, 0
  },
  {"metaconnection",	ALLOW_ADMIN,
   /* no translatable parameters */
   "metaconnection u|up\n"
   "metaconnection d|down\n"
   "metaconnection ?",
   N_("Control metaserver connection."),
   N_("'metaconnection ?' reports on the status of the connection to metaserver.\n"
      "'metaconnection down' or 'metac d' brings the metaserver connection down.\n"
      "'metaconnection up' or 'metac u' brings the metaserver connection up."),
   VCF_NONE, 0
  },
  {"metaserver",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("metaserver <address>"),
   N_("Set address (URL) for metaserver to report to."), NULL,
   VCF_NONE, 0
  },
  {"aitoggle",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("aitoggle <player-name>"),
   N_("Toggle AI status of player."), NULL,
   VCF_NONE, 50
  },
  {"take",    ALLOW_INFO,
   /* TRANS: translate text between [] and <> only */
   N_("take [connection-name] <player-name>"),
   N_("Take over a player's place in the game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to take over a player. If you're not one of these, "
      "only the <player-name> argument is allowed.  If '-' is given for the "
      "player name and the connection does not already control a player, one "
      "is created and assigned to the connection."),
   VCF_NONE, 0
  },
  {"observe",    ALLOW_INFO,
   /* TRANS: translate text between [] only */
   N_("observe [connection-name] [player-name]"),
   N_("Observe a player or the whole game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to observe a player. If you're not one of these, "
      "only the [player-name] argument is allowed. If the console gives no "
      "player-name or the connection uses no arguments, then the connection "
      "is attached to a global observer."),
   VCF_NONE, 0
  },
  {"detach",    ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("detach <connection-name>"),
   N_("detach from a player."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to detach from a player."),
   VCF_NONE, 0
  },
  {"create",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("create <player-name>"),
   N_("Create an AI player with a given name."),
   N_("The 'create' command is only available before the game has "
      "been started."),
   VCF_NONE, 50
  },
  {"away",	ALLOW_BASIC,
   /* no translatable parameters */
   "away",
   N_("Set yourself in away mode. The AI will watch your back."),
   N_("The AI will govern your nation but do minimal changes."),
   VCF_NONE, 50
  },
  {"novice",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("novice\n"
      "novice <player-name>"),
   N_("Set one or all AI players to 'novice'."),
   N_("With no arguments, sets all AI players to skill level 'novice', and "
      "sets the default level for any new AI players to 'novice'.  With an "
      "argument, sets the skill level for that player only."),
   VCF_NONE, 50
  },
  {"easy",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("easy\n"
      "easy <player-name>"),
   N_("Set one or all AI players to 'easy'."),
   N_("With no arguments, sets all AI players to skill level 'easy', and "
      "sets the default level for any new AI players to 'easy'.  With an "
      "argument, sets the skill level for that player only."),
   VCF_NONE, 50
  },
  {"normal",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("normal\n"
      "normal <player-name>"),
   N_("Set one or all AI players to 'normal'."),
   N_("With no arguments, sets all AI players to skill level 'normal', and "
      "sets the default level for any new AI players to 'normal'.  With an "
      "argument, sets the skill level for that player only."),
   VCF_NONE, 50
  },
  {"hard",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("hard\n"
      "hard <player-name>"),
   N_("Set one or all AI players to 'hard'."),
   N_("With no arguments, sets all AI players to skill level 'hard', and "
      "sets the default level for any new AI players to 'hard'.  With an "
      "argument, sets the skill level for that player only."),
   VCF_NONE, 50
  },
  {"cheating",  ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cheating\n"
      "cheating <player-name>"),
   N_("Set one or all AI players to 'cheating'."),
   N_("With no arguments, sets all AI players to skill level 'cheating', and "
      "sets the default level for any new AI players to 'cheating'.  With an "
      "argument, sets the skill level for that player only."),
   VCF_NONE, 50
  },
  {"experimental",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("experimental\n"
      "experimental <player-name>"),
   N_("Set one or all AI players to 'experimental'."),
   N_("With no arguments, sets all AI players to skill 'experimental', and "
      "sets the default level for any new AI players to this.  With an "
      "argument, sets the skill level for that player only. THIS IS ONLY "
      "FOR TESTING OF NEW AI FEATURES! For ordinary servers, this option "
      "has no effect."),
   VCF_NONE, 50
  },
  {"cmdlevel",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("cmdlevel\n"
      "cmdlevel <level>\n"
      "cmdlevel <level> new\n"
      "cmdlevel <level> first\n"
      "cmdlevel <level> <connection-name>"),
   N_("Query or set command access level access."),
   N_("The command access level controls which server commands are available\n"
      "to users via the client chatline.  The available levels are:\n"
      "    none  -  no commands\n"
      "    info  -  informational or observer commands only\n"
      "    basic -  commands available to players in the game\n"
      "    ctrl  -  commands that affect the game and users\n"
      "    admin -  commands that affect server operation\n"
      "    hack  -  *all* commands - dangerous!\n"
      "With no arguments, the current command access levels are reported.\n"
      "With a single argument, the level is set for all existing "
      "connections,\nand the default is set for future connections.\n"
      "If 'new' is specified, the level is set for newly connecting clients.\n"
      "If 'first come' is specified, the 'first come' level is set; it will be\n"
      "granted to the first client to connect, or if there are connections\n"
      "already, the first client to issue the 'first' command.\n"
      "If a connection name is specified, the level is set for that "
      "connection only.\n"
      "Command access levels do not persist if a client disconnects, "
      "because some untrusted person could reconnect with the same name.  "
      "Note that this command now takes connection names, not player names."
      )
  },
  {"first", ALLOW_BASIC,
   /* no translatable parameters */
   "first",
   N_("If there is none, become the game organizer with increased permissions."),
   NULL,
  },
  {"timeoutincrease", ALLOW_CTRL, 
   /* TRANS: translate text between <> only */
   N_("timeoutincrease <turn> <turninc> <value> <valuemult>"), 
   N_("See \"help timeoutincrease\"."),
   N_("Every <turn> turns, add <value> to timeout timer, then add <turninc> "
      "to <turn> and multiply <value> by <valuemult>.  Use this command in "
      "concert with the option \"timeout\". Defaults are 0 0 0 1"),
   VCF_NONE, 50
  },
  {"cancelvote", ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("cancelvote\n"
      "cancelvote <vote number>\n"
      "cancelvote all\n"),
   N_("Cancel a running vote.\n"),
   N_("With no arguments this command removes your own vote.  If you have "
      "an admin access level, you can cancel any vote by vote number, or "
      "all votes with the \'all\' argument."),
    VCF_NONE, 0
  },
  {"endgame",	ALLOW_ADMIN,
   /* no translatable parameters */
   "endgame",
   N_("End the game immediately in a draw."), NULL,
   VCF_NONE, 0
  },
  {"surrender",	ALLOW_BASIC,
   /* no translatable parameters */
   "surrender",
   N_("Concede the game."),
   N_("This tells everyone else that you concede the game, and if all "
      "but one player (or one team) have conceded the game in this way "
      "then the game ends."),
   VCF_NONE, 0
  },
  {"remove",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("remove <player-name>"),
   N_("Fully remove player from game."),
   N_("This *completely* removes a player from the game, including "
      "all cities and units etc.  Use with care!"),
   VCF_NONE, 50
  },
  {"save",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("save\n"
      "save <file-name>"),
   N_("Save game to file."),
   N_("Save the current game to file <file-name>.  If no file-name "
      "argument is given saves to \"<auto-save name prefix><year>m.sav[.gz]\".\n"
      "To reload a savegame created by 'save', start the server with "
      "the command-line argument:\n"
      "    --file <filename>\n"
      "and use the 'start' command once players have reconnected."),
   VCF_NONE, 0
  },
  {"load",      ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("load\n"
      "load <file-name>"),
   N_("Load game from file."),
   N_("Load a game from <file-name>. Any current data including players, "
      "rulesets and server options are lost.\n"),
   VCF_NONE, 50
  },
  {"read",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("read <file-name>"),
   N_("Process server commands from file."), NULL,
   VCF_NONE, 50
  },
  {"write",	ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("write <file-name>"),
   N_("Write current settings as server commands to file."), NULL,
   VCF_NONE, 0
  },
  {"reset",	ALLOW_CTRL,
   N_("reset"),
   N_("Reset all server settings."),
   N_("Reset all settings and re-read the server start script, "
      "if there was one given with the --read server argument. "),
   VCF_NONE, 50
  },
  {"rfcstyle",	ALLOW_HACK,
   /* no translatable parameters */
   "rfcstyle",
   N_("Switch server output between 'RFC-style' and normal style."), NULL,
   VCF_NONE, 0
  },
  {"serverid",	ALLOW_INFO,
   /* no translatable parameters */
   "serverid",
   N_("Simply returns the id of the server."),
   VCF_NONE, 0
  }
};


/**************************************************************************
  ...
**************************************************************************/
const struct command *command_by_number(int i)
{
  assert(i >= 0 && i < CMD_NUM);
  return &commands[i];
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_name(const struct command *pcommand)
{
  return pcommand->name;
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_name_by_number(int i)
{
  return command_by_number(i)->name;
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_synopsis(const struct command *pcommand)
{
  return pcommand->synopsis;
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_short_help(const struct command *pcommand)
{
  return pcommand->short_help;
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_extra_help(const struct command *pcommand)
{
  return pcommand->extra_help;
}

/**************************************************************************
  ...
**************************************************************************/
enum cmdlevel_id command_level(const struct command *pcommand)
{
  return pcommand->level;
}

/**************************************************************************
  Returns a bit-wise combination of all vote flags set for this command.
**************************************************************************/
int command_vote_flags(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_flags : 0;
}

/**************************************************************************
  Returns the vote percent required for this command to pass in a vote.
**************************************************************************/
int command_vote_percent(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_percent : 0;
}
