/*
 * CityDefenseTurretMenuComponent.h
 */

#ifndef CITYDEFENSETURRETMENUCOMPONENT_H_
#define CITYDEFENSETURRETMENUCOMPONENT_H_

#include "server/zone/objects/installation/components/TurretMenuComponent.h"

class CityDefenseTurretMenuComponent : public TurretMenuComponent {
public:
    void fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const override;
    int handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const override;
};

#endif /* CITYDEFENSETURRETMENUCOMPONENT_H_ */
