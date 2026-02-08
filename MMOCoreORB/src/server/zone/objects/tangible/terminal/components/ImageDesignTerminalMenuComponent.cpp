#include "ImageDesignTerminalMenuComponent.h"

#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/player/PlayerObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "server/zone/objects/scene/components/DataObjectComponentReference.h"

#include "server/zone/objects/player/sessions/ImageDesignSession.h"

// Your data component header (adjust include path if yours differs)
#include "server/zone/objects/tangible/terminal/components/ImageDesignTerminalDataComponent.h"

static const int TERMINAL_PRICE = 10000;

// Radial IDs (make sure these don’t collide with other systems you’ve added)
static const byte RADIAL_REGISTER = 80;
static const byte RADIAL_USE      = 81;
static const byte RADIAL_SET_COST = 82; // optional if you still have it
static const byte RADIAL_REMOVE   = 83;

void ImageDesignTerminalMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == nullptr || menuResponse == nullptr || player == nullptr)
		return;

	// Mirror Vendor behavior: NO LOCKERS here (prevents deadlock -> infinite spinning menu)

	// Always allow normal tangible base behavior first (gives Examine / Pick Up / etc.)
	TangibleObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

	PlayerObject* ghost = player->getPlayerObject();
	if (ghost == nullptr)
		return;

	// Data component fetch exactly like Vendor
	DataObjectComponentReference* dataRef = sceneObject->getDataObjectComponent();
	if (dataRef == nullptr || dataRef->get() == nullptr)
		return;

	ImageDesignTerminalDataComponent* termData = cast<ImageDesignTerminalDataComponent*>(dataRef->get());
	if (termData == nullptr)
		return;

	// Root node (like Vendor Control)
	menuResponse->addRadialMenuItem(70, 3, "Image Design Terminal");

	// If not registered yet, only show register
	if (!termData->isRegistered()) {
		menuResponse->addRadialMenuItemToRadialID(70, RADIAL_REGISTER, 3, "Register Terminal");
		return;
	}

	// Registered: show use
	menuResponse->addRadialMenuItemToRadialID(70, RADIAL_USE, 3, "Use Image Design (10,000 cr)");

	// Owner-only options
	bool owner = (termData->getOwnerId() == player->getObjectID());

	if (owner || ghost->isPrivileged()) {
		// Optional: if you still support changing price
		// menuResponse->addRadialMenuItemToRadialID(70, RADIAL_SET_COST, 3, "Set Price");

		menuResponse->addRadialMenuItemToRadialID(70, RADIAL_REMOVE, 3, "Remove Terminal");
	}
}

int ImageDesignTerminalMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (sceneObject == nullptr || player == nullptr)
		return 0;

	// Mirror Vendor behavior: NO LOCKERS here (prevents deadlock)

	PlayerObject* ghost = player->getPlayerObject();
	if (ghost == nullptr)
		return 0;

	DataObjectComponentReference* dataRef = sceneObject->getDataObjectComponent();
	if (dataRef == nullptr || dataRef->get() == nullptr)
		return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);

	ImageDesignTerminalDataComponent* termData = cast<ImageDesignTerminalDataComponent*>(dataRef->get());
	if (termData == nullptr)
		return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);

	bool owner = (termData->getOwnerId() == player->getObjectID());

	switch (selectedID) {
	case RADIAL_REGISTER: {
		if (termData->isRegistered()) {
			player->sendSystemMessage("This terminal is already registered.");
			return 0;
		}

		// Keep your exact skill gate here (adjust if your server uses different skill names)
		if (!player->hasSkill("social_imagedesigner_novice") && !player->hasSkill("social_imagedesigner_master")) {
			player->sendSystemMessage("You must be an Image Designer to register this terminal.");
			return 0;
		}

		termData->registerOwner(player);
		player->sendSystemMessage("Terminal registered.");
		return 0;
	}

	case RADIAL_USE: {
		if (!termData->isRegistered()) {
			player->sendSystemMessage("This terminal is not registered yet.");
			return 0;
		}

		if (player->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
			player->sendSystemMessage("You already have an outstanding Image Design session.");
			return 0;
		}

		ManagedReference<ImageDesignSession*> session = new ImageDesignSession(player);
		session->deploy();

		session->setTerminalContext(sceneObject->getObjectID(), TERMINAL_PRICE);
		session->setTerminalSkillSnapshot(termData->getSnapshotString());

		// Self-service
		session->startImageDesign(player, player);
		return 0;
	}

	case RADIAL_REMOVE: {
		if (!owner && !ghost->isPrivileged()) {
			player->sendSystemMessage("You are not the owner of this terminal.");
			return 0;
		}

		TangibleObject* tang = cast<TangibleObject*>(sceneObject);
		if (tang == nullptr)
			return 0;

		tang->destroyObjectFromWorld(true);
		tang->destroyObjectFromDatabase(true);

		player->sendSystemMessage("Terminal removed.");
		return 0;
	}

	default:
		break;
	}

	return TangibleObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}
