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

-----------------------------------------------------------------------------------------------------
-- default.lua callback overrides

-- Get gold from entering a hut.
function alien_hut_get_gold(unit, gold)
  local owner = unit.owner

  notify.event(owner, unit.tile, E.HUT_GOLD, PL_("It was a safe containing %d gold.",
                                                 "It was a safe containing %d gold.", gold),
               gold)
  owner:change_gold(gold)
end

-- Get a tech from entering a hut.
function alien_hut_get_tech(unit)
  local owner = unit.owner
  local tech = owner:give_tech(nil, -1, false, "hut")

  if tech then
    notify.event(owner, unit.tile, E.HUT_TECH,
                 _("There was a datapod containing research info about %s."),
                 tech:name_translation())
    notify.research(owner, false, E.TECH_GAIN,
                 _("The %s found datapod containing research info about %s."),
                 owner.nation:plural_translation(),
                 tech:name_translation())
    notify.research_embassies(owner, E.HUT_TECH,
                 -- /* TRANS: first %s is leader or team name */
                 _("%s has acquired %s from Space Capsule they found."),
                 owner:research_name_translation(),
                 tech:name_translation())
    return true
  else
    return false
  end
end

-- Get new city from hut
function alien_hut_get_city(unit)
  local owner = unit.owner

  if unit:is_on_possible_city_tile() then
    owner:create_city(unit.tile, "")
    notify.event(owner, unit.tile, E.HUT_CITY,
                 _("You found a base starting kit."))
    return true
  else
    return false
  end
end

-- Get barbarians from hut, unless close to a city, king enters, or
-- barbarians are disabled
-- Unit may die: returns true if unit is alive
function alien_hut_get_barbarians(unit)
  local tile = unit.tile
  local type = unit.utype
  local owner = unit.owner

  if server.setting.get("barbarians") == "DISABLED"
    or unit.tile:city_exists_within_max_city_map(true)
    or type:has_flag('Gameloss') then
      notify.event(owner, unit.tile, E.HUT_BARB_CITY_NEAR,
                   _("The Space Capsule was already scavenged by someone."))
    return true
  end
  
  local alive = tile:unleash_barbarians()
  if alive then
    notify.event(owner, tile, E.HUT_BARB,
                  _("It was a trap! Set by the outcasts."));
  else
    notify.event(owner, tile, E.HUT_BARB_KILLED,
                  _("Your %s has been killed by outcasts!"),
                  type:name_translation());
  end
  return alive
end

-- Randomly choose a hut event
function alien_hut_enter_callback(unit)
  local chance = random(0, 11)
  local alive = true

  if chance == 0 then
    alien_hut_get_gold(unit, 25)
  elseif chance == 1 or chance == 2 or chance == 3 then
    alien_hut_get_gold(unit, 50)
  elseif chance == 4 then
    alien_hut_get_gold(unit, 100)
  elseif chance == 5 or chance == 6 or chance == 7 then
    alien_hut_get_tech(unit)
  elseif chance == 8 or chance == 9 then
    alien_hut_get_gold(unit, 25)
  elseif chance == 10 then
    alive = alien_hut_get_barbarians(unit)
  elseif chance == 11 then
    if not alien_hut_get_city(unit) then
      alien_hut_get_gold(unit, 50)
    end
  end

  -- do not process default.lua
  return true
end

signal.connect("hut_enter", "alien_hut_enter_callback")

-----------------------------------------------------------------------------------------------------
-- Alien ruleset specific callbacks

-- Show a pop up telling the beginning of the story when the game starts.
function turn_callback(turn, year)
  if turn == 1 then
    notify.event(nil, nil, E.SCRIPT,
_("Deneb 7 was known to have strange force field surrounding it\nthat made it impossible to get near planet with year\n250 Galactic Era Earth technology. However, when you were\nflying past, that field suddenly reverted and sucked you\nto the planet.\n\nYou find yourself in a strange world, probably touched\nby superior technology, where big portion of Earth science\nis invalid. You have to learn new rules,\nrules of this world.\n\nThere's deadly radiation that no known shielding works against.\nThere's alien life, but more surprisingly also some\nedible plants just like on Earth.\n\nRadio doesn't work,\nair doesn't allow flying, some basic Physics does\nnot apply here.\n\nYou struggle to live on this planet, and read\nRoadside Picnic by Strugatsky brothers once more."))
  end
end

signal.connect('turn_begin', 'turn_callback')
