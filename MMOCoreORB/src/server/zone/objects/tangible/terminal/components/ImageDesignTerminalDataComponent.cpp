/*
 * ImageDesignTerminalDataComponent.cpp
 */

#include "ImageDesignTerminalDataComponent.h"

#include "templates/manager/TemplateManager.h"
#include "templates/datatables/DataTableIff.h"
#include "templates/datatables/DataTableRow.h"

void ImageDesignTerminalDataComponent::registerOwner(CreatureObject* owner) {
	if (owner == nullptr)
		return;

	ownerId = owner->getObjectID();
	ownerName = owner->getFirstName();

	buildSkillModSnapshot(owner);

	// reset transient cache
	parsedSnapshot = false;
	parsedSkillMods.removeAll();
}

int ImageDesignTerminalDataComponent::getSnapshotSkillMod(const String& modName) const {
	parseSnapshotIfNeeded();

	for (int i = 0; i < parsedSkillMods.size(); ++i) {
		if (parsedSkillMods.elementAt(i).getKey() == modName)
			return parsedSkillMods.elementAt(i).getValue();
	}

	return 0;
}

void ImageDesignTerminalDataComponent::parseSnapshotIfNeeded() const {
	if (parsedSnapshot)
		return;

	parsedSkillMods.removeAll();
	parsedSnapshot = true;

	String snap = skillModsSnapshot;

	if (snap.isEmpty())
		return;

	// format: "mod=value;mod=value;..."
	int start = 0;
	while (start < snap.length()) {
		int semi = snap.indexOf(";", start);
		if (semi < 0)
			semi = snap.length();

		String pair = snap.subString(start, semi);

		if (!pair.isEmpty()) {
			int eq = pair.indexOf("=");
			if (eq > 0) {
				String key = pair.subString(0, eq);
				String valStr = pair.subString(eq + 1);

				int val = 0;
				try {
					val = Integer::valueOf(valStr);
				} catch (...) {
					val = 0;
				}

				parsedSkillMods.put(key, val);
			}
		}

		start = semi + 1;
	}
}

void ImageDesignTerminalDataComponent::buildSkillModSnapshot(CreatureObject* owner) {
	if (owner == nullptr)
		return;

	TemplateManager* templateManager = TemplateManager::instance();
	IffStream* iffStream = templateManager->openIffFile("datatables/customization/customization_data.iff");

	Vector<String> uniqueMods;

	// Always include hair, because ImageDesignManager::createHairObject checks "hair" directly
	uniqueMods.add("hair");

	if (iffStream != nullptr) {
		DataTableIff dataTable;
		dataTable.readObject(iffStream);

		// Column 15 is imageDesignSkillMod (see templates/customization/CustomizationData.h)
		for (int i = 0; i < dataTable.getTotalRows(); ++i) {
			DataTableRow* row = dataTable.getRow(i);
			if (row == nullptr)
				continue;

			String mod;
			try {
				row->getValue(15, mod);
			} catch (...) {
				mod = "";
			}

			if (mod.isEmpty())
				continue;

			bool found = false;
			for (int j = 0; j < uniqueMods.size(); ++j) {
				if (uniqueMods.get(j) == mod) {
					found = true;
					break;
				}
			}

			if (!found)
				uniqueMods.add(mod);
		}

		delete iffStream;
		iffStream = nullptr;
	}

	// Build snapshot: "mod=value;mod=value;..."
	StringBuffer sb;

	for (int i = 0; i < uniqueMods.size(); ++i) {
		const String& modName = uniqueMods.get(i);
		int val = owner->getSkillMod(modName);

		sb << modName << "=" << val << ";";
	}

	skillModsSnapshot = sb.toString();
}
