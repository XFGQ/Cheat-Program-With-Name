#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

struct MemoryAddress {
    uintptr_t address;
    int oldValue;
    int newValue;
};

//Find the PID (Process ID)
DWORD GetProcessID(const std::wstring& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (processName == entry.szExeFile) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
    }
    CloseHandle(snapshot);
    return pid;
}

//Search for a number in the RAM
std::vector<MemoryAddress> ScanForValue(HANDLE hProcess, int target) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    std::vector<MemoryAddress> results;

    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

    MEMORY_BASIC_INFORMATION mbi;
    while (addr < maxAddr) {
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi)) == sizeof(mbi) &&
            (mbi.State == MEM_COMMIT) && (mbi.Protect & PAGE_READWRITE)) {

            std::vector<BYTE> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < bytesRead - sizeof(int); i++) {
                    int val = *(int*)&buffer[i];
                    if (val == target) {
                        results.push_back({ addr + i, val, val });
                    }
                }
            }
        }
        addr += mbi.RegionSize;
    }
    return results;
}

//Filter the results with new value
std::vector<MemoryAddress> FilterAddresses(HANDLE hProcess, const std::vector<MemoryAddress>& prev, int newVal) {
    std::vector<MemoryAddress> updated;
    for (const auto& mem : prev) {
        int currentVal = 0;
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, (LPCVOID)mem.address, &currentVal, sizeof(int), &bytesRead)) {
            if (currentVal == newVal) {
                updated.push_back({ mem.address, mem.oldValue, currentVal });
                std::cout << "[*] Match: 0x" << std::hex << mem.address << " | Old: " << mem.oldValue << " -> New: " << currentVal << "\n";
            }
        }
    }
    return updated;
}

//Write a new value to memory addresses
void WriteToAddresses(HANDLE hProcess, const std::vector<MemoryAddress>& addresses, int newVal) {
    for (const auto& mem : addresses) {
        SIZE_T bytesWritten;
        WriteProcessMemory(hProcess, (LPVOID)mem.address, &newVal, sizeof(int), &bytesWritten);
        std::cout << "[+] Wrote value to 0x" << std::hex << mem.address << "\n";
    }
}

int main() {
    std::wstring exeName;
    std::wcout << L"[?] Type process name (like game.exe): ";
    std::getline(std::wcin, exeName);

    DWORD pid = GetProcessID(exeName);
    if (!pid) {
        std::cerr << "[-] Cannot find process.\n";
        return 1;
    }
    std::wcout << L"[+] Process ID is: " << pid << "\n";

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[-] Cannot open the process.\n";
        return 1;
    }

    std::vector<MemoryAddress> currentResults;
    while (true) {
        int val;
        std::cout << "\n[?] Type value to search (-1 to stop): ";
        std::cin >> val;
        if (val == -1) break;

        if (currentResults.empty()) {
            currentResults = ScanForValue(hProcess, val);
            std::cout << "[*] Found " << currentResults.size() << " results.\n";
        }
        else {
            currentResults = FilterAddresses(hProcess, currentResults, val);
            std::cout << "[*] Now " << currentResults.size() << " addresses match.\n";
        }

        if (currentResults.size() <= 3 && !currentResults.empty()) {
            std::cout << "\n[!] Final matches found. Ready to change value.\n";
            int newValue;
            std::cout << "[?] Type new value to write: ";
            std::cin >> newValue;
            WriteToAddresses(hProcess, currentResults, newValue);
            break;
        }

        if (currentResults.empty()) {
            std::cout << "[-] No matches found. Starting again.\n";
            break;
        }
    }

    CloseHandle(hProcess);
    return 0;
}
