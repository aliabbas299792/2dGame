//you need to define this to make sure <windows.h> doesn't conflict with any min()/max() functions, 
//as this header file has min() and max() functions as macros, so would otherwise cause conflix
#define NOMINMAX

#include <Windows.h>
#include <header.h>
#include <TGUI/TGUI.hpp>

void helpFunction() { //the function to make the help button open the help setting in browser
	ShellExecute(0, 0, L"https://erewhon.xyz/game/help/", 0, 0, SW_SHOW);
	//opens URL in browser,
	//1st param is handle to parent windows, but we're using SFML rather than win32 windows so this is NULL or 0
	//2nd param is the action, which isnt necessary here
	//3rd is the thing to 'do' (the URL), so the default browser would be used on windows
	//4th would be parameters to pass to the file, but not necessary for opening a URL
	//5th is the working directory of the action, none specified so current one used
	//6th is how to show the application once opened, so it shows it
}

bool launcherBit(networking* networkObject, sf::Thread* pingThread, sf::Thread* receiveThread, sf::Clock* globalClock) {
	sf::RenderWindow *launcherWindow = new sf::RenderWindow(sf::VideoMode(800, 600), "window", sf::Style::Close); //window, the one used for the launcher
	launcher* launcherObject = new launcher(networkObject, launcherWindow, pingThread, receiveThread, globalClock); //launcher

	while (launcherWindow->isOpen())
	{
		sf::Event event;
		while (launcherWindow->pollEvent(event)) //check for events
		{
			if (event.type == sf::Event::Closed)
				launcherWindow->close(); //close the window when you find the close event basically

			launcherObject->gui->handleEvent(event); // Pass the event to the widgets
		}

		if (networkObject->active == true) { //this means the login was successful
			launcherWindow->close(); //close the window afterwards
		}

		launcherWindow->clear(); //clears the previous contents of the screen off

		launcherObject->liveUpdate(); //does the entire notifications bit as well as drawing widgets

		launcherWindow->display(); //the contents of the screen are shown

		sf::sleep(sf::milliseconds(50)); //so the program doesnt just fry your CPU
	}

	delete launcherWindow;
	delete launcherObject;

	//below would evaluate whether or not a login was succesful and would return the appropriate value
	if (networkObject->active == true) {
		return true;
	}
	else {
		return false;
	}
}

void gameBit(sf::Clock* globalClock, networking* networkObject){
	//the below is just the settings stuff for it
	sf::ContextSettings settings;
	settings.antialiasingLevel = 0; //so drawn objects don't look too sharp (especially for circles and stuff)

	sf::RenderWindow *gameWindow = new sf::RenderWindow(sf::VideoMode(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height), "window", sf::Style::Fullscreen, settings); 
	//fullscreen window for the game

	tgui::Gui gui(*gameWindow); //the main gui for the entire game bit

	sf::Time loadingScreenRemove = sf::milliseconds(globalClock->getElapsedTime().asMilliseconds() + 5000); //so it sets the duration of the loading screen as 2 seconds

	//below makes the loading screen, main screen, and tool bar (buttons in the top left) objects
	loadingScreen *loadingBit = new loadingScreen(gameWindow, globalClock);  
	//the above is dynamically allocated because we won't need it after and so we can know it's actually gone (as we can set it to NULL)
	mainScreen mainGameScreen(gui, networkObject);
	toolbar mainToolbar(gameWindow, gui);

	while (gameWindow->isOpen()) //so long as the window is open
	{
		sf::Event event; //will store the current event
		while (gameWindow->pollEvent(event)) //check for events
		{
			if (event.type == sf::Event::Closed) 
				gameWindow->close(); //close the window when you find the close event basically

			gui.handleEvent(event); // Pass the event to the widgets
		}

		gameWindow->clear(sf::Color(26, 25, 30)); //clears the previous contents of the screen off, and replaces it with a nice colour

		//below is the stuff for the loading screen, basically it says that if the expiry time has passed and the loading screen isn't null, delete it, set it to null
		//and then make the main screen active, otherwise draw the loading screen stuff and that little animation too
		if (loadingScreenRemove.asSeconds() - globalClock->getElapsedTime().asSeconds() <= 0 && loadingBit != NULL) {
			delete loadingBit;
			loadingBit = NULL;
			mainGameScreen.setActive(true);
			mainToolbar.toolbarGroup->setVisible(true); //sets the toolbar as visible too
		}
		else if (loadingBit != NULL) {
			loadingBit->liveUpdate();
		}

		//the below would check if the main game screen has been made active, and to then call the live update method, 
		//which calls the chat's live update method (for sending messages and stuff)
		if (mainGameScreen.active == true)
			mainGameScreen.liveUpdate();

		gui.draw(); //draws everything that's been added to it (hopefully just groups of tgui objects for the different screens)

		gameWindow->display(); //the contents of the screen are shown
		sf::sleep(sf::milliseconds(15)); //so the program doesnt just fry your CPU
	}

	delete gameWindow;
}

void clearResources(networking* networkObject, sf::Thread* pingThread, sf::Thread* receiveThread, sf::Clock* globalClock){
	//From here on, the TCP connection is severed, the threads are waited on and then destroyed, and then the network and launcher objects are deleted, and 0 is returned
	std::string msg; //the string used in getInput(...)
	sf::Packet sendPacket; //the packet which will contain the data to send
	msg = "USER::LEAVE";
	//this will help with decoding the data on the server side
	sendPacket << msg.c_str(); //converts the string into a C style array, and puts it into the packet which will be sent
	networkObject->socket->send(sendPacket); //sends the packet

	if (pingThread) { //if the ping thread exists...
		pingThread->wait(); //wait for it's current bit of code to finish
		delete pingThread; //then delete it
	}

	if (receiveThread) { //if the receiveThread exists...
		receiveThread->wait(); //wait for it's current bit of code to finish
		delete receiveThread; //then delete it
	}

	//when delete is called, the destructor is called before deallocating the memory
	delete networkObject;

	delete globalClock; //deletes the clock
}