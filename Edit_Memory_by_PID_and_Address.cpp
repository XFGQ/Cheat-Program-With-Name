#include <windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <iomanip>

struct AddressWrite {
    uintptr_t address;
    int value;
};

// Hex string → integer dönüşüm
uintptr_t HexStringToAddress(const std::string& hexStr) {
    uintptr_t addr = 0;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> addr;
    return addr;
}

int main() {
    DWORD pid;
    std::cout << "[?] Enter PID: ";
    std::cin >> pid;
    std::cin.ignore(); // Flush newline

    std::string input;
    std::cout << "[?] Enter addresses (comma separated, e.g. 0x61, 0x62, 0x63): ";
    std::getline(std::cin, input);

    std::vector<AddressWrite> toWrite;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::string trimmed;
        for (char c : token) if (!isspace(c)) trimmed += c;
        uintptr_t addr = HexStringToAddress(trimmed);
        int val;
        std::cout << "[?] Value to write to address " << trimmed << ": ";
        std::cin >> val;
        toWrite.push_back({ addr, val });
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[-] Failed to open process.\n";
        return 1;
    }

    for (const auto& item : toWrite) {
        SIZE_T written;
        if (WriteProcessMemory(hProcess, (LPVOID)item.address, &item.value, sizeof(int), &written)) {
            std::cout << "[+] Wrote " << item.value << " to 0x" << std::hex << item.address << "\n";
        }
        else {
            std::cerr << "[-] Failed to write to 0x" << std::hex << item.address << "\n";
        }
    }

    CloseHandle(hProcess);
    return 0;
}
