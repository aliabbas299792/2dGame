#ifndef NETWORK_HEADER
#define NETWORK_HEADER

#include <iostream>
#include <string>
#include "gui.h"
#include <curl/curl.h>
#include <SFML/Network.hpp>
#include <json.hpp>

using json = nlohmann::json;

class networking {
private:
	std::string msg;

	sf::Clock* keepAliveTimer = NULL;

	sf::Time pingTime;

	friend size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp); //friend function to get data from curl, this is forward declaring the function as we can't include the header file here, as this file is included in it
public:
	std::string roomGuild = "main.alpha"; //public because we need to update this a lot, stores current room/guild (this is the one you start in)

	int userID; //will hold the user ID

	json messages; //messages from DB container

	void getMessagesFromDB(); //this will update the internal messages buffer

	chat *chatBoxObject = NULL;
	std::string usernameReal = ""; //this global variable would store the username
	bool chatBoxActive = false; //is the small chat box active or not

	bool active = false; //this single global variable allows us to indicate whether or not the server connection is active
	//we make the above public so that we can see in the GUI bit whether or not there is a connection

	sf::TcpSocket *socket = new sf::TcpSocket; //makes a new socket

	std::string ip; //the IP address to connect to
	unsigned short port; //the port to connect to

	bool login(std::string username);
	//this will loop repeatedly until either the user crosses it off or gets the correct login details

	void sendMessage(std::string msg);
	//sends any message passed to it, to the server

	void getResponses();
	//this will get any responses from the server, decode them, and output them, and it's run on a separate thread

	void stayAlive();
	//this will ping the server periodically so that the server knows that this client is still acutally connected, as there's no other real way of checking, and it's run on another thread

	networking(std::string IPADDRESS, int PORT, sf::Clock * globalClock) : ip(IPADDRESS), port(PORT), keepAliveTimer(globalClock) {};
	//the constructor to store all the necessary data and references to start the networking bit

	~networking(); //destructor to delete socket
};

#endif