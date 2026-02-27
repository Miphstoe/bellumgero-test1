/*
 * AddCityDefensesSuiCallback.h
 */

#ifndef ADDCITYDEFENSESSUICALLBACK_H_
#define ADDCITYDEFENSESSUICALLBACK_H_

#include "server/zone/objects/player/sui/SuiCallback.h"
#include "server/zone/Zone.h"
#include "server/zone/managers/city/CityManager.h"
#include "server/zone/managers/city/CityFactionTurretObserver.h"
#include "server/zone/objects/installation/TurretObject.h"
#include "templates/params/ObserverEventType.h"
#include "templates/faction/Factions.h"

class AddCityDefensesSuiCallback : public SuiCallback {
	static void fixupTrackedCityTurrets(CreatureObject* player, CityRegion* city) {
		if (player == nullptr || city == nullptr)
			return;

		Locker cityLock(city, player);

		const int count = city->getFactionTurretCount();
		for (int i = 0; i < count; ++i) {
			ManagedReference<SceneObject*> so = city->getCityFactionTurret(i);
			ManagedReference<TangibleObject*> to = so.castTo<TangibleObject*>();
			if (to == nullptr || !to->isTurret())
				continue;

			Locker tLock(to, player);
			// Runtime repair for legacy city turrets (old templates) so the mayor can remove them.
			to->setObjectMenuComponent("CityDefenseTurretMenuComponent");
		}
	}

public:
	AddCityDefensesSuiCallback(ZoneServer* server) : SuiCallback(server) {}

	void run(CreatureObject* player, SuiBox* suiBox, uint32 eventIndex, Vector<UnicodeString>* args) {
		ManagedReference<CityRegion*> city = player != nullptr ? player->getCityRegion().get() : nullptr;

		// Always attempt to repair any already-tracked city turrets so old ones become removable after a restart.
		if (player != nullptr && city != nullptr && city->isMayor(player->getObjectID()))
			fixupTrackedCityTurrets(player, city);

		// Cancel pressed
		if (eventIndex == 1)
			return;

		if (player == nullptr || args == nullptr || args->size() < 1)
			return;

		// Must be placed outdoors.
		if (player->getParent() != nullptr)
			return;

		CityManager* cityManager = player->getZoneServer()->getCityManager();
		if (city == nullptr || cityManager == nullptr)
			return;

		if (!city->isMayor(player->getObjectID()))
			return;

		Zone* zone = player->getZone();
		if (zone == nullptr)
			return;

		PlayerObject* ghost = player->getPlayerObject();
		if (ghost == nullptr || !ghost->hasAbility("addcitydefenses"))
			return;

		const String alignment = city->getCityFactionAlignment();
		if (alignment != "imperial" && alignment != "rebel") {
			player->sendSystemMessage("Your city must be faction-aligned (Rebel or Imperial) to place city defenses.");
			return;
		}

		if (!cityManager->canSupportMoreFactionTurrets(city)) {
			player->sendSystemMessage("Your city can't support any more city defense turrets at the current cap.");
			return;
		}

		int option = Integer::valueOf(args->get(0).toString());

		String turretTemplate = "";
		int deployCost = 5000;

		switch (option) {
		case 0:
			turretTemplate = "object/installation/city_defense/turret/tower_sm.iff";
			deployCost = 5000;
			break;
		case 1:
			turretTemplate = "object/installation/city_defense/turret/tower_med.iff";
			deployCost = 10000;
			break;
		case 2:
			turretTemplate = "object/installation/city_defense/turret/tower_lg.iff";
			deployCost = 20000;
			break;
		default:
			return;
		}

		Locker cityLock(city, player);

		if (city->getCityTreasury() < deployCost) {
			StringIdChatParameter msg;
			msg.setStringId("@city/city:action_no_money");
			msg.setDI(deployCost);
			player->sendSystemMessage(msg);
			return;
		}

		if (player->isSwimming() || player->isIncapacitated() || player->isDead())
			return;

		ManagedReference<TangibleObject*> object =
			(server->createObject(turretTemplate.hashCode(), "playerstructures", 1)).castTo<TangibleObject*>();

		if (object == nullptr || !object->isTurret()) {
			if (object != nullptr)
				object->destroyObjectFromDatabase(true);
			player->sendSystemMessage("Failed to deploy the turret. Try again.");
			return;
		}

		ManagedReference<TurretObject*> turret = object.castTo<TurretObject*>();
		if (turret == nullptr) {
			object->destroyObjectFromDatabase(true);
			return;
		}

		Locker turretLock(turret, player);

		// Set faction ONLY. PvP status is controlled by the LUA template (ATTACKABLE + OVERT).
		uint32 turretFaction = (alignment == "imperial") ? (uint32)Factions::FACTIONIMPERIAL : (uint32)Factions::FACTIONREBEL;
		turret->setFaction(turretFaction);

		// Place + face
		turret->initializePosition(player->getPositionX(), player->getPositionZ(), player->getPositionY());
		turret->setDirection(Math::deg2rad(player->getDirectionAngle()));

		zone->transferObject(turret, -1, true);
		turret->createChildObjects();

		// Ensure the city turret uses our removal/refund menu
		turret->setObjectMenuComponent("CityDefenseTurretMenuComponent");

		ManagedReference<CityFactionTurretObserver*> observer = new CityFactionTurretObserver();
		turret->registerObserver(ObserverEventType::OBJECTDESTRUCTION, observer);

		city->subtractFromCityTreasury(deployCost);
		city->addFactionTurret(turret);

		if (!city->isRegistered())
			zone->unregisterObjectWithPlanetaryMap(turret);
	}
};

#endif /* ADDCITYDEFENSESSUICALLBACK_H_ */