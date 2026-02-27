object_installation_city_defense_turret_tower_lg = object_installation_faction_perk_turret_shared_tower_lg:new {
	lotSize = 1,
	pvpStatusBitmask = ATTACKABLE + OVERT,
	optionsBitmask = 0,
	maxCondition = 300000,

	gameObjectType = 4105,

	groundZoneComponent = "TurretZoneComponent",
	dataObjectComponent = "TurretDataComponent",
	containerComponent = "TurretContainerComponent",
	objectMenuComponent = "CityDefenseTurretMenuComponent",

	-- Damagetypes in WeaponObject
	vulnerability = BLAST + LIGHTSABER,
	-- LIGHT, MEDIUM, HEAVY
	rating = HEAVY,

	kinetic = 90,
	energy = 95,
	electricity = 90,
	stun = 100,
	blast = -1,
	heat = 90,
	cold = 90,
	acid = 90,
	lightSaber = -1,
	chanceHit = 4,
	weapon = "object/weapon/ranged/turret/city_turret_tower.iff",
}

ObjectTemplates:addTemplate(object_installation_city_defense_turret_tower_lg, "object/installation/city_defense/turret/tower_lg.iff")