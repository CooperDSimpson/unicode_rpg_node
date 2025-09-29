#include <windows.h>
#include <chrono>
#include <codecvt>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <io.h>
#include <iostream>
#include <locale>
#include <memory>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <wchar.h>

#pragma comment(lib, "winmm.lib")

// headers
#include "utils.hpp"

std::wstring utf8_to_wstring(const std::string &utf8_str) {
  if (utf8_str.empty()) {
    return L"";
  }

  // Check for UTF-8 BOM (0xEF, 0xBB, 0xBF) and remove it if present
  std::string content = utf8_str;
  if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
      static_cast<unsigned char>(content[1]) == 0xBB &&
      static_cast<unsigned char>(content[2]) == 0xBF) {
    content = content.substr(3); // Remove BOM
  }

  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
  if (size_needed <= 0) {
    return L"";
  }

  std::wstring wstr(size_needed - 1, 0); // -1 to remove null terminator
  MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wstr[0], size_needed);

  return wstr;
}

// Function to read file into a string and convert to wstring
std::wstring read_file_as_wstring(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::wcerr << "Error opening file! Make sure '" << filename.c_str()
               << "' exists.\n";
    return L"";
  }

  // Read file contents into a string
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  if (content.empty()) {
    std::wcerr << "Error: File read resulted in an empty string.\n";
    return L"";
  }

  // Convert to wstring (handles BOM)
  return utf8_to_wstring(content);
}

// Function to enable Unicode output in Windows Console
void enable_unicode_output() { _setmode(_fileno(stdout), _O_WTEXT); }

enum States { UNKNOWN = -1, WANDERING, INVENTORY };

struct Item {
  std::wstring name;
  std::wstring description;
};

struct Room {
  // attributes
  std::wstring name;
  std::wstring description;
  std::vector<Item> inventory = {};

  // Map to hold directions to other rooms
  std::map<std::wstring, Room *> connections;

  // Function to connect rooms
  void connectRoom(const std::wstring &direction, Room *room) {
    connections[direction] = room;
  }

  // Function to propagate from one room to another
  Room *move(const std::wstring &direction) {
    auto it = connections.find(direction);
    if (it != connections.end()) {
      return it->second; // Return the connected room
    } else {
      std::wcout << L"Invalid direction: " << direction << std::endl;
      return nullptr;
    }
  }

  // Optional: Display room connections for debugging
  void displayConnections() {
    std::wcout << name << L" connections: ";
    for (const auto &connection : connections) {
      std::wcout << connection.first << L" -> " << connection.second->name
                 << L" | ";
    }
    std::wcout << std::endl;
  }
};

struct Player {
private:
public:
  // variables that I dont care to make getters and setters for:
  unsigned long health = 100;
  unsigned long max_health = 100;
  int state = UNKNOWN;
  std::vector<Item> inventory = {};

  // getters:
  // setters:
};

class World {
public:
  Player player;
  // A map of all rooms in the world (using room names as keys)
  std::map<std::wstring, std::shared_ptr<Room>> rooms;

  // The current room where the player is located
  Room *currentRoom = nullptr;

  // Add a room to the world
  void addRoom(const std::wstring &roomName) {
    auto room = std::make_shared<Room>();
    room->name = roomName;
    room->description = L"undefined";
    rooms[roomName] = room;
  }

  // Find a room by name
  std::shared_ptr<Room> getRoom(const std::wstring &roomName) {
    auto it = rooms.find(roomName);
    if (it != rooms.end()) {
      return it->second;
    } else {
      std::wcout << L"Room not found: " << roomName << std::endl;
      return nullptr;
    }
  }

  // Function to connect two rooms in the world with direction for both rooms
  void connectRooms(const std::wstring &roomName1,
                    const std::wstring &direction1,
                    const std::wstring &roomName2,
                    const std::wstring &direction2) {
    // Retrieve the rooms by name
    auto room1 = getRoom(roomName1);
    auto room2 = getRoom(roomName2);

    if (room1 && room2) {
      // Connect room1 to room2 in direction1 and vice versa
      room1->connectRoom(direction1, room2.get());
      room2->connectRoom(direction2, room1.get());
    }
  }

  // Function to display all rooms and their connections for debugging
  void displayWorldConnections() {
    for (const auto &[name, room] : rooms) {
      room->displayConnections();
    }
  }

  // Set the starting room for the player using the room name
  void setStartingRoom(const std::wstring &roomName) {
    currentRoom =
        getRoom(roomName)
            .get(); // Retrieve the room by name and set it as the current room
  }

  // Function to handle movement based on direction
  void move(const std::wstring &direction) {
    if (currentRoom) {
      Room *nextRoom = currentRoom->move(direction);
      if (nextRoom) {
        currentRoom = nextRoom; // Move to the next room
        std::wcout << L"Moved to: " << currentRoom->name << std::endl;
      }
    }
  }

  // Function to display the current room and its description
  void displayCurrentRoom() {
    if (currentRoom) {
      std::wcout << L"You are in: " << currentRoom->name << std::endl;
      std::wcout << L"Description: " << currentRoom->description << std::endl;
    }
  }
};


/*
World build() {
  World mansion;
  mansion.addRoom(L"Foyer");
  mansion.getRoom(L"Foyer")->description =
      L"A grand entrance with a large chandelier and a red carpet leading "
      L"inward.";

  mansion.addRoom(L"Main Hall");
  mansion.getRoom(L"Main Hall")->description =
      L"A vast hall with a grand staircase and doors leading to various rooms.";

  mansion.addRoom(L"Dining Room");
  mansion.getRoom(L"Dining Room")->description =
      L"An opulent room with a long table set with silverware and a grand "
      L"fireplace.";

  mansion.addRoom(L"Kitchen");
  mansion.getRoom(L"Kitchen")->description =
      L"A well-stocked kitchen with polished counters and the scent of old "
      L"spices.";

  mansion.addRoom(L"Library");
  mansion.getRoom(L"Library")->description =
      L"Shelves packed with ancient books and a cozy reading nook by the "
      L"fireplace.";

  mansion.addRoom(L"Study");
  mansion.getRoom(L"Study")->description =
      L"A dark wooden study filled with dusty tomes and an old writing desk.";

  mansion.addRoom(L"Ballroom");
  mansion.getRoom(L"Ballroom")->description =
      L"A vast room with a shining floor, an empty stage, and a grand "
      L"chandelier.";

  mansion.addRoom(L"Conservatory");
  mansion.getRoom(L"Conservatory")->description =
      L"A glass-domed room filled with exotic plants and the scent of fresh "
      L"flowers.";

  mansion.addRoom(L"Guest Room");
  mansion.getRoom(L"Guest Room")->description =
      L"A well-decorated room with a neatly made bed and a nightstand.";

  mansion.addRoom(L"Master Bedroom");
  mansion.getRoom(L"Master Bedroom")->description =
      L"A lavishly furnished bedroom with an enormous four-poster bed and a "
      L"balcony view.";

  mansion.addRoom(L"Bathroom");
  mansion.getRoom(L"Bathroom")->description =
      L"A marble-tiled bathroom with a large bathtub and golden faucets.";

  mansion.addRoom(L"Attic");
  mansion.getRoom(L"Attic")->description =
      L"A dusty attic filled with old trunks, cobwebs, and forgotten memories.";

  mansion.addRoom(L"Basement");
  mansion.getRoom(L"Basement")->description =
      L"A dark, damp basement with a lingering musty smell.";

  mansion.addRoom(L"Wine Cellar");
  mansion.getRoom(L"Wine Cellar")->description =
      L"A chilly cellar lined with racks of expensive wines, some covered in "
      L"dust.";

  mansion.addRoom(L"Servant's Quarters");
  mansion.getRoom(L"Servant's Quarters")->description =
      L"A modest living space with simple beds and an old wooden table.";

  mansion.addRoom(L"Secret Passage");
  mansion.getRoom(L"Secret Passage")->description =
      L"A narrow, dimly lit tunnel hidden behind a bookshelf.";

  mansion.addRoom(L"Hidden Chamber");
  mansion.getRoom(L"Hidden Chamber")->description =
      L"A mysterious chamber filled with ancient artifacts and cryptic symbols "
      L"on the walls.";

  mansion.addRoom(L"Garden");
  mansion.getRoom(L"Garden")->description =
      L"A lush garden with overgrown ivy and a central stone fountain.";

  mansion.addRoom(L"Garage");
  mansion.getRoom(L"Garage")->description =
      L"A dusty garage housing vintage cars covered in white sheets.";

  // Connecting rooms
  mansion.connectRooms(L"Foyer", L"north", L"Main Hall", L"south");
  mansion.connectRooms(L"Main Hall", L"west", L"Library", L"east");
  mansion.connectRooms(L"Main Hall", L"east", L"Dining Room", L"west");
  mansion.connectRooms(L"Dining Room", L"north", L"Kitchen", L"south");
  mansion.connectRooms(L"Library", L"north", L"Study", L"south");
  mansion.connectRooms(L"Main Hall", L"north", L"Ballroom", L"south");
  mansion.connectRooms(L"Ballroom", L"east", L"Conservatory", L"west");
  mansion.connectRooms(L"Main Hall", L"up", L"Guest Room", L"down");
  mansion.connectRooms(L"Guest Room", L"east", L"Master Bedroom", L"west");
  mansion.connectRooms(L"Master Bedroom", L"north", L"Bathroom", L"south");
  mansion.connectRooms(L"Attic", L"down", L"Master Bedroom", L"up");
  mansion.connectRooms(L"Foyer", L"down", L"Basement", L"up");
  mansion.connectRooms(L"Basement", L"east", L"Wine Cellar", L"west");
  mansion.connectRooms(L"Basement", L"west", L"Servant's Quarters", L"east");
  mansion.connectRooms(L"Servant's Quarters", L"north", L"Secret Passage",
                       L"south");
  mansion.connectRooms(L"Secret Passage", L"east", L"Hidden Chamber", L"west");
  mansion.connectRooms(L"Ballroom", L"north", L"Garden", L"south");
  mansion.connectRooms(L"Garage", L"west", L"Garden", L"east");

  // Setting starting location
  mansion.setStartingRoom(L"Foyer");
  return mansion;
}
*/
void saveWorldToFile(const World &world, const std::string &filename) {
  // Check if current room exists
  if (!world.currentRoom) {
    std::wcerr << L"Error: Cannot save - current room is null\n";
    return;
  }

  // Open file for writing
  FILE *file = nullptr;
  fopen_s(&file, filename.c_str(), "wb"); // Open in binary mode
  if (!file) {
    std::wcerr << L"Error: Could not open file for writing: "
               << filename.c_str() << L"\n";
    return;
  }

  // Write UTF-8 BOM
  const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
  fwrite(bom, sizeof(unsigned char), 3, file);

  // Helper function to write wide string to file as UTF-8
  auto writeUtf8Line = [file](const std::wstring &wstr) {
    if (wstr.empty()) {
      fwrite("\n", 1, 1, file);
      return;
    }

    // Get required buffer size
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0,
                                   nullptr, nullptr);
    if (size <= 0) {
      std::wcerr << L"Error converting wstring to UTF-8\n";
      return;
    }

    // Convert to UTF-8
    std::vector<char> buffer(size);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), size,
                        nullptr, nullptr);

    // Write to file and add newline
    fwrite(buffer.data(), sizeof(char), size - 1,
           file); // -1 to exclude null terminator
    fwrite("\n", 1, 1, file);
  };

  // Save current room
  writeUtf8Line(world.currentRoom->name);

  // Save player inventory
  writeUtf8Line(L"PLAYER_INVENTORY");
  for (const auto &item : world.player.inventory) {
    writeUtf8Line(item.name + L"$" + item.description);
  }
  writeUtf8Line(L""); // Empty line to mark end of section

  // Save rooms
  writeUtf8Line(L"ROOMS");
  for (const auto &[name, room] : world.rooms) {
    if (!room)
      continue; // Skip null rooms

    writeUtf8Line(name + L"$" + room->description);

    // Save room inventory
    writeUtf8Line(L"ITEMS");
    for (const auto &item : room->inventory) {
      writeUtf8Line(item.name + L"$" + item.description);
    }
    writeUtf8Line(L"END_ITEMS");
  }
  writeUtf8Line(L""); // Empty line to mark end of section

  // Save connections
  writeUtf8Line(L"CONNECTIONS");
  for (const auto &[name, room] : world.rooms) {
    if (!room)
      continue; // Skip null rooms

    for (const auto &[direction, connectedRoom] : room->connections) {
      if (!connectedRoom)
        continue; // Skip null connected rooms
      writeUtf8Line(name + L"%" + direction + L"%" + connectedRoom->name);
    }
  }

  fclose(file);
  std::wcout << L"World saved successfully to " << filename.c_str() << L"\n";
}

void loadWorldFromFile(World &world, const std::string &filename) {
  // Clear existing world data
  world.rooms.clear();
  world.player.inventory.clear();
  world.currentRoom = nullptr;

  // Open file for reading
  FILE *file = nullptr;
  fopen_s(&file, filename.c_str(), "rb"); // Open in binary mode
  if (!file) {
    std::wcerr << L"Error: Could not open file for reading: "
               << filename.c_str() << L"\n";
    return;
  }

  // Check for UTF-8 BOM and skip if present
  unsigned char bom[3];
  fread(bom, sizeof(unsigned char), 3, file);
  if (!(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
    // Not a BOM, rewind to start of file
    fseek(file, 0, SEEK_SET);
  }

  // Helper function to read a line as UTF-8 and convert to wstring
  auto readUtf8Line = [file]() -> std::wstring {
    std::string line;
    char c;
    while (fread(&c, 1, 1, file) == 1 && c != '\n' && c != '\r') {
      line += c;
    }

    // Handle CRLF
    if (c == '\r') {
      fread(&c, 1, 1, file);
      if (c != '\n') {
        fseek(file, -1, SEEK_CUR); // Push back if not LF
      }
    }

    if (line.empty())
      return L"";

    // Convert UTF-8 to wstring
    int size = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, nullptr, 0);
    if (size <= 0) {
      std::wcerr << L"Error converting UTF-8 to wstring\n";
      return L"";
    }

    std::vector<wchar_t> buffer(size);
    MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, buffer.data(), size);
    return std::wstring(buffer.data());
  };

  // Read starting room
  std::wstring startingRoomName = readUtf8Line();
  if (startingRoomName.empty()) {
    std::wcerr << L"Error: Could not read starting room name\n";
    fclose(file);
    return;
  }

  std::wstring line, section;
  std::shared_ptr<Room> currentRoom = nullptr;

  while (!feof(file)) {
    line = readUtf8Line();

    // Handle empty lines
    if (line.empty())
      continue;

    // Check for section markers
    if (line == L"PLAYER_INVENTORY") {
      section = L"player_inventory";
      continue;
    } else if (line == L"ROOMS") {
      section = L"rooms";
      continue;
    } else if (line == L"ITEMS") {
      section = L"items";
      continue;
    } else if (line == L"END_ITEMS") {
      section = L"rooms";
      continue;
    } else if (line == L"CONNECTIONS") {
      section = L"connections";
      continue;
    }

    // Process line based on current section
    if (section == L"player_inventory") {
      size_t pos = line.find(L'$');
      if (pos != std::wstring::npos) {
        std::wstring name = line.substr(0, pos);
        std::wstring desc = line.substr(pos + 1);
        world.player.inventory.push_back({name, desc});
      }
    } else if (section == L"rooms") {
      size_t pos = line.find(L'$');
      if (pos != std::wstring::npos) {
        std::wstring name = line.substr(0, pos);
        std::wstring desc = line.substr(pos + 1);

        world.addRoom(name);
        auto room = world.getRoom(name);
        if (room) {
          room->description = desc;
          currentRoom = room; // Set current room for item loading
        }
      }
    } else if (section == L"items" && currentRoom) {
      size_t pos = line.find(L'$');
      if (pos != std::wstring::npos) {
        std::wstring name = line.substr(0, pos);
        std::wstring desc = line.substr(pos + 1);
        currentRoom->inventory.push_back({name, desc});
      }
    } else if (section == L"connections") {
      // Parse connection (room1%direction%room2)
      size_t pos1 = line.find(L'%');
      if (pos1 != std::wstring::npos) {
        size_t pos2 = line.find(L'%', pos1 + 1);
        if (pos2 != std::wstring::npos) {
          std::wstring room1Name = line.substr(0, pos1);
          std::wstring direction = line.substr(pos1 + 1, pos2 - pos1 - 1);
          std::wstring room2Name = line.substr(pos2 + 1);

          // Get the rooms
          auto room1 = world.getRoom(room1Name);
          auto room2 = world.getRoom(room2Name);

          // Connect the rooms (only if both exist)
          if (room1 && room2) {
            room1->connectRoom(direction, room2.get());
          }
        }
      }
    }
  }

  fclose(file);

  // Set starting room
  auto startRoom = world.getRoom(startingRoomName);
  if (startRoom) {
    world.currentRoom = startRoom.get();
  } else {
    std::wcerr << L"Error: Starting room '" << startingRoomName
               << L"' not found\n";
    // Set some default room if available
    if (!world.rooms.empty()) {
      world.currentRoom = world.rooms.begin()->second.get();
      std::wcerr << L"Using '" << world.currentRoom->name
                 << L"' as starting room instead\n";
    }
  }

  // Final verification
  if (!world.currentRoom) {
    std::wcerr << L"Error: No valid starting room could be set!\n";
  } else {
    std::wcout << L"World loaded successfully from " << filename.c_str()
               << L"\n";
  }
}

int main() {
  std::wcout << L"starting...";
  std::wstring credits = L"--under construction--";
  World world;

  // world;

  /*saveWorldToFile(world, "map.map");*/

  loadWorldFromFile(world, "map.map");

  _setmode(_fileno(stdout), _O_U16TEXT);
  std::wstring UIPT;

  // SetConsoleCP(CP_UTF8);
  // SetConsoleOutputCP(CP_UTF8);
  // setlocale(LC_ALL, ".UTF8");
  // std::wcout.imbue(std::locale("en_US.UTF-8"));

  // finish
  // initialization-------------------------------------------------------------------------------------------------
title_screen:

  system("cls"); // clear

  std::wcout << "(1) start\n";
  std::wcout << "(2) credits\n";
  std::wcout << "(3) exit\n";

  switch (utils::getNumber(L"enter a number: ", 3, 1)) {
  case 1: {
    system("cls"); // clear
    break;
  }
  case 2: {
    std::wcout << credits;
    Sleep(1000);
    goto title_screen;
    break;
  }
  case 3: {
    goto end;
  }
  };

  world.player.state = WANDERING;

  while (true) {
    if (world.player.state == WANDERING) {
      system("cls");
      std::wcout << L"stats:\n";
      std::wcout << L"health: [" << world.player.health << L"/"
                 << world.player.max_health << L"]\n\n";
      std::wcout << world.currentRoom->name << L"\n";
      std::wcout << world.currentRoom->description << L"\n\n";
      UIPT = utils::toUpperCase(utils::getString(L"> "));
      if (UIPT.substr(0, 3) == L"GO ") {
        PlaySound("whoosh-6316.wav", NULL, SND_FILENAME | SND_ASYNC);
        world.move(utils::toLowerCase(UIPT).substr(3));
      }
      if (UIPT == L"LOOK") {
        std::wcout << L"There are " << world.currentRoom->inventory.size()
                   << " items in this room:\n";
        for (Item i : world.currentRoom->inventory) {
          std::wcout << i.name << L"\n";
          Sleep(10);
        }
        Sleep(1000);
      }
      if (UIPT == L"TAKE") {
        std::wcout << L"You take the "
                   << world.currentRoom->inventory.back().name << L"\n";
        Sleep(1000);
        world.player.inventory.push_back(world.currentRoom->inventory.back());
        world.currentRoom->inventory.pop_back();
      }
      if (UIPT.substr(0, 5) == L"OPEN ") {
        if (UIPT.substr(5) == L"BACKPACK") {
          system("cls");
          std::wcout << L"BACKPACK: \n";
          for (Item i : world.player.inventory) {
            std::wcout << i.name << L"\n";
          }
          world.player.state = INVENTORY;
        }
      }
    } else if (world.player.state == INVENTORY) {
      UIPT = utils::toUpperCase(utils::getString(L"i> "));
      if (UIPT == L"CLOSE") {
        world.player.state = WANDERING;
      } else {
        for (Item i : world.player.inventory) {
          if (UIPT == utils::toUpperCase(i.name)) {
            std::wcout << i.description << L"\n";
            break;
          }
        }
      }
    }
  }

end:
  return 0;
}
