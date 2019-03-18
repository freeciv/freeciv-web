Docker volume plugin for referencing rulesets by git revision
--------------------------------------------------------------


I propose to make ruleset management easier on freeciv-web. Right now, rulesets are manually managed and versioned, and the only way to handle multiple version of the same ruleset is to make copies, like for each longturn game running. The proposed solution is to be able to specify rulesetdir like /rulesetdir civ2@git-revision, and this 9p file server and docker volume plugin enable this use case.

This will make it possible for users to make pull requests to github repo and then play their rulesets on public servers without any work other than an owner or bot accepting a pull request.
