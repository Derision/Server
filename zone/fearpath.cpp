/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2006  EQEMu Development Team (http://eqemulator.net)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "../common/debug.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

#include "../common/rulesys.h"
#include "../common/MiscFunctions.h"
#include "zone_profile.h"
#include "map.h"
#include "zone.h"
#include "pathing.h"
#ifdef _WINDOWS
#define snprintf	_snprintf
#endif


extern Zone* zone;

#define FEAR_PATHING_DEBUG


//this is called whenever we are damaged to process possible fleeing
void Mob::CheckFlee() {
	//if were allready fleeing, dont need to check more...
	if(flee_mode && curfp)
		return;
	
	//dont bother if we are immune to fleeing
	if(SpecAttacks[IMMUNE_FLEEING] || spellbonuses.ImmuneToFlee)
		return;

	if(!flee_timer.Check())
		return;	//only do all this stuff every little while, since
				//its not essential that we start running RIGHT away
	
	//see if were possibly hurt enough
	float ratio = GetHPRatio();
	if(ratio >= RuleI(Combat, FleeHPRatio))
		return;
	
	//we might be hurt enough, check con now..
	Mob *hate_top = GetHateTop();
	if(!hate_top) {
		//this should never happen...
		StartFleeing();
		return;
	}
	
	float other_ratio = hate_top->GetHPRatio();
	if(other_ratio < 20) {
		//our hate top is almost dead too... stay and fight
		return;
	}
	
	//base our flee ratio on our con. this is how the 
	//attacker sees the mob, since this is all we can observe
	uint32 con = GetLevelCon(hate_top->GetLevel(), GetLevel());
	float run_ratio;
	switch(con) {
		//these values are not 100% researched
		case CON_GREEN:
			run_ratio = RuleI(Combat, FleeHPRatio);
			break;
		case CON_LIGHTBLUE:
			run_ratio = RuleI(Combat, FleeHPRatio) * 8 / 10;
			break;
		case CON_BLUE:
			run_ratio = RuleI(Combat, FleeHPRatio) * 6 / 10;
			break;
		default:
			run_ratio = RuleI(Combat, FleeHPRatio) * 4 / 10;
			break;
	}
	if(ratio < run_ratio)
	{
		if( RuleB(Combat, FleeIfNotAlone) 
		  || ( !RuleB(Combat, FleeIfNotAlone) 
		    && (entity_list.GetHatedCount(hate_top, this) == 0)))
			StartFleeing();

	}
}


void Mob::ProcessFlee() {

	//Stop fleeing if effect is applied after they start to run. 
	//When ImmuneToFlee effect fades it will turn fear back on and check if it can still flee. 
	if(flee_mode && (SpecAttacks[IMMUNE_FLEEING] || spellbonuses.ImmuneToFlee) && !spellbonuses.IsFeared){
		curfp = false;
		return;
	}

	//see if we are still dying, if so, do nothing
	if(GetHPRatio() < (float)RuleI(Combat, FleeHPRatio))
		return;
	
	//we are not dying anymore... see what we do next
	
	flee_mode = false;
	
	//see if we are legitimately feared now
	if(!spellbonuses.IsFeared) {
		//not feared... were done...
		curfp = false;
		return;
	}
}

float Mob::GetFearSpeed() {
    if(flee_mode) {
        //we know ratio < FLEE_HP_RATIO
        float speed = GetRunspeed();
        float ratio = GetHPRatio();

		// mob's movement will halt with a decent snare at HP specified by rule.
		if (ratio <= RuleI(Combat, FleeSnareHPRatio) && GetSnaredAmount() > 40) {
				return 0.0001f;
		}

		if (ratio < FLEE_HP_MINSPEED)
			ratio = FLEE_HP_MINSPEED;

        speed = speed * 0.5 * ratio / 100;
 
        return(speed);
    }
    return(GetRunspeed());
}

void Mob::CalculateNewFearpoint()
{
	if(RuleB(Pathing, Fear) && zone->pathing)
	{
		int Node = zone->pathing->GetRandomPathNode();
	
		VERTEX Loc = zone->pathing->GetPathNodeCoordinates(Node);

		++Loc.z;

		VERTEX CurrentPosition = {GetX(), GetY(), GetZ()};

		list<int> Route = zone->pathing->FindRoute(CurrentPosition, Loc);

		if(Route.size() > 0)
		{
			fear_walkto_x = Loc.x;
			fear_walkto_y = Loc.y;
			fear_walkto_z = Loc.z;
			curfp = true;

			mlog(PATHING__DEBUG, "Feared to node %i (%8.3f, %8.3f, %8.3f)", Node, Loc.x, Loc.y, Loc.z);
			return;
		}

		mlog(PATHING__DEBUG, "No path found to selected node. Falling through to old fear point selection.");
	}

	int loop = 0;
	float ranx, rany, ranz;
	curfp = false;
	while (loop < 100) //Max 100 tries
	{
		int ran = 250 - (loop*2);
		loop++;
		ranx = GetX()+MakeRandomInt(0, ran-1)-MakeRandomInt(0, ran-1);
		rany = GetY()+MakeRandomInt(0, ran-1)-MakeRandomInt(0, ran-1);
		ranz = FindGroundZ(ranx,rany);
		if (ranz == -999999)
			continue;
		float fdist = ranz - GetZ();
		if (fdist >= -12 && fdist <= 12 && CheckCoordLosNoZLeaps(GetX(),GetY(),GetZ(),ranx,rany,ranz))
		{
			curfp = true;
			break;
		}
	}
	if (curfp)
	{
		fear_walkto_x = ranx;
		fear_walkto_y = rany;
		fear_walkto_z = ranz;
	}
	else //Break fear
	{
		BuffFadeByEffect(SE_Fear);
	}
}
