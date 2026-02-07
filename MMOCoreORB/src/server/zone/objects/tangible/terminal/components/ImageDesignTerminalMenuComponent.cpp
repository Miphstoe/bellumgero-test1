/*
 * ImageDesignTerminalMenuComponent.cpp
 *
 * Minimal + safe radial build to avoid client "spinning" when the server
 * fails to send ObjectMenuResponse (usually due to a crash/null deref).
 */

#include "ImageDesignTerminalMenuComponent.h"

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "server/zone/objects/creature/CreatureObject.h"

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == nullptr || menuResponse == nullptr || player == nullptr)
		return;

	// IMPORTANT: add our items first, then call base (matches several stable forks)
	menuResponse->addRadialMenuItemToRadialID(20, 80, 3, "Register Terminal");
	menuResponse->addRadialMenuItemToRadialID(20, 81, 3, "Use Image Design (10,000 cr)");

	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (sceneObject == nullptr || player == nullptr)
		return 0;

	if (selectedID == 80) {
		player->sendSystemMessage("Register clicked (debug).");
		return 0;
	}

	if (selectedID == 81) {
		player->sendSystemMessage("Use clicked (debug).");
		return 0;
	}

	return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}

