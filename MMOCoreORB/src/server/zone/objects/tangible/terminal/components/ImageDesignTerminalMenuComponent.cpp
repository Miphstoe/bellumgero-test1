/*
 * ImageDesignTerminalMenuComponent.cpp
 *
 * Offline Image Design Terminal radial.
 *
 * CRITICAL:
 * - Do NOT use addRadialMenuItemToRadialID() (it can throw and cause infinite spinning radial).
 * - The client may send RadialOptions::IMAGEDESIGN (built-in) instead of SERVER_MENU2.
 *   We intercept ITEM_USE and IMAGEDESIGN and route them into terminal use logic.
 */

#include "ImageDesignTerminalMenuComponent.h"

#include "engine/engine.h"

#include "server/zone/managers/radial/RadialOptions.h"
#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/player/sessions/ImageDesignSession.h"
#include "server/zone/objects/tangible/TangibleObject.h"
#include "server/zone/objects/creature/buffs/Buff.h"

#include "server/zone/objects/tangible/terminal/components/ImageDesignTerminalDataComponent.h"

static constexpr uint32 kTerminalBuffCRC = STRING_HASHCODE("imagedesign_terminal_mods");
static constexpr int kTerminalDefaultPrice = 10000;
static constexpr float kTerminalUseRange = 7.0f;

static inline bool isTerminalUseSelection(byte selectedID) {
	return (selectedID == RadialOptions::SERVER_MENU2 ||
	        selectedID == RadialOptions::ITEM_USE ||
	        selectedID == RadialOptions::IMAGEDESIGN);
}

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == nullptr || menuResponse == nullptr || player == nullptr)
		return;

	// Keep base options (examine, etc.)
	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

	auto* tano = cast<TangibleObject*>(sceneObject);
	if (tano == nullptr)
		return;

	DataObjectComponentReference* dataRef = tano->getDataObjectComponent();
	auto* termData = (dataRef != nullptr) ? cast<ImageDesignTerminalDataComponent*>(dataRef->get()) : nullptr;

	const bool isRegistered = (termData != nullptr && termData->isRegistered());
	const bool isOwner = (termData != nullptr && termData->getOwnerId() == player->getObjectID());

	// Registration requires real Image Designer skills (since it snapshots mods)
	const bool hasAnyImageDesignerSkill =
		player->hasSkill("social_imagedesigner_novice") || player->hasSkill("social_imagedesigner_master");

	if (!isRegistered) {
		if (hasAnyImageDesignerSkill) {
			menuResponse->addRadialMenuItem(RadialOptions::SERVER_MENU1, 3, "Register Terminal");
		}
	} else {
		if (isOwner && hasAnyImageDesignerSkill) {
			menuResponse->addRadialMenuItem(RadialOptions::SERVER_MENU1, 3, "Re-Register Terminal");
		}

		// Give this a distinct label so you can visually confirm you're clicking OUR option.
		menuResponse->addRadialMenuItem(RadialOptions::SERVER_MENU2, 3, "Use Offline Image Design (10,000 cr)");
	}
}

static void applySnapshotBuff(CreatureObject* player, const String& snapshot) {
	static constexpr uint32 kTerminalBuffCRC = STRING_HASHCODE("imagedesign_terminal_mods");

	player->removeBuff(kTerminalBuffCRC);

	ManagedReference<Buff*> buff = new Buff(player, kTerminalBuffCRC, 300, BuffType::SKILL); // 5 minutes
	{
		Locker block(buff);

		// snapshot format: "mod=value;mod=value;..."
		int start = 0;
		while (start < snapshot.length()) {
			int semi = snapshot.indexOf(";", start);
			if (semi < 0)
				semi = snapshot.length();

			String pair = snapshot.subString(start, semi);
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

					if (!key.isEmpty() && val != 0) {
						buff->setSkillModifier(key, val);
					}
				}
			}

			start = semi + 1;
		}
	}

	player->addBuff(buff);
}

static int startTerminalSession(TangibleObject* terminal, CreatureObject* player, ImageDesignTerminalDataComponent* termData) {
	if (termData == nullptr || !termData->isRegistered()) {
		player->sendSystemMessage("This terminal has not been registered.");
		return 0;
	}

	ManagedReference<SceneObject*> parent = terminal->getParent().get();
	if ((parent != nullptr && !parent->isCellObject()) || !player->isInRange(terminal, kTerminalUseRange)) {
		player->sendSystemMessage("You must be within 7 meters of the terminal to use it.");
		return 0;
	}

	if (player->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
		player->sendSystemMessage("You are already using Image Design.");
		return 0;
	}

	const String snapshot = termData->getSnapshotString();
	applySnapshotBuff(player, snapshot);

	int price = (int)termData->getPrice();
	if (price <= 0)
		price = kTerminalDefaultPrice;

	ManagedReference<ImageDesignSession*> session = new ImageDesignSession(player);
	session->deploy();

	session->setTerminalContext(terminal->getObjectID(), price);
	session->setTerminalSkillSnapshot(snapshot);

	Locker clocker(player);
	session->startImageDesign(player, player);

	return 0;
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (sceneObject == nullptr || player == nullptr)
		return 0;

	auto* terminal = cast<TangibleObject*>(sceneObject);
	if (terminal == nullptr)
		return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);

	DataObjectComponentReference* dataRef = terminal->getDataObjectComponent();
	auto* termData = (dataRef != nullptr) ? cast<ImageDesignTerminalDataComponent*>(dataRef->get()) : nullptr;

	// Register / Re-register
	if (selectedID == RadialOptions::SERVER_MENU1) {
		if (termData == nullptr) {
			player->sendSystemMessage("This terminal is missing its data component.");
			return 0;
		}

		const bool hasAnyImageDesignerSkill =
			player->hasSkill("social_imagedesigner_novice") || player->hasSkill("social_imagedesigner_master");

		if (!hasAnyImageDesignerSkill) {
			player->sendSystemMessage("You do not have the required Image Designer skills to register this terminal.");
			return 0;
		}

		if (termData->isRegistered() && termData->getOwnerId() != player->getObjectID()) {
			player->sendSystemMessage("Only the terminal owner may re-register this terminal.");
			return 0;
		}

		Locker locker(terminal, player);
		termData->registerOwner(player);

		// Default price for now; adjust however you want.
		termData->setPrice(kTerminalDefaultPrice);

		player->sendSystemMessage("Image Design Terminal registered.");
		return 0;
	}

	// Use (ours, default Use, OR built-in IMAGEDESIGN)
	if (isTerminalUseSelection(selectedID)) {
		return startTerminalSession(terminal, player, termData);
	}

	return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}
