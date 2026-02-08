/*
 * ImageDesignTerminalMenuComponent.cpp
 *
 * Offline Image Design Terminal radial.
 *
 * Key points:
 * - Use RadialOptions::SERVER_MENU# for custom actions (safe, like other systems)
 * - For terminal use, mark the ImageDesignSession as a TERMINAL session
 *   via setTerminalContext + setTerminalSkillSnapshot before startImageDesign().
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
static constexpr int kTerminalPrice = 10000;

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == nullptr || menuResponse == nullptr || player == nullptr)
		return;

	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

	auto* tano = cast<TangibleObject*>(sceneObject);
	if (tano == nullptr)
		return;

	DataObjectComponentReference* dataRef = tano->getDataObjectComponent();
	auto* termData = (dataRef != nullptr) ? cast<ImageDesignTerminalDataComponent*>(dataRef->get()) : nullptr;

	const bool isRegistered = (termData != nullptr && termData->isRegistered());
	const bool isOwner = (termData != nullptr && termData->getOwnerId() == player->getObjectID());

	// Registration should require actual Image Designer skills (since it snapshots mods).
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

		// Use is available to everyone once registered
		menuResponse->addRadialMenuItem(RadialOptions::SERVER_MENU2, 3, "Use Image Design (10,000 cr)");
	}
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (sceneObject == nullptr || player == nullptr)
		return 0;

	auto* tano = cast<TangibleObject*>(sceneObject);
	if (tano == nullptr)
		return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);

	DataObjectComponentReference* dataRef = tano->getDataObjectComponent();
	auto* termData = (dataRef != nullptr) ? cast<ImageDesignTerminalDataComponent*>(dataRef->get()) : nullptr;

	// SERVER_MENU1 = Register / Re-register
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

		// If already registered, only the owner can re-register.
		if (termData->isRegistered() && termData->getOwnerId() != player->getObjectID()) {
			player->sendSystemMessage("Only the terminal owner may re-register this terminal.");
			return 0;
		}

		Locker locker(tano, player);
		termData->registerOwner(player);
		player->sendSystemMessage("Image Design Terminal registered.");
		return 0;
	}

	// SERVER_MENU2 = Use
	if (selectedID == RadialOptions::SERVER_MENU2) {
		if (termData == nullptr || !termData->isRegistered()) {
			player->sendSystemMessage("This terminal has not been registered.");
			return 0;
		}

		if (player->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
			player->sendSystemMessage("You are already using Image Design.");
			return 0;
		}

		// Snapshot skill-mods (optional but useful if other code reads getSkillMod())
		player->removeBuff(kTerminalBuffCRC);

		ManagedReference<Buff*> buff = new Buff(player, kTerminalBuffCRC, 300, BuffType::SKILL); // 5 minutes
		{
			Locker block(buff);

			String snap = termData->getSnapshotString();
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

						if (!key.isEmpty() && val != 0) {
							buff->setSkillModifier(key, val);
						}
					}
				}

				start = semi + 1;
			}
		}

		player->addBuff(buff);

		// ---- CRITICAL FIX ----
		// Mark the session as a terminal session BEFORE startImageDesign().
		// Your ImageDesignSessionImplementation.cpp only grants the UI-required skills
		// when isTerminalSession() == true.
		ManagedReference<ImageDesignSession*> session = new ImageDesignSession(player);
		session->deploy();

		session->setTerminalContext(tano->getObjectID(), kTerminalPrice);
		session->setTerminalSkillSnapshot(termData->getSnapshotString());

		Locker clocker(player);
		session->startImageDesign(player, player);

		return 0;
	}

	return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}
