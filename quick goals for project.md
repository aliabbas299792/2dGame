# Goals

- keep the jump to bottom thing for now

  - but you'll need to make a special case of it to prevent jump to bottom when old messages are loaded

  - run a checker on another thread to check for old messages every 5 seconds and deletes anything beyond last 50 messages

    - it should loop through all of the elements, and delete all but newest 50

    - and add

      - the database for messages should be like these columns: 

      + msgID (primary key), roomGuildID (so in form [roomID.guildID]), relativeMsgID (so increment ID of the msg relative to that specific room)

      - so with this, there may be 100 msgs in room.guild, and a msgID could be 4352 but relativeMsgID can be 100, so we can discern exactly what the newest ones are
      - realistically separate roomGuildID into two numbers, and then store roomID, guildID, msgID and relativeMsg as 4 integers in the msgContainer panel, and I've edited the include files so that the 4 integers are public variables of tgui::Panel

    - using these we can easily use the checker method on some thread to delete all the old messages

- get rid of loading animation, too slow, just have "Loading..." instead
- make the buttons redirect to empty pages wherever necessary (like in the designs), and make the tabs in those too
- find out why accessToken is sending
   the wrong items
- then find out if there are any other
   problems
- and then remove print_r stuff from logout.php
- and remove cout and stuff from lines 190, 191, 282 and 283 of server.cpp
- restart the computer to get rid of random issues
- restart the laptop for the same reason
- get the ftp file transfer working
- get the code to download all the files when none are present working, then the below
- once that's done get the directory making system working
- then make the system to actually remove unused files working too
- then youre done with the auto updater

# Achieved Goals

- Chat between players on GUI works
- refactor chatbox code
- fix that nullptr issue --this was done by the refactoring alone, as the error arose because i believe that the chatBox would be active one loop after the the main screen object told the window to close, and it was then checking for keyboard presses which caused a runtime error, but now that the chatBox was made part of the mainScreenGame object, it would stop checking for keyboard presses at the same time as as when the window was told to close so it fixes the error
- made the chatBox code a bit cleaner and made it easy to reuse elsewhere (like the planned social tab)
- refactored some more and split the main screen group off from the toolbar (the ones in the top right), as they will appear on all screens except the loading and launcher screens
- simple xor encryption (really simple, easily hackable but it sets a precedent)
- made it so messages from the same user in the last 10 minutes are grouped together
- i changed the include files and tried to use those to store msgID, but i was unaware this would cause undefined behaviour as this requires recompilation of the DLLs as well, so i've reversed that action and undefined behaviour (containers of objects just not working correctly whatsoever), and am now just going to use tgui labels inside tgui panels to hold any extra information from now on (if need be)
- added social tab panel
- newest build of TGUI was needed for right click behaviour, and i forgot to put the DLLs in the x64 folder too, which is necessary
- you can left click on the listed room/guild things and you will be switched to that one accordingly, and the chat box on the main screen will switch to the currently selected one too
- auto updater
- automatically looks at old checksum, compares with remote checksum to download new files
- removes files which are supposedly outdated/don't exist in the remote checksum, and checks they exist before doing so to prevent errors
- connect to database
- make it load 50 messages at first
- fix the button scaling, so like 0.7 for everything

# Abandoned Goals

- then make it load 50 more messages when at top of that, and 50 more and so on

# Stuff I've changed in the include files

> - in ScrollablePanel.hpp (TGUI/Widgets), I've moves 2 lines from private to public:
>   - CopiedSharedPtr<ScrollbarChildWidget> m_verticalScrollbar;
>  CopiedSharedPtr<ScrollbarChildWidget> m_horizontalScrollbar;
>   - this allowed me to access the vertical scroll value of the panel's scrollbar
> - In Panel.hpp I've added 4 lines to public, so I can store the message ID's and stuff:
>   - int relativeMsgID = 0;
>  int roomID = 0;
>  int networkID = 0;
>  int msgID = 0;
> - and I've reversed everything I did, because not only did the developer of tgui implement ability to manipulate the scrollbars without editing the class, it also causes undefined behaviour (as noted above)