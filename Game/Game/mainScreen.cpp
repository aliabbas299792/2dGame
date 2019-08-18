#include <header.h>
#include <gui.h>

mainScreen::mainScreen(tgui::Gui &gui, networking* networkObject) : window(window), networkObject(networkObject) {
	tgui::Theme theme("Game.txt");
	//the above loads the colours and stuff for the widgets from the file "Game.txt"

	//below makes the inventory items buttons
	for (int i = 0; i < 7; i++) {
		smallInventoryButtons.push_back(tgui::BitmapButton::create());
		smallInventoryButtons[i]->setRenderer(theme.getRenderer("Button2"));
		smallInventoryButtons[i]->setSize("5%", "8%");
		smallInventoryButtons[i]->setPosition(std::to_string(65 + 5 * i) + "%", 0);

		mainScreenGroup->add(smallInventoryButtons[i]); //adds current one to the main group
	}

	smallInventoryButtons[0]->setRenderer(theme.getRenderer("Button3"));
	smallInventoryButtons[6]->setRenderer(theme.getRenderer("Button4"));
	smallInventoryButtons[6]->setImage("resources/more.png");
	//the above set some special cases which aren't possible through the loops

	/////////////////////////////
	////////ADD GROUP STUFF

	//makes a chat object and then adds it to the main screen group
	chatBox = new chat(25, 50, 2, 10);

	//the below for loop loops through the json object from the networking object, and adds all of them to the chat box scrollable panel
	for (int i = networkObject->messages.size() - 1; i >= 0; i--) {
		int time = std::stoi(networkObject->messages[i]["time"].get<std::string>());
		int msgID = std::stoi(networkObject->messages[i]["msgID"].get<std::string>());
		std::string imgLocation = networkObject->messages[i]["imgURL"].get<std::string>();
		std::string from = networkObject->messages[i]["from"].get<std::string>();
		std::string msg = networkObject->messages[i]["msg"].get<std::string>();

		if (imgLocation == "EMPTY") {
			imgLocation = "";
		}

		chatBox->addMessages(time, from, msg, imgLocation, msgID);
	}

	mainScreenGroup->add(chatBox->chatBoxContainer);

	//gives the network object the chatBox object, and sets the bool indicating whether the chatBox is active to true
	networkObject->chatBoxObject = chatBox;
	networkObject->chatBoxActive = true;

	//adds the main screen group to the gui and makes sure that it is invisible
	gui.add(mainScreenGroup);
	mainScreenGroup->setVisible(false);
}

void mainScreen::liveUpdate(sf::Clock* globalClock){
	chatBox->liveUpdate(networkObject, globalClock); //calls the live update function for the chat box, so basically allows for sending messages
}

void mainScreen::setActive(bool active) { //by setting the visibility through this, groups are really easy to manage so multiple screens are easy to manage
	if (active == true) { 
		//for the above, when the input parameter is true, it makes the group visible and sets the active boolean in the object as 
		//true (used to decide whether or not to have the live update function be active or not in the main game loop)
		this->active = true;
		mainScreenGroup->setVisible(true);
	}
	else {
		//sets the active variable false and makes the main screen group invisible so they won't be drawn
		this->active = false;
		mainScreenGroup->setVisible(false); 
	}
}