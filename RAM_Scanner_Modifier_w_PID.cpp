#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

//Struct to hold memory address and value changes
struct MemoryAddress {
    uintptr_t address;
    int oldValue;
    int newValue;
};

//Search for a specific value in RAM
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

//Filter matches based on a new value
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

//Write a new value to the matching memory addresses
void WriteToAddresses(HANDLE hProcess, const std::vector<MemoryAddress>& addresses, int newVal) {
    for (const auto& mem : addresses) {
        SIZE_T bytesWritten;
        WriteProcessMemory(hProcess, (LPVOID)mem.address, &newVal, sizeof(int), &bytesWritten);
        std::cout << "[+] Value written to 0x" << std::hex << mem.address << "\n";
    }
}

int main() {
    DWORD pid;
    std::cout << "[?] Enter PID of the target process: ";
    std::cin >> pid;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[-] Failed to open process. Check PID.\n";
        return 1;
    }

    std::vector<MemoryAddress> currentResults;
    while (true) {
        int val;
        std::cout << "\n[?] Enter value to scan (-1 to stop): ";
        std::cin >> val;
        if (val == -1) break;

        if (currentResults.empty()) {
            currentResults = ScanForValue(hProcess, val);
            std::cout << "[*] Found " << currentResults.size() << " matching addresses.\n";
        }
        else {
            currentResults = FilterAddresses(hProcess, currentResults, val);
            std::cout << "[*] Filtered to " << currentResults.size() << " addresses.\n";
        }

        if (currentResults.size() <= 3 && !currentResults.empty()) {
            std::cout << "\n[!] Final candidates found. Ready to patch.\n";
            int newValue;
            std::cout << "[?] Enter value to WRITE: ";
            std::cin >> newValue;
            WriteToAddresses(hProcess, currentResults, newValue);
            break;
        }

        if (currentResults.empty()) {
            std::cout << "[-] No matching addresses left. Restarting scan.\n";
            break;
        }
    }

    CloseHandle(hProcess);
    return 0;
}
