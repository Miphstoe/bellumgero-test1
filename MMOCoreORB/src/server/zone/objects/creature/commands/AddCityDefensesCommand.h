/*
 * AddCityDefensesCommand.h
 *
 * Mayor-only command to place city defense turrets (faction perk turrets) with city treasury costs.
 */

#ifndef ADDCITYDEFENSESCOMMAND_H_
#define ADDCITYDEFENSESCOMMAND_H_

#include <system/thread/Locker.h>

#include "server/zone/objects/player/sui/listbox/SuiListBox.h"
#include "server/zone/objects/creature/commands/sui/AddCityDefensesSuiCallback.h"
#include "server/zone/objects/region/CityRegion.h"

class AddCityDefensesCommand : public QueueCommand {
public:
	AddCityDefensesCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		PlayerObject* ghost = creature->getPlayerObject();
		if (ghost == nullptr)
			return GENERALERROR;

		if (!ghost->hasAbility("addcitydefenses"))
			return GENERALERROR;

		if (creature->isIncapacitated() || creature->isDead())
			return GENERALERROR;

		ManagedReference<CityRegion*> city = creature->getCityRegion().get();
		if (city == nullptr)
			return GENERALERROR;

		if (!city->isMayor(creature->getObjectID()))
			return GENERALERROR;

		const String alignment = city->getCityFactionAlignment();
		if (alignment != "imperial" && alignment != "rebel") {
			creature->sendSystemMessage("Your city must be faction-aligned (Rebel or Imperial) to place city defenses.");
			return GENERALERROR;
		}

		ManagedReference<SuiListBox*> sui = new SuiListBox(creature, SuiWindowType::CITY_STATUS_REPORT, 0);
		sui->setCallback(new AddCityDefensesSuiCallback(server->getZoneServer()));

		sui->setPromptTitle("Add City Defenses");
		sui->setPromptText("Select a turret to deploy at your current position. The listed cost will be deducted from the city treasury.");

		// Costs are enforced again in the callback.
		sui->addMenuItem("Small Defense Turret (5,000c)", 0);
		sui->addMenuItem("Medium Defense Turret (10,000c)", 1);
		sui->addMenuItem("Large Defense Turret (20,000c)", 2);

		ghost->addSuiBox(sui);
		creature->sendMessage(sui->generateMessage());

		return SUCCESS;
	}

};

#endif /* ADDCITYDEFENSESCOMMAND_H_ */
