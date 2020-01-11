#include "../../header/ecs.h"
#include "../../header/helper.h"

#include <SFML/Network.hpp>
#include <curl/curl.h>

using namespace ecs::system;
using namespace ecs::component;

void network::removeUser(unsigned int i)
{										
	//std::cout << "trying to get past main user mtuex...\n";
	std::lock_guard<std::mutex> mutex(mutexs::mainUserLockMutex);	//function to basically properly log out a user
	
	unsigned int entityID = users.vectorToEntityMap(i);	//get the user entityID
	sf::Vector2f coordinates = physicsObjects.compVec[physicsObjects.entityToVectorMap(entityID)].coordinates; //get the user's coordinates
	ecs::system::coordinatesStruct removeUserCoordinates(chunkCoordHelperX(int(coordinates.x), chunkPixelSize_x), chunkCoordHelperY(int(coordinates.y), chunkPixelSize_y)); //get what their coordinates would be in the world using simple mod math

	//std::cout << "trying to get past chunk lock mtuex...\n";
	mutexs::getInstance()->chunkLockMutex.lock();
	//std::cout << "removing user now\n";
	for(int i = 0; i < chunks[removeUserCoordinates].second.size(); i++){
		if(chunks[removeUserCoordinates].second[i].id == entityID){
			chunks[removeUserCoordinates].second.erase(chunks[removeUserCoordinates].second.begin() + i);
			break;
		}
	}
	chunks[removeUserCoordinates].first.userCount--; //decrements the number of users in this chunk
	updateActiveChunkData::getInstance()->updateActiveChunks(); //updates the active chunks
	mutexs::getInstance()->chunkLockMutex.unlock();

	sf::Packet packet; //a packet to hold a string
	packet << std::string("die").c_str(); //putting the c style string into the packet
	users.compVec[i].socket->send(packet); //sending the packet, this will cause the client to end if it hasn't already
	users.compVec[i].gameSocket->send(packet); //sending the packet, this will cause the client to end if it hasn't already

	sessionIDToEntityID.erase(users.compVec[i].sessionID); //will erase the mapping of the unique session ID to the index in the component vector for this user

	ecs::entity::superEntityManager.destroy(entity::entity(entityID), ecs::entity::USER); //removes them from the array if it is

	std::string msgString = "SERVER: A USER HAS LEFT\nSERVER: CLIENTS ONLINE: " + std::to_string(users.compVec.size()); //outputs some server info to indicate that the user is gone
	std::cout << msgString << "\n";	//outputs this string
}

void network::forwardToAllUsers(std::string msg, int userNum)
{ //forwards to every user
	for (int i = 0; i < users.compVec.size(); i++)
	{
		if (users.compVec[i].loggedIn != true)
		{
			continue;
		}

		if (users.compVec[i].roomGuild == users.compVec[userNum].roomGuild)
		{
			sf::Packet packet;	 //a packet to hold a string
			packet << msg.c_str(); //putting the c style string into the packet

			users.compVec[i].socket->send(packet); //sending the packet
		}
	}
}

void network::broadcastToLocal(std::string msg, int userNum)
{ //this broadcasts to users within 100 units of the player (hence the 100 in the if statement below)
	auto userChunk = coordinatesStruct(chunkCoordHelperX(physicsObjects.compVec[physicsObjects.entityToVectorMap(users.vectorToEntityMap(userNum))].coordinates.x, chunkPixelSize_x), chunkCoordHelperY(physicsObjects.compVec[physicsObjects.entityToVectorMap(users.vectorToEntityMap(userNum))].coordinates.y, chunkPixelSize_y));
	for(int x = userChunk.coordinates.first-1; x <= userChunk.coordinates.first+1; x++){
		for(int y = userChunk.coordinates.second-1; y <= userChunk.coordinates.second+1; y++){
			auto currentChunk = coordinatesStruct(x, y);
			
			mutexs::getInstance()->chunkLockMutex.lock();
			for(auto user : chunks[currentChunk].second){
				if(users.entityToVectorMap(user.id) != -1){
					if(users.compVec[users.entityToVectorMap(user.id)].roomGuild == users.compVec[userNum].roomGuild){
						sf::Packet packet;	 //a packet to hold a string
						packet << msg.c_str(); //putting the c style string into the packet

						users.compVec[users.entityToVectorMap(user.id)].socket->send(packet); //sends the packet to the user currently being looped over
					}
				}
			}
			mutexs::getInstance()->chunkLockMutex.unlock();

		}
	}
}

void network::saveMsgDB(std::string msg, int userNum, int time)
{
	CURL *curl = curl_easy_init(); //we can set options for this to make it control how a transfer/transfers will be made
	std::string readBuffer;		   //string for the returning data

	int id = users.compVec[userNum].userID;
	msg = curl_easy_escape(curl, msg.c_str(), msg.length());
	std::string roomGuild = users.compVec[userNum].roomGuild;

	curl_easy_setopt(curl, CURLOPT_URL, ("https://erewhon.xyz/game/serverMessages.php?save=true&id=" + std::to_string(id) + "&msg=" + msg + "&roomGuild=" + roomGuild + "&time=" + std::to_string(time)).c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); //the callback
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);		  //will write data to the string, so the fourth param of the last callback is stored here

	curl_easy_perform(curl);

	users.compVec[userNum].tempNewestMsgID = readBuffer;
}

void network::messageProcessing(){
	std::time_t t; //current time
	std::tm *now;  //object which processes into year, month, day, etc

	for (int i = 0; i < users.compVec.size(); i++){ //loops through every connected socket
		if (selector.isReady(*(users.compVec[i].socket)) && users.compVec[i].loggedIn) { 
			//checks through every socket in the selector, checking if it is ready for action (all sockets are in the selector so this is valid)
			sf::Packet receivePacket;		//packet for message to be received
			std::string receiveString = ""; //the string for the packet
			std::string currentTime;		//currentTime string
			std::string sendString;			//string to send to users

			users.compVec[i].socket->receive(receivePacket); //receives packet

			if ((receivePacket >> receiveString) && receiveString != "")
			{ //if the packet data can be extracted, and it is not empty
				t = std::time(0);		  //gets current time
				now = std::localtime(&t); //makes it into an object which we can extract time from, needed for saving messages in the database

				if (processString(receiveString))
				{ //if this contains the ping flag which is to be received every few seconds, update the socket expiry time
					users.compVec[i].timeOfExpiry = sf::seconds(expiryTimer.getElapsedTime().asSeconds() + 5); //updates the socket expiry time
					continue; //then continue, to the next loop
				}

				if (checkLeave(receiveString))
				{
					users.compVec[i].leave = true;
					break;
					/*
					removeUser(i);
					break; //maybe causing issue (crashing other clients)*/
				}

				if (receiveString.find("USER::CHANGEROOMGUILD::") == 0)
				{
					receiveString.erase(receiveString.begin(), receiveString.begin() + (receiveString.find("USER::CHANGEROOMGUILD::") + std::string("USER::CHANGEROOMGUILD::").length()));

					users.compVec[i].roomGuild = receiveString;

					continue;
				}

				if (!extractInformation(receiveString))
				{			  //if username and message could not be extracted...
					continue; //then continue
				}

				//below is basically saying to set the local message id to 0 if LOCALCHAT, you'd only use it to make sure your messages aren't recorded whatsoever
				if (users.compVec[i].roomGuild != "LOCALCHAT")
				{
					saveMsgDB(receiveString, i, t);
				}
				else
				{
					users.compVec[i].tempNewestMsgID = "-1"; //ID is not 0
				}

				std::string time = std::to_string(t);

				sendString += "MSG::ID::" + users.compVec[i].tempNewestMsgID;
				sendString += "USER::USERNAME::" + users.compVec[i].username;
				sendString += "USER::TIME::" + time;
				sendString += "USER::MSG::" + receiveString;

				std::cout << sendString << "\n";

				if (users.compVec[i].roomGuild != "LOCALCHAT")
				{
					forwardToAllUsers(sendString, i); //also sends to all users but the one which just sent this
				}
				else
				{
					broadcastToLocal(sendString, i);
				}

				currentTime = ""; //empties this string
			}
		}
	}
}

void network::process() {
	while (true)
	{ //loop endlessly
		if (selector.wait(sf::milliseconds(250))){ //waits 0.25 seconds for any activity from any of the sockets
			mutexs::getInstance()->mainUserLockMutex.lock();
			network::getInstance()->messageProcessing(); //will do the entire messages thing
			mutexs::getInstance()->mainUserLockMutex.unlock();
		}

		for (int i = 0; i < users.compVec.size(); i++){ //loops through every user...
			if (users.compVec[i].timeOfExpiry.asSeconds() <= expiryTimer.getElapsedTime().asSeconds()){ //checks if their socket's time of expiry is now or passed...
				//std::cout << "expiry timer issue, for user " << users.compVec[i].username << std::endl;
				users.compVec[i].leave = true;
			}
		}

		for(int i = 0; i < users.compVec.size(); i++){
			if(users.compVec[i].leave){
				//std::cout << "Removing user: " << users.compVec[i].username << std::endl;
				removeUser(i);
			}
		}

		sf::sleep(sf::milliseconds(50)); //slows down the process loop so less intensive on my poor laptop
	}
}

std::string network::login(std::string input, user *userPtr)
{
	std::string usernameToken = "SERVER::LOGON::USERNAME::";
	std::string username = input;

	if (input.find(usernameToken) == 0 != std::string::npos)
	{
		username.erase(username.begin(), (username.begin() + usernameToken.size()));

		CURL *curl = curl_easy_init(); //we can set options for this to make it control how a transfer/transfers will be made
		std::string readBuffer;		   //string for the returning data

		for (int i = 0; i < users.compVec.size(); i++)
		{ //makes sure the same user can't join twice
			if (users.compVec[i].username == username)
			{
				return "failed";
			}
		}

		username = curl_easy_escape(curl, username.c_str(), username.length());

		curl_easy_setopt(curl, CURLOPT_URL, ("http://erewhon.xyz/game/serverLogin.php?username=" + username).c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); //the callback
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);		  //will write data to the string, so the fourth param of the last callback is stored here

		curl_easy_perform(curl);

		if (readBuffer.find("AVATAR::") != 0 || readBuffer.find("AVATAR::") == std::string::npos || readBuffer.find("USER::ID::") == std::string::npos)
		{
			return "failed"; //login failed if no ID found
		}
		else
		{
			//the below will extract the user's avatar's name
			std::string avatar = readBuffer;
			avatar.erase(0, avatar.find("AVATAR::") + std::string("AVATAR::").size());
			avatar.erase(avatar.find("USER::ID::"), avatar.size());

			//the below will erase all irrelavent information and extract the user ID
			readBuffer.erase(0, readBuffer.find("USER::ID::") + std::string("USER::ID::").length());
			userPtr->userID = std::stoi(readBuffer);
			userPtr->username = username;

			return avatar;
		}
	}

	return "failed"; //otherwise return false
}

void network::server()
{							  //the function for server initialisation
	sf::TcpSocket *socket; //makes a socket pointer

	user *userPtr; //makes a user pointer

	std::cout << "SERVER: SERVER STARTED" << std::endl; //outputs that the server has started
	//unsigned int entityID2 = ecs::entity::superEntityManager.create({components::USER, components::LOCATION});
	//std::cout << entityID2 << std::endl;

	ecs::system::coordinatesStruct startCoord(0, 0); //the starting coordinates
	ecs::entity::entity tempEntity;					 //used to put the users entity inside some chunk

	while (true)
	{								//loop endlessly
		std::string outputString;   //the string to alert connected users that another user has joined
		socket = new sf::TcpSocket; //a new socket in case a user joins
		userPtr = new user;			//a new user struct in case a user joins

		if (listener.accept(*socket) == sf::Socket::Done)
		{ //if a new user has joined...
			//std::cout << "NEW USER" << std::endl;
			sf::Packet packet;

			socket->receive(packet);

			std::string receiveString;
			packet >> receiveString;

			sf::Packet sendPacket;

			std::string response = login(std::string(receiveString), userPtr); //this will either contain the user's avatar's name, or it will be "failed" in the event that it failed to log in
			if (response == "failed")
			{
				sendPacket << "SERVER::LOGON::RESPONSE::falseUSER::ID::-1";
				socket->send(sendPacket);

				delete userPtr;
				delete socket;
				continue;
			}
			else
			{
				//we are adding a user which could screw with all of the processing threads, so our hacky solution right now is to just use a mutex
				mutexs::getInstance()->mainUserLockMutex.lock();
				mutexs::getInstance()->chunkLockMutex.lock();

				int randomNumber = rand() * 100000; //appended to the end of the username for a unique ID kind of thing
				randomNumber = (randomNumber > 0 ? randomNumber : randomNumber*-1); //if it's negative, makes it positive
				userPtr->sessionID = userPtr->username + std::to_string(randomNumber);

				sendPacket << std::string("SERVER::LOGON::RESPONSE::trueUSER::SESSION_ID::" + userPtr->sessionID + "USER::ID::" + std::to_string(userPtr->userID)).c_str();
				socket->send(sendPacket);

				outputString = "SERVER: NEW CONNECTION @ " + socket->getRemoteAddress().toString() + "\n"; //make a string which includes their IP address...

				unsigned int entityID = ecs::entity::superEntityManager.create({components::USER, components::DRAWABLE, components::PHYSICAL}); //a new object with those attributes is made

				tempEntity.id = entityID; //sets the temp entity to have the correct ID
				//tempEntity.type = ecs::entity::USER; //sets the temp entity to have the correct ID
				
				ecs::system::chunks[startCoord].second.push_back(tempEntity); //pushes users to the (0, 0) chunk
				chunks[startCoord].first.userCount++; //increments the number of users in this chunk
				updateActiveChunkData::getInstance()->updateActiveChunks(); //updates the active chunks
				
				unsigned int componentVectorIndex = users.entityToVectorMap(entityID);	//gets the component vector index of this entity
				unsigned int physicsCompVecIndex = physicsObjects.entityToVectorMap(entityID);

				physicsObjects.compVec[physicsCompVecIndex].boxCorners = {sf::Vector2f(0, -7.5), sf::Vector2f(6, -7.5), sf::Vector2f(0, 0), sf::Vector2f(6, 0)}; //the bounding box for a user character
				physicsObjects.compVec[physicsCompVecIndex].coordinates.y = -5; //they start off at this height (floor is below them)

				userPtr->socket = socket;														   //make the new user object contain their socket
				userPtr->timeOfExpiry = sf::seconds(expiryTimer.getElapsedTime().asSeconds() + 5); //set the expiry time for their socket
				userPtr->loggedIn = true;
				userPtr->avatar = response;

				users.compVec[componentVectorIndex] = *userPtr;									 //add this user to the users vector
				sessionIDToEntityID.insert({userPtr->sessionID, entityID}); //will add a map entry, mapping their unique session ID to some index in the component vector

				selector.add(*socket); //add this vector to the selector
				std::cout << outputString;
				
				mutexs::getInstance()->chunkLockMutex.unlock();
				mutexs::getInstance()->mainUserLockMutex.unlock();
			}
		}
		else
		{ //if a new user hasn't joined...
			delete userPtr; //delete the user struct
			delete socket;  //and delete the socket
		}

		sf::sleep(sf::milliseconds(150)); //slows down the listener loop so less intensive on my poor laptop
	}
}