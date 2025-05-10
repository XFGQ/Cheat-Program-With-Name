# RAM Value Scanner & Modifier (C++)

This is a C++ tool that scans the RAM of a running process, finds addresses that hold a specific value, and allows the user to modify those values.  
It is intended for educational and academic use only.

---

- [What is this project?](#what-is-this-project)
- [Disclaimer](#disclaimer)
- [What do you need?](#what-do-you-need)
- [How to run on Windows](#how-to-run-on-windows)
- [How to run on Linux](#how-to-run-on-linux)
- [How it works](#how-it-works)
- [Author](#author)

---

## What is this project?

This program allows you to:

- Enter the name of a running program (e.g., `game.exe`)
- Enter the value currently visible on-screen
- Find all RAM addresses holding that value
- Narrow down the correct address by filtering with new values
- Change the value at those addresses

It can be used to understand how RAM works and how values are stored in memory at runtime.

---

## Disclaimer

This tool is developed for **educational purposes and school projects only**.  
**The author is not responsible for any misuse of this tool.**  
Using this program on software you do not own may violate laws or terms of service.

---

## What do you need?

| Platform | Tools Required |
|----------|----------------|
| Windows  | Visual Studio (C++) or any compiler with `Windows.h` support |
| Linux    | g++ and root permissions (for `ptrace`) |

---

## How to run on Windows

1. Open the project in Visual Studio or any C++ IDE.
2. Build the project.
3. Run the program as Administrator.
4. When asked:
   - Enter the **process name**, e.g., `game.exe`
   - Enter the **current on-screen value**, e.g., `15`
5. Follow prompts to filter and modify values.

---

## How to run on Linux

> Requires `sudo` to access process memory

1. Make sure `g++` is installed:
```bash
sudo apt update
sudo apt install g++
