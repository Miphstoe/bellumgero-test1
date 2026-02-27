-- City defense turret weapon template.
-- Based on turret_tower_large.lua, but isolated under a new template path so we can tune later
-- without affecting GCW/base turrets.

object_weapon_ranged_turret_city_turret_tower = object_weapon_ranged_turret_shared_city_turret_tower:new {
	-- HEAVYLIGHTNINGBEAMATTACK, HEAVYPARTICLEBEAMATTACK, HEAVYROCKETLAUNCHERATTACK, HEAVYLAUNCHERATTACK
	attackType = RANGEDATTACK,

	-- ENERGY, KINETIC, ELECTRICITY, STUN, BLAST, HEAT, COLD, ACID, LIGHTSABER
	damageType = ENERGY,

	-- NONE, LIGHT, MEDIUM, HEAVY
	armorPiercing = HEAVY,

	-- See http://www.ocdsoft.com/files/accuracy.xls
	creatureAccuracyModifiers = { "rifle_accuracy" },
	creatureAimModifiers = { "rifle_aim", "aim" },

	-- See http://www.ocdsoft.com/files/defense.xls
	defenderDefenseModifiers = { "ranged_defense" },
	defenderSecondaryDefenseModifiers = { },

	-- See http://www.ocdsoft.com/files/speed.xls
	speedModifiers = { "rifle_speed" },

	damageModifiers = { },

	pointBlankRange = 0,
	pointBlankAccuracy = 0,
	idealRange = 40,
	idealAccuracy = 70,
	maxRange = 80,
	maxRangeAccuracy = 0,

	minDamage = 300,
	maxDamage = 700,
	attackSpeed = 1,
	woundsRatio = 9,
}

ObjectTemplates:addTemplate(object_weapon_ranged_turret_city_turret_tower, "object/weapon/ranged/turret/city_turret_tower.iff")