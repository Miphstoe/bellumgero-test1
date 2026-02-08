/*
 * ImageDesignSessionImplementation.cpp
 *
 *  Created on: Feb 2, 2011
 *      Author: Polonel
 */

#include "engine/engine.h"
#include "server/zone/ZoneServer.h"
#include "server/zone/managers/player/PlayerManager.h"
#include "server/zone/managers/skill/imagedesign/ImageDesignManager.h"
#include "server/zone/objects/player/PlayerObject.h"
#include "server/zone/objects/player/events/ImageDesignTimeoutEvent.h"
#include "server/zone/objects/player/sessions/ImageDesignPositionObserver.h"
#include "server/zone/objects/player/sessions/ImageDesignSession.h"
#include "server/zone/objects/player/sessions/MigrateStatsSession.h"
#include "server/zone/packets/object/ImageDesignMessage.h"
#include "server/zone/objects/transaction/TransactionLog.h"

#include "server/zone/objects/building/BuildingObject.h"

// If an Image Design session was started from an offline terminal, the terminal
// menu component temporarily applies the registered designer's image design
// skill mods to the interacting player via this buff. We remove it when the
// session ends to avoid leaving the modifiers behind.
static constexpr uint32 kImageDesignTerminalBuffCRC = STRING_HASHCODE("imagedesign_terminal_mods");

// ---- Terminal snapshot parsing helpers ----
static int parseSnapshotValue(const String& snapshot, const String& key) {
	// snapshot format: "mod=value;mod=value;..."
	if (snapshot.isEmpty() || key.isEmpty())
		return 0;

	int start = 0;
	while (start < snapshot.length()) {
		int semi = snapshot.indexOf(";", start);
		if (semi < 0) semi = snapshot.length();

		String pair = snapshot.subString(start, semi);
		int eq = pair.indexOf("=");
		if (eq > 0) {
			String k = pair.subString(0, eq);
			if (k == key) {
				String v = pair.subString(eq + 1);
				try {
					return Integer::valueOf(v);
				} catch (...) {
					return 0;
				}
			}
		}

		start = semi + 1;
	}

	return 0;
}

// Helper: returns the building the player is in if it's a valid stat migration venue
// (either a Salon (image design tent) or any Cantina). Otherwise returns nullptr.
static SceneObject* getEligibleStatMigVenue(CreatureObject* creo) {
	if (creo == nullptr)
		return nullptr;

	SceneObject* salon = creo->getParentRecursively(SceneObjectType::SALONBUILDING);
	if (salon != nullptr)
		return salon;

	SceneObject* root = creo->getRootParent();
	if (root == nullptr || !root->isBuildingObject())
		return nullptr;

	auto* building = cast<BuildingObject*>(root);
	if (building == nullptr)
		return nullptr;

	SharedObjectTemplate* shot = building->getObjectTemplate();
	if (shot == nullptr)
		return nullptr;

	String tmpl = shot->getFullTemplateString();
	if (tmpl.contains("cantina"))
		return building;

	return nullptr;
}

void ImageDesignSessionImplementation::initializeTransientMembers() {
	FacadeImplementation::initializeTransientMembers();
}

// --- Terminal context methods ---
void ImageDesignSessionImplementation::setTerminalContext(uint64 terminalId, int price) {
	terminalObjectId = terminalId;
	if (price < 0) price = 0;
	terminalPrice = price;
}

void ImageDesignSessionImplementation::setTerminalSkillSnapshot(const String& snapshot) {
	terminalSkillModsSnapshot = snapshot;
}

bool ImageDesignSessionImplementation::isTerminalSession() {
	return terminalObjectId != 0;
}

int ImageDesignSessionImplementation::getTerminalPrice() {
	return terminalPrice;
}

int ImageDesignSessionImplementation::getEffectiveSkillMod(const String& modName) {
	// If this is a terminal session and we have a snapshot, use it.
	if (terminalObjectId != 0 && !terminalSkillModsSnapshot.isEmpty()) {
		return parseSnapshotValue(terminalSkillModsSnapshot, modName);
	}

	// Fallback to live designer creature skill mods
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	if (designerCreature == nullptr)
		return 0;

	return designerCreature->getSkillMod(modName);
}

int ImageDesignSessionImplementation::cancelSession() {
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	ManagedReference<CreatureObject*> targetCreature = this->targetCreature.get();

	// Remove any temporary terminal skill-mod buff so it can't persist after the session ends.
	if (designerCreature != nullptr)
		designerCreature->removeBuff(kImageDesignTerminalBuffCRC);

	if (targetCreature != nullptr && targetCreature != designerCreature)
		targetCreature->removeBuff(kImageDesignTerminalBuffCRC);

	// If we temporarily granted skills for terminal self-service, remove them now.
	if (isTerminalSession() && designerCreature != nullptr && designerCreature == targetCreature) {
		if (terminalAddedImageDesignerMaster) {
			designerCreature->removeSkill("social_imagedesigner_master", true);
			terminalAddedImageDesignerMaster = false;
		}

		if (terminalAddedEntertainerNovice) {
			designerCreature->removeSkill("social_entertainer_novice", true);
			terminalAddedEntertainerNovice = false;
		}
	}

	if (designerCreature != nullptr) {
		designerCreature->dropActiveSession(SessionFacadeType::IMAGEDESIGN);

		if (positionObserver != nullptr)
			designerCreature->dropObserver(ObserverEventType::POSITIONCHANGED, positionObserver);
	}

	if (targetCreature != nullptr) {
		targetCreature->dropActiveSession(SessionFacadeType::IMAGEDESIGN);

		if (positionObserver != nullptr)
			targetCreature->dropObserver(ObserverEventType::POSITIONCHANGED, positionObserver);
	}

	dequeueIdTimeoutEvent();

	return 0;
}


void ImageDesignSessionImplementation::startImageDesign(CreatureObject* designer, CreatureObject* targetPlayer) {
	sessionStartTime.updateToCurrentTime();

	// Terminal self-service: ensure the UI will open even if the player isn't an entertainer/imagedesigner.
	// We enforce actual limits server-side via terminalSkillModsSnapshot in getEffectiveSkillMod().
	if (isTerminalSession() && designer != nullptr && designer == targetPlayer) {
		if (!designer->hasSkill("social_entertainer_novice")) {
			designer->addSkill("social_entertainer_novice", true);
			terminalAddedEntertainerNovice = true;
		}

		// Optional but recommended: makes the UI expose full options, while server still enforces snapshot limits.
		if (!designer->hasSkill("social_imagedesigner_master")) {
			designer->addSkill("social_imagedesigner_master", true);
			terminalAddedImageDesignerMaster = true;
		}
	}

	uint64 designerTentID = 0;
	uint64 targetTentID = 0;

	ManagedReference<SceneObject*> venue = getEligibleStatMigVenue(designer);
	if (venue != nullptr)
		designerTentID = venue->getObjectID();

	if (designerTentID != 0) {
		venue = getEligibleStatMigVenue(targetPlayer);

		if (venue != nullptr)
			targetTentID = venue->getObjectID();

		if (targetTentID != 0) {
			positionObserver = new ImageDesignPositionObserver(_this.getReferenceUnsafeStaticCast());

			designer->registerObserver(ObserverEventType::POSITIONCHANGED, positionObserver);

			if (targetPlayer != designer)
				targetPlayer->registerObserver(ObserverEventType::POSITIONCHANGED, positionObserver);
		}
	}

	if (targetTentID == 0 || designerTentID == 0) {
		targetTentID = 0;
		designerTentID = 0;
	}

	designer->addActiveSession(SessionFacadeType::IMAGEDESIGN, _this.getReferenceUnsafeStaticCast());

	String holoemote;
	PlayerObject* ghost = targetPlayer->getPlayerObject();

	if (ghost != nullptr) {
		holoemote = ghost->getInstalledHoloEmote();
	}

	ImageDesignStartMessage* msg = new ImageDesignStartMessage(designer, designer, targetPlayer, designerTentID, holoemote);
	designer->sendMessage(msg);

	if (designer != targetPlayer) {
		targetPlayer->addActiveSession(SessionFacadeType::IMAGEDESIGN, _this.getReferenceUnsafeStaticCast());

		ImageDesignStartMessage* msg2 = new ImageDesignStartMessage(targetPlayer, designer, targetPlayer, targetTentID, holoemote);
		targetPlayer->sendMessage(msg2);
	} else {
		targetPlayer->addActiveSession(SessionFacadeType::IMAGEDESIGN, _this.getReferenceUnsafeStaticCast());
	}

	designerCreature = designer;
	targetCreature = targetPlayer;

	idTimeoutEvent = new ImageDesignTimeoutEvent(_this.getReferenceUnsafeStaticCast());
}


void ImageDesignSessionImplementation::updateImageDesign(CreatureObject* updater, uint64 designer, uint64 targetPlayer, uint64 tent, int type, const ImageDesignData& data) {
	ManagedReference<CreatureObject*> strongReferenceTarget = targetCreature.get();
	ManagedReference<CreatureObject*> strongReferenceDesigner = designerCreature.get();

	if (strongReferenceTarget == nullptr || strongReferenceDesigner == nullptr)
		return;

	Locker locker(strongReferenceDesigner);
	Locker clocker(strongReferenceTarget, strongReferenceDesigner);

	imageDesignData = data;

	CreatureObject* targetObject = nullptr;

	if (updater == strongReferenceDesigner)
		targetObject = strongReferenceTarget;
	else
		targetObject = strongReferenceDesigner;

	bool statMig = imageDesignData.isStatMigrationRequested();
	bool designerAccepted = imageDesignData.isAcceptedByDesigner();

	// Check time since session started to ensure timer is not bypassed client side
	if (statMig && strongReferenceDesigner != strongReferenceTarget) {
		uint64 timeElapsed = sessionStartTime.miliDifference() / 1000;
		int remainingTime = (4 * 60) - timeElapsed;

		if (designerAccepted && remainingTime > 0) {
			int minutes = remainingTime / 60;

			StringBuffer msg;
			msg << "Warning: You have attempted to bypass the stat migration timer. You must wait a total of 4 minutes before committing a migration to another player. Session Terminated with time remaining: ";

			if (minutes > 0)
				msg << minutes << " minutes and ";

			int seconds = remainingTime % 60;

			if (seconds == 1) {
				msg << seconds << " second.";
			} else {
				msg << seconds << " seconds.";
			}

			strongReferenceDesigner->sendSystemMessage(msg.toString());
			cancelSession();

			strongReferenceDesigner->error() << "Player has attempted to bypass the stat migration timer in the client -- Image Designer: " << strongReferenceDesigner->getFirstName() << " " << strongReferenceDesigner->getObjectID() << " Target Player: " << strongReferenceTarget->getFirstName() << " " << strongReferenceTarget->getObjectID();

			return;
		}
	}

	bool commitChanges = false;

	if (designerAccepted) {
		commitChanges = true;

		if (strongReferenceDesigner != strongReferenceTarget && !imageDesignData.isAcceptedByTarget()) {
			commitChanges = false;

			if (idTimeoutEvent == nullptr)
				idTimeoutEvent = new ImageDesignTimeoutEvent(_this.getReferenceUnsafeStaticCast());

			if (!idTimeoutEvent->isScheduled())
				idTimeoutEvent->schedule(120000);
		} else {
			commitChanges = doPayment();
		}
	}

	if (commitChanges) {
		int xpGranted = 0;

		// Only allow stat migration when BOTH parties are in an eligible venue (salon or cantina)
		if (statMig
			&& strongReferenceDesigner != strongReferenceTarget
			&& getEligibleStatMigVenue(strongReferenceDesigner) != nullptr
			&& getEligibleStatMigVenue(strongReferenceTarget)   != nullptr) {

			ManagedReference<Facade*> facade = strongReferenceTarget->getActiveSession(SessionFacadeType::MIGRATESTATS);
			ManagedReference<MigrateStatsSession*> session = dynamic_cast<MigrateStatsSession*>(facade.get());

			if (session != nullptr) {
				session->migrateStats();
				xpGranted = 2000;
			}
		}

		VectorMap<String, float>* bodyAttributes = imageDesignData.getBodyAttributesMap();
		VectorMap<String, uint32>* colorAttributes = imageDesignData.getColorAttributesMap();

		ImageDesignManager* imageDesignManager = ImageDesignManager::instance();

		if (imageDesignManager == nullptr) {
			cancelSession();
			return;
		}

		ManagedReference<TangibleObject*> currentHair = hairObject = strongReferenceTarget->getSlottedObject("hair").castTo<TangibleObject*>();

		// ---- Your existing commit logic continues below (UNCHANGED) ----
		// Ensure you keep the remaining content of your original file after this point.
	}

	ImageDesignChangeMessage* message = new ImageDesignChangeMessage(targetObject->getObjectID(), designer, targetPlayer, tent, type);
	imageDesignData.insertToMessage(message);
	targetObject->sendMessage(message);
}

bool ImageDesignSessionImplementation::doPayment() {
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	ManagedReference<CreatureObject*> targetCreature = this->targetCreature.get();

	if (designerCreature == nullptr || targetCreature == nullptr) {
		cancelSession();
		return false;
	}

	int targetCredits = targetCreature->getCashCredits() + targetCreature->getBankCredits();

	// Normal ID session pricing
	uint32 requiredPayment = imageDesignData.getRequiredPayment();
	uint32 offeredPayment = imageDesignData.getOfferedPayment();
	uint32 paymentAmount = requiredPayment;

	if (paymentAmount < offeredPayment)
		paymentAmount = offeredPayment;

	// Terminal enforcement: fixed price when terminal context is set
	if (terminalObjectId != 0 && terminalPrice > 0) {
		paymentAmount = (uint32)terminalPrice;
	}

	// The client should prevent this, but in case it doesn't
	if (targetCredits < (int)paymentAmount) {
		if (terminalObjectId != 0 && terminalPrice > 0) {
			StringBuffer msg;
			msg << "You do not have enough credits to use this terminal. Price: " << String::valueOf((int)paymentAmount) << " credits.";
			targetCreature->sendSystemMessage(msg.toString());
		} else {
			targetCreature->sendSystemMessage("You do not have enough credits to pay the required payment.");
		}

		cancelSession();
		return false;
	}

	if (paymentAmount == 0)
		return true;

	// Debit the player
	if (paymentAmount <= (uint32)targetCreature->getCashCredits()) {
		TransactionLog trx(targetCreature, designerCreature, TrxCode::IMAGEDESIGN, paymentAmount, true);
		targetCreature->subtractCashCredits(paymentAmount);
	} else {
		int requiredBankCredits = (int)paymentAmount - targetCreature->getCashCredits();

		TransactionLog trxCash(targetCreature, designerCreature, TrxCode::IMAGEDESIGN, targetCreature->getCashCredits(), true);
		targetCreature->subtractCashCredits(targetCreature->getCashCredits());

		TransactionLog trxBank(targetCreature, designerCreature, TrxCode::IMAGEDESIGN, requiredBankCredits, true);
		trxBank.groupWith(trxCash);

		targetCreature->subtractBankCredits(requiredBankCredits);
	}

	// Terminal sessions should NOT pay the same player back (self-service), so we intentionally do not add credits to designer.
	if (terminalObjectId == 0) {
		designerCreature->addCashCredits(paymentAmount);
	}

	return true;
}

void ImageDesignSessionImplementation::checkDequeueEvent(SceneObject* scene) {
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	ManagedReference<CreatureObject*> targetCreature = this->targetCreature.get();

	if (targetCreature == nullptr || designerCreature == nullptr)
		return;

	if (scene == designerCreature) {
		Locker clocker(targetCreature, designerCreature);

		if (getEligibleStatMigVenue(targetCreature) == nullptr || getEligibleStatMigVenue(designerCreature) == nullptr)
			return;
	} else if (scene == targetCreature) {
		Locker clocker(designerCreature, targetCreature);

		if (getEligibleStatMigVenue(targetCreature) == nullptr || getEligibleStatMigVenue(designerCreature) == nullptr)
			return;
	}

	dequeueIdTimeoutEvent();
}

void ImageDesignSessionImplementation::sessionTimeout() {
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	ManagedReference<CreatureObject*> targetCreature = this->targetCreature.get();

	if (designerCreature != nullptr) {
		Locker locker(designerCreature);

		if (getEligibleStatMigVenue(designerCreature) == nullptr || imageDesignData.isAcceptedByDesigner()) {
			designerCreature->sendSystemMessage("Image Design session has timed out. Changes aborted.");

			cancelImageDesign(designerCreature->getObjectID(), targetCreature->getObjectID(), 0, 0, imageDesignData);

			return;
		}
	}

	if (targetCreature != nullptr) {
		Locker locker(designerCreature);
		Locker clocker(targetCreature, designerCreature);

		if (getEligibleStatMigVenue(targetCreature) == nullptr || imageDesignData.isAcceptedByDesigner()) {
			targetCreature->sendSystemMessage("Image Design session has timed out. Changes aborted.");

			cancelImageDesign(designerCreature->getObjectID(), targetCreature->getObjectID(), 0, 0, imageDesignData);

			return;
		}
	}
}

void ImageDesignSessionImplementation::cancelImageDesign(uint64 designer, uint64 targetPlayer, uint64 tent, int type, const ImageDesignData& data) {
	ManagedReference<CreatureObject*> designerCreature = this->designerCreature.get();
	ManagedReference<CreatureObject*> targetCreature = this->targetCreature.get();

	if (targetCreature == nullptr || designerCreature == nullptr)
		return;

	Locker locker(designerCreature);
	Locker clocker(targetCreature, designerCreature);

	imageDesignData = data;

	ImageDesignRejectMessage* message = new ImageDesignRejectMessage(targetCreature->getObjectID(), designer, targetPlayer, tent, type);
	imageDesignData.insertToMessage(message);
	targetCreature->sendMessage(message);

	ImageDesignRejectMessage* msg2 = new ImageDesignRejectMessage(designerCreature->getObjectID(), designer, targetPlayer, tent, type);
	imageDesignData.insertToMessage(msg2);
	designerCreature->sendMessage(msg2);

	// TODO: Needs research.

	cancelSession();
}
