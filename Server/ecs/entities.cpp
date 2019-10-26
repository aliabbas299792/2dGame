#include "../header/ecs.h"
using namespace ecs::entity;

unsigned int entityManager::create(std::initializer_list<ecs::component::components> initialiseWithStructs){
    nextEntity.id = 0;
    while(alive(nextEntity)){ //loops through all entities until an unused ID is found
        nextEntity.id++;
    }

    entities.insert(nextEntity); //and when it is found, the entity with this ID is inserted

    for(int i = 0;i < initialiseWithStructs.size(); i++){ //loops through the initializer
        if(*(initialiseWithStructs.begin()+i) == ecs::component::components::USER){ //uses the *(struct.begin()+i) rather than struct[i] notation because that notation doesn't work here
            ecs::component::user userTemp;
            ecs::component::users.addComponent(userTemp, nextEntity.id); //appropriate component added to the appropriate structure
        }
        
        if(*(initialiseWithStructs.begin()+i) == ecs::component::components::LOCATION){
            ecs::component::location location;
            ecs::component::locationStructs.addComponent(location, nextEntity.id); //appropriate component added to the appropriate structure
        }
    }
    
    return nextEntity.id; //returns the entity ID
}

bool entityManager::alive(entity entityStruct){ //returns whether or not the entity ID provided is in use or not basically
    if(entities.find(entityStruct) != entities.end()){
        return true;
    }else{
        return false;
    }
}

void entityManager::destroy(unsigned int entityID){ //removes entities and all of the associated components
    ecs::component::users.removeComponent(entityID);
    ecs::component::locationStructs.removeComponent(entityID);
    //add in any other component objects

    nextEntity.id = entityID; //uses the entity ID to basically make a temporary entity using nextEntity
    entities.erase(nextEntity); //erases the entity which is exactly like that
    nextEntity.id = 0; //and sets the entity ID to 0 so that nextEntity can be used elsewhere with no concerns
}