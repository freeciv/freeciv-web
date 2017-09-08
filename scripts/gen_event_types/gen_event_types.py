#! /usr/bin/env python3

'''**********************************************************************
    Copyright (C) 2017  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************'''

import re

root = '../freeciv/freeciv/common/'
in_comment = False

section_names = []
section_descriptions = []
events = {}

# Naive regexes to match what we want, we are not writing a C interpreter
start_re_s = r'^(?:.*\{)?[\s,]*'
end_re_s = r'[\s,};]*$'
section_name_re = re.compile(start_re_s + r'(E_S_[A-Z0-9_]+)' + end_re_s)
section_description_re = re.compile(start_re_s + r'N_\("([^:"]*)[^"]*"\)' + end_re_s)
empty_description_re = re.compile(start_re_s + r'NULL' + end_re_s)
event_number_re = re.compile(r'^#define\s+SPECENUM_VALUE([0-9]+)\s+(E_[A-Z0-9_]+)\s*$')
event_data_re = re.compile(start_re_s + r'GEN_EV\(\s*' +
                           r'(E_[A-Z0-9_]+)\s*,\s*' +
                           r'(E_S_[A-Z0-9_]+)\s*,\s*' +
                           r'N_\("([^"]*)"\)\)' + end_re_s)

# Fooled by having comment delimiters inside strings
def remove_comments(line):
    global in_comment
    if in_comment:
        found = line.find("*/", 0)
        if found == -1:
            line = ""
        else:
            in_comment = False
            line = line[found + 2:]
    else:
        found = line.find("/*", 0)
        if found != -1:
            end = line.find("*/", found + 2)
            if end != -1:
                line = line[:found] + line[end + 2:]
            else:
                in_comment = True
                return line[:found]

    if found == -1:
        return line
    return remove_comments(line)


with open(root + 'events.h', 'r') as f:
    for line in f:
        line = remove_comments(line)
        match = event_number_re.match(line)
        if match:
            events[match.group(2)] = {'index': int(match.group(1)),
                                      'name': match.group(2)}

with open(root + 'events.c', 'r') as f:
    for line in f:
        line = remove_comments(line)
        match = section_name_re.match(line)
        if match:
            section_names.append(match.group(1))
            continue
        match = section_description_re.match(line)
        if match:
            section_descriptions.append(match.group(1))
            continue
        if empty_description_re.match(line):
            section_descriptions.append("Misc")
            continue
        match = event_data_re.match(line)
        if match:
            events[match.group(1)].update(section=match.group(2), description=match.group(3))

# We are going to use some synthetic events for chat, so let's first check
# that we can.
if ('E_S_CHAT' in section_names
 or 'E_CHAT_PRIVATE' in events
 or 'E_CHAT_ALLIES' in events
 or 'E_CHAT_OBSERVER' in events
 or 'E_UNDEFINED' in events
 or 'E_CHAT_ERROR' not in events
 or 'E_CHAT_MSG' not in events
 or events['E_CHAT_ERROR']['section'] != 'E_S_XYZZY'
 or events['E_CHAT_MSG']['section'] != 'E_S_XYZZY'):
    raise Exception('The handling of chat messages has to be reworked')

events['E_CHAT_ERROR']['section'] = 'E_S_CHAT'
events['E_CHAT_MSG']['section'] = 'E_S_CHAT'
section_names.append('E_S_CHAT')
section_descriptions.append('Chat')

events = sorted(events.values(), key=lambda e: e['index'])
for i, v in enumerate(events):
    while i < v['index']:
        events.insert(i, {
            'index': i,
            'name': 'E_UNDEFINED',
            'section': 'E_S_XYZZY',
            'description': 'Unknown event'})
        i = i + 1

events.append({
    'index': ++i,
    'name': 'E_CHAT_PRIVATE',
    'section': 'E_S_CHAT',
    'description': 'Private messages'})
events.append({
    'index': ++i,
    'name': 'E_CHAT_ALLIES',
    'section': 'E_S_CHAT',
    'description': 'Allies messages'})
events.append({
    'index': ++i,
    'name': 'E_CHAT_OBSERVER',
    'section': 'E_S_CHAT',
    'description': 'Observers messages'})
events.append({
    'index': ++i,
    'name': 'E_UNDEFINED',
    'section': 'E_S_XYZZY',
    'description': 'Unknown event'})

with open('fc_events.js', 'w') as f:
    print('''/****************************************
 * THIS IS A GENERATED FILE, DO NOT EDIT
 *
 * From common/events.{h,c}
 * By gen_event_types.py
 ****************************************/
''', file=f)
    print(*('var {0} = {1};'.format(v, i) for i, v in enumerate(section_names)),
          file=f, sep='\n')
    print('\nvar fc_e_section_names = [\n  ', file=f, end='')
    print(*("'" + s.lower() + "'" for s in section_names),
          file=f, sep=',\n  ', end='\n];\n')
    print('\nvar fc_e_section_descriptions = [\n  ', file=f, end='')
    print(*('"' + s + '"' for s in section_descriptions),
          file=f, sep=',\n  ', end='\n];\n')

    print(file=f)
    print(*('var {0} = {1};'.format(v['name'], i) for i, v in enumerate(events)),
          file=f, sep='\n')
    print('\nvar fc_e_events = [\n  ', file=f, end='')
    print(*('["{0}", {1}, "{2}"]'
            .format(e['name'].lower(), e['section'], e['description'])
            for e in events),
          file=f, sep=',\n  ', end='\n];\n')
    print('''
var E_I_NAME = 0;
var E_I_SECTION = 1;
var E_I_DESCRIPTION = 2;
''', file=f)
