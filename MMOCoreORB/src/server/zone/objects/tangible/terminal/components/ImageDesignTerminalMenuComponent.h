/*
 * ImageDesignTerminalMenuComponent.h
 *
 * Radial for Image Design Terminal:
 * - Register Terminal (owner only, requires image designer skill line)
 * - Use Image Design (10,000 cr) (stub for now; session/UI wiring comes next)
 */

#ifndef IMAGEDESIGNTERMINALMENUCOMPONENT_H_
#define IMAGEDESIGNTERMINALMENUCOMPONENT_H_

#include "server/zone/objects/tangible/components/TangibleObjectMenuComponent.h"

class ImageDesignTerminalMenuComponent : public TangibleObjectMenuComponent {
protected:
	enum {
		RADIAL_REGISTER = 120,
		RADIAL_USE      = 121
	};

public:
	virtual void fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const;
	virtual int handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const;
};

#endif /* IMAGEDESIGNTERMINALMENUCOMPONENT_H_ */
