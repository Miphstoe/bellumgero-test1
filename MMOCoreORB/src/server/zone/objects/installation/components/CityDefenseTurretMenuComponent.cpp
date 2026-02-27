/*
 * CityDefenseTurretMenuComponent.cpp
 */

#include "CityDefenseTurretMenuComponent.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "server/zone/objects/region/CityRegion.h"
#include "server/zone/managers/city/CityRemoveFactionTurretTask.h"

void CityDefenseTurretMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
    TurretMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

    ManagedReference<CityRegion*> city = sceneObject->getCityRegion().get();

    if (city != nullptr && city->isMayor(player->getObjectID()))
        menuResponse->addRadialMenuItem(72, 3, "@city/city:mt_remove"); // Remove
}

int CityDefenseTurretMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
    if (selectedID == 72) {
        ManagedReference<CityRegion*> city = sceneObject->getCityRegion().get();

        if (city != nullptr && city->isMayor(player->getObjectID())) {
            CityRemoveFactionTurretTask* task = new CityRemoveFactionTurretTask(sceneObject, city);
            task->execute();
            player->sendSystemMessage("@city/city:mt_removed");
        }

        return 0;
    }

    return TurretMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}
