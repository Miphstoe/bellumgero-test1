/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef IMAGEDESIGNCOMMAND_H_
#define IMAGEDESIGNCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/player/sessions/ImageDesignSession.h"

class ImagedesignCommand : public QueueCommand {
public:

	ImagedesignCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->isPlayerCreature())
			return GENERALERROR;

		ManagedReference<CreatureObject*> designer = cast<CreatureObject*>(creature);
		if (designer == nullptr)
			return GENERALERROR;

		// --------------------------------------------------------------------
		// TERMINAL SAFETY GUARD:
		// If a terminal menu click already started an Image Design session,
		// the client may still fire /imagedesign immediately afterward.
		// In that case, do NOT send noskill or any other error message.
		// Just return success and let the already-running session continue.
		// --------------------------------------------------------------------
		if (designer->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
			return SUCCESS;
		}

		ManagedReference<SceneObject*> object = server->getZoneServer()->getObject(target);

		// -------------------------
		// NORMAL IMAGE DESIGN PATH
		// -------------------------
		if (!designer->hasSkill("social_entertainer_novice")) {
			designer->sendSystemMessage("@ui_imagedesigner:noskill"); // You don't have any image designer skills
			return GENERALERROR;
		}

		CreatureObject* playerTarget = nullptr;

		if (object == nullptr || !object->isPlayerCreature())
			playerTarget = designer;
		else
			playerTarget = cast<CreatureObject*>(object.get());

		Locker clocker(playerTarget, creature);

		if (playerTarget->isDead()) {
			designer->sendSystemMessage("@image_designer:target_dead");
			return GENERALERROR;
		}

		if (playerTarget->isInvisible()) {
			designer->sendSystemMessage("You can't image design an invisible player.");
			return GENERALERROR;
		}

		// --- GROUP CHECKING
		if (designer != playerTarget) {
			StringIdChatParameter stringIdNotGrp;
			stringIdNotGrp.setStringId("@image_designer:not_in_same_group");
			stringIdNotGrp.setTT(playerTarget->getObjectID());

			if (!designer->isGrouped() || designer->getGroupID() != playerTarget->getGroupID()) {
				designer->sendSystemMessage(stringIdNotGrp);
				return GENERALERROR;
			}
		}

		if (playerTarget->containsActiveSession(SessionFacadeType::IMAGEDESIGN) && playerTarget != designer) {
			StringIdChatParameter stringId;
			stringId.setStringId("@image_designer:outstanding_offer");
			stringId.setTT(playerTarget->getObjectID());

			designer->sendSystemMessage(stringId);
			return GENERALERROR;
		}

		ManagedReference<Facade*> facade = designer->getActiveSession(SessionFacadeType::IMAGEDESIGN);
		ManagedReference<ImageDesignSession*> session = dynamic_cast<ImageDesignSession*>(facade.get());

		if (session != nullptr) {
			designer->sendSystemMessage("@image_designer:already_image_designing");
			return GENERALERROR;
		}

		// Create Session
		session = new ImageDesignSession(designer);
		session->deploy();
		session->startImageDesign(designer, playerTarget);

		return SUCCESS;
	}

};

#endif //IMAGEDESIGNCOMMAND_H_
