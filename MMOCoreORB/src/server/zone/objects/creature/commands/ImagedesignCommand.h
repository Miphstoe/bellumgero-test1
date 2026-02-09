/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef IMAGEDESIGNCOMMAND_H_
#define IMAGEDESIGNCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/player/sessions/ImageDesignSession.h"

// Terminal support
#include "server/zone/objects/tangible/TangibleObject.h"
#include "server/zone/objects/creature/buffs/Buff.h"
#include "server/zone/objects/tangible/terminal/components/ImageDesignTerminalDataComponent.h"

class ImagedesignCommand : public QueueCommand {
public:

	ImagedesignCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	static void applySnapshotBuff(CreatureObject* player, const String& snapshot) {
		static constexpr uint32 kTerminalBuffCRC = STRING_HASHCODE("imagedesign_terminal_mods");

		player->removeBuff(kTerminalBuffCRC);

		ManagedReference<Buff*> buff = new Buff(player, kTerminalBuffCRC, 300, BuffType::SKILL); // 5 minutes
		{
			Locker block(buff);

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

		// If a session is already active, don't spam errors.
		if (designer->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
			return SUCCESS;
		}

		ManagedReference<SceneObject*> object = server->getZoneServer()->getObject(target);

		// ---------------------------------------------------------
		// TERMINAL EXCEPTION:
		// If the target has ImageDesignTerminalDataComponent, run terminal session.
		// This bypasses entertainer/imagedesigner skill requirements.
		// ---------------------------------------------------------
		if (object != nullptr && object->isTangibleObject()) {
			TangibleObject* terminal = cast<TangibleObject*>(object.get());
			if (terminal != nullptr) {
				DataObjectComponentReference* dataRef = terminal->getDataObjectComponent();
				ImageDesignTerminalDataComponent* termData = (dataRef != nullptr) ? cast<ImageDesignTerminalDataComponent*>(dataRef->get()) : nullptr;

				if (termData != nullptr && termData->isImageDesignTerminalData() && termData->isRegistered()) {
					static constexpr float kTerminalUseRange = 7.0f;
					static constexpr int kTerminalDefaultPrice = 10000;

					ManagedReference<SceneObject*> parent = terminal->getParent().get();
					if ((parent != nullptr && !parent->isCellObject()) || !designer->isInRange(terminal, kTerminalUseRange)) {
						designer->sendSystemMessage("You must be within 7 meters of the terminal to use it.");
						return GENERALERROR;
					}

					if (designer->containsActiveSession(SessionFacadeType::IMAGEDESIGN)) {
						return SUCCESS;
					}

					const String snapshot = termData->getSnapshotString();
					applySnapshotBuff(designer, snapshot);

					int price = (int)termData->getPrice();
					if (price <= 0)
						price = kTerminalDefaultPrice;

					ManagedReference<ImageDesignSession*> session = new ImageDesignSession(designer);
					session->deploy();

					session->setTerminalContext(terminal->getObjectID(), price);
					session->setTerminalSkillSnapshot(snapshot);

					Locker clocker(designer);
					session->startImageDesign(designer, designer);

					return SUCCESS;
				}
			}
		}

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

		session = new ImageDesignSession(designer);
		session->deploy();
		session->startImageDesign(designer, playerTarget);

		return SUCCESS;
	}

};

#endif // IMAGEDESIGNCOMMAND_H_
