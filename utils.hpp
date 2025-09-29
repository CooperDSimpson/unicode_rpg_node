#include <Windows.h>
#include <chrono>
#include <codecvt>
#include <cstdlib>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <limits>
#include <locale>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <sstream>
#include <type_traits>
#include <vector>
#include <wchar.h>

namespace utils {

template <typename T>
std::basic_string<T> toUpperCase(std::basic_string<T> input) {
  for (T &character : input) {
    character = std::toupper(character, std::locale());
  }
  return input;
}

template <typename T>
std::basic_string<T> toLowerCase(std::basic_string<T> input) {
  for (T &character : input) {
    character = std::tolower(character, std::locale());
  }
  return input;
}

template <typename T>
T getNumber(std::wstring prompt = L"", T max = 10, T min = 1,
            std::wstring reprompt = L"") {
  static_assert(std::is_arithmetic<T>::value,
                "T must be an arithmetic type (int, float, or double)");

  T option = -1;
  std::wstring UIPT;
  std::wcout << prompt;

  while (true) {
    std::wcin >> UIPT;
    try {
      if constexpr (std::is_integral<T>::value) {
        option =
            static_cast<T>(std::stoi(UIPT)); // Convert to int if T is integral
      } else {
        option = static_cast<T>(std::stod(UIPT)); // Convert to float/double
      }

      if (option >= min && option <= max) {
        goto end; // Valid input, exit loop
      } else {
        std::wcout << (reprompt.empty() ? L"Please enter a value between " +
                                              std::to_wstring(min) + L" and " +
                                              std::to_wstring(max) + L": "
                                        : reprompt);
      }
    } catch (const std::invalid_argument &) {
      std::wcout << L"Invalid input. Please enter a valid number: ";
    } catch (const std::out_of_range &) {
      std::wcout
          << L"The number is out of range. Please enter a valid number: ";
    }
  }
end:

  return option;
}

std::string getString(std::string prompt) {
  std::string option = "";
  std::cout << prompt;
  std::getline(std::cin, option);
  return option;
}

std::wstring getString(std::wstring prompt) {
  std::wstring option = L"";
  std::wcout << prompt;
  std::getline(std::wcin, option);
  return option;
}

std::vector<std::wstring> splitStringByNewline(const std::wstring &str) {
  std::vector<std::wstring> result;
  std::wstringstream stream(str);
  std::wstring line;

  while (std::getline(stream, line)) {
    result.push_back(line);
  }

  return result;
}

std::wstring oppositeDirection(std::wstring direction) {
  if (direction == L"north") {
    return L"south";
  } else if (direction == L"south") {
    return L"north";
  } else if (direction == L"east") {
    return L"west";
  } else if (direction == L"west") {
    return L"east";
  } else if (direction == L"up") {
    return L"down";
  } else if (direction == L"down") {
    return L"up";
  } else if (direction == L"inside") {
    return L"outside";
  } else if (direction == L"outside") {
    return L"inside";
  }
}
} // namespace utils
