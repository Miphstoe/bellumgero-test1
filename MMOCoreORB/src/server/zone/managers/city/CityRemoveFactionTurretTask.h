#ifndef CITYREMOVEFACTIONTURRETTASK_H_
#define CITYREMOVEFACTIONTURRETTASK_H_

#include "server/zone/objects/region/CityRegion.h"
#include "server/zone/objects/scene/SceneObject.h"

class CityRemoveFactionTurretTask : public Task {
	ManagedReference<SceneObject*> turret;
	ManagedReference<CityRegion*> city;

	static int getRefundForTurret(SceneObject* obj) {
		if (obj == nullptr || !obj->isTurret())
			return 0;

		const String templ = obj->getObjectTemplate()->getFullTemplateString();

		// NEW city defense templates
		if (templ.contains("object/installation/city_defense/turret/tower_sm.iff"))
			return 5000;
		if (templ.contains("object/installation/city_defense/turret/tower_med.iff"))
			return 10000;
		if (templ.contains("object/installation/city_defense/turret/tower_lg.iff"))
			return 20000;

		// LEGACY support (old city turrets spawned from faction_perk templates)
		if (templ.contains("object/installation/faction_perk/turret/tower_sm.iff"))
			return 5000;
		if (templ.contains("object/installation/faction_perk/turret/tower_med.iff"))
			return 10000;
		if (templ.contains("object/installation/faction_perk/turret/tower_lg.iff"))
			return 20000;

		return 0;
	}

public:
	CityRemoveFactionTurretTask(SceneObject* sceno, CityRegion* cityRegion) {
		turret = sceno;
		city = cityRegion;
	}

	void run() {
		if (city == nullptr || turret == nullptr)
			return;

		Locker locker(city);
		Locker clocker(turret, city);

		const int refund = getRefundForTurret(turret);
		if (refund > 0)
			city->addToCityTreasury(refund);

		city->removeFactionTurret(turret);

		turret->destroyObjectFromWorld(true);
		turret->destroyObjectFromDatabase(true);
	}
};

#endif /* CITYREMOVEFACTIONTURRETTASK_H_ */