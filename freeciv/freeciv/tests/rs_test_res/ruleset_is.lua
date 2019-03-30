-- ruleset_is.lua
-- Makes Freeciv exit with error if the environment variable EXPECTED_RULESET
-- isn't set to the current ruleset directory. Exits normally if it is.
-- Example of what can be done in an unsafe Freeciv Lua environment.
--     /lua unsafe-file tests/ruleset_is.lua

rsdir_expected = os.getenv("EXPECTED_RULESET")
rsdir_actual = game.rulesetdir()

if (rsdir_expected == nil) then
  log.fatal("Environmental variable EXPECTED_RULESET not set.")
  os.exit(false)
end

if (rsdir_actual == rsdir_expected) then
  log.normal("rulesetdir is %s, as expected.", rsdir_actual)
  os.exit(true)
else
  log.normal("rulesetdir is %s but %s was expected.",
             rsdir_actual, rsdir_expected)
  os.exit(false)
end
