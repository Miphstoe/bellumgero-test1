/*
 * ImageDesignTerminalDataComponent.h
 *
 * Offline Image Design Terminal persistent data:
 * - ownerId / ownerName
 * - snapshot of designer skill mods used by customization_data.iff + "hair"
 *
 */

#ifndef IMAGEDESIGNTERMINALDATACOMPONENT_H_
#define IMAGEDESIGNTERMINALDATACOMPONENT_H_

#include "server/zone/objects/scene/components/DataObjectComponent.h"
#include "server/zone/objects/creature/CreatureObject.h"

class ImageDesignTerminalDataComponent : public DataObjectComponent {
protected:
	uint64 ownerId;
	SerializableString ownerName;
	SerializableString skillModsSnapshot; // "mod=value;mod=value;..."

	// transient cache (not serialized)
	mutable bool parsedSnapshot;
	mutable VectorMap<String, int> parsedSkillMods;

public:
	ImageDesignTerminalDataComponent() {
		ownerId = 0;
		ownerName = "";
		skillModsSnapshot = "";
		parsedSnapshot = false;

		addSerializableVariables();
	}

	virtual ~ImageDesignTerminalDataComponent() {
	}

	bool isImageDesignTerminalData() const {
		return true;
	}

	uint64 getOwnerId() const {
		return ownerId;
	}

	String getOwnerName() const {
		return ownerName;
	}

	bool isRegistered() const {
		return ownerId != 0;
	}

	/**
	 * Registers an owner and snapshots their Image Design skill mods.
	 * Call while parent + owner are locked.
	 */
	void registerOwner(CreatureObject* owner);

	/**
	 * Gets a skill mod value from the snapshot.
	 */
	int getSnapshotSkillMod(const String& modName) const;

	/**
	 * For debugging / admin visibility (optional).
	 */
	String getSnapshotString() const {
		return skillModsSnapshot;
	}

	void writeJSON(nlohmann::json& j) const override {
		DataObjectComponent::writeJSON(j);
		SERIALIZE_JSON_MEMBER(ownerId);
		SERIALIZE_JSON_MEMBER(ownerName);
		SERIALIZE_JSON_MEMBER(skillModsSnapshot);
	}

private:
	void addSerializableVariables() {
		addSerializableVariable("ownerId", &ownerId);
		addSerializableVariable("ownerName", &ownerName);
		addSerializableVariable("skillModsSnapshot", &skillModsSnapshot);
	}

	void buildSkillModSnapshot(CreatureObject* owner);
	void parseSnapshotIfNeeded() const;
};

#endif /* IMAGEDESIGNTERMINALDATACOMPONENT_H_ */
