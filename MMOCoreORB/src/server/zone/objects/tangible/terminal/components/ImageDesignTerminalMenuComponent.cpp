#include "ImageDesignTerminalMenuComponent.h"
#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	// IMPORTANT: do NOT lock here. The radial manager already has the correct locks.
	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

	if (player == nullptr || sceneObject == nullptr)
		return;

	// Basic proof-of-life entries
	menuResponse->addRadialMenuItem(200, 3, "Use Image Design");
	menuResponse->addRadialMenuItem(201, 3, "Set Price");
	menuResponse->addRadialMenuItem(202, 3, "Pack Up");
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	// IMPORTANT: do NOT lock here either (unless you really know what you're doing).
	if (player == nullptr || sceneObject == nullptr)
		return 0;

	switch (selectedID) {
	case 200:
		player->sendSystemMessage("Use Image Design (stub)");
		break;
	case 201:
		player->sendSystemMessage("Set Price (stub)");
		break;
	case 202:
		player->sendSystemMessage("Pack Up (stub)");
		break;
	default:
		break;
	}

	return 0;
}
