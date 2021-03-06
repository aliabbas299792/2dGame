#include "../header/ecs.h"
using namespace ecs::component;

template <class T>
void ecsComponentStructure<T>::addComponent(T component, unsigned int entityID){
    compVec.push_back(component); //adds to the component vector
    entityVectorMap.insert(entityID, (compVec.size()-1)); //inserts an entry in the map, entityID as the key, the index in the component vector as the value
}

template <class T>
void ecsComponentStructure<T>::removeComponent(unsigned int entityID){
    unsigned int index = entityVectorMap.getBfromA(entityID);
    if(entityVectorMap.countInA(entityID)){ //if the ID exists in the map
        if((compVec.size() - 1) == index){ //if this is the last element in the vector
            entityVectorMap.eraseFromA(entityID); //remove the entry for the entity
            compVec.pop_back(); //and remove the final element
        }else{
            int lastEntityID = entityVectorMap.getAfromB(compVec.size()-1);

            compVec[index] = compVec[compVec.size() - 1]; //replaces the value at the index of the element to remove with the values of the last element in the vector

            entityVectorMap.eraseFromA(lastEntityID);
            entityVectorMap.eraseFromA(entityID); //and erase the entry for the entity who's component we want to remove

            entityVectorMap.insert(lastEntityID, index); //sets the component vector index of the final element to now be equal to the index that we wanted to remove, as we have made the value of the index equal to the final elements value
       
            compVec.pop_back(); //removes the final entry in the component vector, we do this after we change the entity vector map entries, as otherwise it could contribute to crashing players
        }
    }
}

template <class T>
unsigned int ecsComponentStructure<T>::entityToVectorMap(unsigned int entityID){ //returns the component vector index associated with some entity ID
    if(entityVectorMap.countInA(entityID)){
        return entityVectorMap.getBfromA(entityID); //uses my custom bimap thing
    }else{
        return -1;
    }
}

template <class T>
unsigned int ecsComponentStructure<T>::vectorToEntityMap(unsigned int componentIndex){ //returns the entity ID that is associated with this component vector index
    if(entityVectorMap.countInB(componentIndex)){
        return entityVectorMap.getAfromB(componentIndex); //uses my custom bimap thing
    }else{
        return -1;
    }
}

//these are explicit instantiations for every type that this object will be used for, so it needs to be updated for any new components
template class ecsComponentStructure<ecs::component::drawable>;
template class ecsComponentStructure<ecs::component::user>;
template class ecsComponentStructure<ecs::component::physical>;
template class ecsComponentStructure<ecs::component::mob>;
template class ecsComponentStructure<ecs::component::mp_hp>;
template class ecsComponentStructure<ecs::component::npc>;
template class ecsComponentStructure<ecs::component::mission>;
template class ecsComponentStructure<ecs::component::thrown_item>;