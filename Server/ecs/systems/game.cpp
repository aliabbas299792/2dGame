#include "../../header/ecs.h"

using namespace ecs::system;
using namespace ecs::component;

void game::runGame()
{
	while (true)
	{
		mutexs::getInstance()->chunkLockMutex.lock();
		physics::getInstance()->moveEntities();
		updateActiveChunkData::getInstance()->updateChunkData();
		gameBroadcast::getInstance()->broadcastGameState();
		mutexs::getInstance()->chunkLockMutex.unlock();

		sf::sleep(sf::milliseconds(1000 / fps)); //will only run the *fps* number of times per second at most, maybe slightly longer due to mutex locking
	}
}