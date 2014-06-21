#! /usr/bin/env python

import wikipedia
import json
from subprocess import call

freeciv_wiki_doc = {};

def fix_tech(tech_name):
  if (tech_name == "Advanced Flight"): tech_name = "Aeronautics";
  if (tech_name == "Rocketry"): tech_name = "Rocket";
  if (tech_name == "Stealth"): tech_name = "Stealth technology";
  if (tech_name == "Tactics"): tech_name = "Military tactics";
  if (tech_name == "The Republic"): tech_name = "Repubic";
  if (tech_name == "The Corporation"): tech_name = "Corporation";
  if (tech_name == "The Wheel"): tech_name = "Wheel";
  return tech_name;

def download_wiki_page(tech_name):
  print(tech_name);
  page = wikipedia.page(fix_tech(tech_name), auto_suggest=True, redirect=True);
  freeciv_wiki_doc[tech_name] = {"title" : page.title, "summary" : page.summary};



f = open('../freeciv/data/fcweb/techs.ruleset')
lines = f.readlines()
f.close()

techs = []
for line in lines:
  if line.startswith("name"):
    tech_line = line.split("\"");
    techs.append(tech_line[1]);


for tech in techs:
  download_wiki_page(tech);

f = open('../freeciv-web/src/main/webapp/javascript/freeciv-wiki-doc.js' ,'w');
f.write("var freeciv_wiki_docs = " + json.dumps(freeciv_wiki_doc));
f.close();
print("Downloading tech summaries from Wikipedia complete!");
