# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''


packet_sizes = {"MAX_NUM_TECH_LIST" : 10,
                "MAX_VET_LEVELS" : 10,
                 "O_LAST" : 6,
		 "MAX_NUM_UNIT_LIST" : 10,
		 "MAX_NUM_BUILDING_LIST" : 10,
		 "A_LAST" : 4,
         "A_LAST+1" : 88,
		 "B_LAST" : 200,
		 "MAX_GRANARY_INIS" : 24,
		 "MAX_NUM_PLAYERS" : 30,
		 "MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS" : 32,
		 "MAX_NUM_BARBARIANS" : 2,
		 "NUM_SS_STRUCTURALS+1" : 33,
		 "NUM_SS_STRUCTURALS" : 32,
		 "S_LAST" : 8,
		 "MAX_LEN_NAME" : 32,
		 "MAX_NUM_LEADERS" : 200,
		 "MAX_NUM_ITEMS" : 200,
         "bv_imprs" : 175,          #B_LAST
         "bv_unit_class_flags" : 14,#UCF_LAST
         "bv_flags" : 64,           #F_MAX
         "bv_roles" : 64,           #L_MAX  
         "bv_terrain_flags" : 64,   #TER_MAX
         "bv_city_options" : 1,     #CITYO_LAST 
         "bv_special" : 8,          # S_LAST
         "bv_player" : 32,             # MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS 
         "bv_unit_classes" : 32,    #UCL_LAST
         "bv_bases" : 32,           #MAX_BASE_TYPES
         "bv_base_flags" : 5,       #BF_LAST
         "NUM_TRADEROUTES": 4
         
         }  

