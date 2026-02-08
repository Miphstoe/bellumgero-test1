#ifndef IMAGEDESIGNTERMINALMENUCOMPONENT_H_
#define IMAGEDESIGNTERMINALMENUCOMPONENT_H_

#include "server/zone/objects/tangible/components/TangibleObjectMenuComponent.h"

class ImageDesignTerminalMenuComponent : public TangibleObjectMenuComponent {
public:
	void fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const override;
	int handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const override;
};

#endif /* IMAGEDESIGNTERMINALMENUCOMPONENT_H_ */
