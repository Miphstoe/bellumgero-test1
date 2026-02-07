/*
 * ImageDesignTerminalMenuComponent.cpp
 */

#include "ImageDesignTerminalMenuComponent.h"

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "server/zone/objects/tangible/TangibleObject.h"

#include "server/zone/objects/tangible/terminal/components/ImageDesignTerminalDataComponent.h"

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == nullptr || menuResponse == nullptr || player == nullptr)
		return;

	ManagedReference<TangibleObject*> tano = sceneObject->asTangibleObject();
	if (tano == nullptr)
		return;

	// Always offer Use option (even if not registered yet, it will instruct the user)
	menuResponse->addRadialMenuItemToRadialID(20, RADIAL_USE, 3, "Use Image Design (10,000 cr)");

	// Register option if unregistered OR the owner is interacting (allows refreshing snapshot later if you want)
	DataObjectComponent* dcomp = tano->getDataObjectComponent()->get();
	ImageDesignTerminalDataComponent* data = dynamic_cast<ImageDesignTerminalDataComponent*>(dcomp);

	if (data == nullptr || !data->isRegistered() || data->getOwnerId() == player->getObjectID()) {
		menuResponse->addRadialMenuItemToRadialID(20, RADIAL_REGISTER, 3, "Register Terminal");
	}

	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (sceneObject == nullptr || player == nullptr)
		return 0;

	if (!sceneObject->isTangibleObject() || !player->isPlayerCreature())
		return 0;

	ManagedReference<TangibleObject*> tano = sceneObject->asTangibleObject();
	if (tano == nullptr)
		return 0;

	if (selectedID != RADIAL_REGISTER && selectedID != RADIAL_USE)
		return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);

	DataObjectComponent* dcomp = tano->getDataObjectComponent()->get();
	ImageDesignTerminalDataComponent* data = dynamic_cast<ImageDesignTerminalDataComponent*>(dcomp);

	if (data == nullptr) {
		player->sendSystemMessage("This terminal is missing its data component (ImageDesignTerminalDataComponent).");
		return 0;
	}

	Locker locker(tano, player);

	if (selectedID == RADIAL_REGISTER) {
		// Must be an Image Designer (your server uses entertainer novice as the gate in /imagedesign)
		if (!player->hasSkill("social_entertainer_novice")) {
			player->sendSystemMessage("@ui_imagedesigner:noskill"); // You don't have any image designer skills
			return 0;
		}

		data->registerOwner(player);

		String msg = "Image Design Terminal registered to " + data->getOwnerName() + ".";
		player->sendSystemMessage(msg);

		return 0;
	}

	// RADIAL_USE
	if (!data->isRegistered()) {
		player->sendSystemMessage("This Image Design Terminal has not been registered yet. An Image Designer must register it first.");
		return 0;
	}

	// NOTE: The actual offline Image Design session + UI wiring is the next milestone.
	// We keep this stub in place so this milestone can be tested without touching ImageDesignSession behavior.
	player->sendSystemMessage("Image Design Terminal is registered. Offline Image Design session wiring will be enabled in the next step.");

	return 0;
}
