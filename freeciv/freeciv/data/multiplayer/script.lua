-- Freeciv - Copyright (C) 2007 - The Freeciv Project
--   This program is free software; you can redistribute it and/or modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation; either version 2, or (at your option)
--   any later version.
--
--   This program is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.

-- This file is for lua-functionality that is specific to a given
-- ruleset. When freeciv loads a ruleset, it also loads script
-- file called 'default.lua'. The one loaded if your ruleset
-- does not provide an override is default/default.lua.


-- Place Ruins at the location of the destroyed city.
function city_destroyed_callback(city, loser, destroyer)
  city.tile:create_extra("Ruins", NIL)
  -- continue processing
  return false
end

signal.connect("city_destroyed", "city_destroyed_callback")


-- Grant tech when the wonder Darwin`s Voyage is built.
function building_built_handler(btype, city)
  local darw_btype = find.building_type("Darwin's Voyage")
  local darw_id = darw_btype.id
  local player, id = city.owner, btype.id
  local gained

  if id == darw_id then
    -- Block the player from destroying the wonder, rebuilding it and
    -- getting free advance again.
    -- This also prevents those they share research with from getting
    -- free advance from building Darwin`s Voyage themselves.
    if player:give_tech(find.tech_type("Theory of Evolution"),
                        0, false, "researched") then
      -- Give the player free advance.
      gained = player:give_tech(nil, 0, false, "researched")

      -- Notify the player. Include the tech name in a way that makes it
      -- look natural no matter if tech is announced or not.
      notify.event(player, NIL, E.TECH_GAIN,
        _("%s boosts research; you gain the immediate advance %s."),
        darw_btype:name_translation(),
        gained:name_translation())

      notify.research(player, false, E.TECH_GAIN,
        _("%s boosts %s research; you gain the immediate advance %s."),
        darw_btype:name_translation(),
        player.nation:name_translation(),
        gained:name_translation())

      -- default.lua informs the embassies when the tech source is a hut.
      -- They should therefore be informed about the source here too.
      notify.research_embassies(player, E.TECH_EMBASSY,
                                -- /* TRANS: 1st %s is leader or team name */
                                _("%s gains %s from %s."),
                                player:research_name_translation(),
                                gained:name_translation(),
                                darw_btype:name_translation())
    end
  end
end

signal.connect("building_built", "building_built_handler")


-- Hack: record which players already got Philosophy, to avoid
-- teams getting it multiple times with team_pooled_research.
-- Stored as a string as this is a type simple enough to be included
-- in savefiles.
-- (It`s probably not necessary to test for existence as savefile
-- data is loaded after this script is executed.)
if philo_players == nil then
  philo_players = ""
end

-- Record that a player got Philosophy in our hacky string.
function record_philo(player)
  local pos = player.id + 1
  philo_players = string.sub(philo_players, 1, pos-1) ..
                  string.rep(" ", math.max(0, pos - 1 - #philo_players)) ..
                  "." .. string.sub(philo_players, pos+1)
end

-- Grant one tech when the tech Philosophy is researched.
function tech_researched_handler(tech, player, how)
  local id
  local gained

  if tech == nil then
    -- no tech was researched.
    return
  end

  id = tech.id
  if id == find.tech_type("Philosophy").id and how == "researched" then

    -- Check potential teammates.
    for p in players_iterate() do
      if player:shares_research(p)
         and string.sub(philo_players, p.id+1, p.id+1) == "." then
        -- Another player in the same team already got Philosophy.
        record_philo(player)
        return
      end
    end

    record_philo(player)

    -- Give the player a free advance.
    -- This will give a free advance for each player that shares research.
    gained = player:give_tech(nil, -1, false, "researched")

      -- Notify the player. Include the tech names in a way that makes it
      -- look natural no matter if each tech is announced or not.
    notify.event(player, NIL, E.TECH_GAIN,
                 _("Great philosophers from all the world join your civilization: you get the immediate advance %s."),
                 gained:name_translation())

    -- Notify research partners
    notify.research(player, false, E.TECH_GAIN,
                    _("Great philosophers from all the world join the %s: you get the immediate advance %s."),
                    player.nation:plural_translation(),
                    gained:name_translation())

    -- default.lua informs the embassies when the tech source is a hut.
    -- They should therefore be informed about the source here too.
    notify.research_embassies(player, E.TECH_EMBASSY,
            -- /* TRANS: first %s is leader or team name */
            _("Great philosophers from all the world join %s: they get %s as an immediate advance."),
            player:research_name_translation(),
            gained:name_translation())
  end
end 

signal.connect("tech_researched", "tech_researched_handler")
