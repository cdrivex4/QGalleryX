#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>


// Simple watchdog wrapper for running over network shares where .ps1 is blocked
// or unreliable. Launches appScrollBench.exe and holds the terminal open on
// failure.

void dumpCrashLog() {
  std::ifstream logFile("logs/crash.log");
  if (logFile.is_open()) {
    std::cout << "\n--- CONTENT OF logs/crash.log ---" << std::endl;
    std::string line;
    while (std::getline(logFile, line)) {
      std::cout << line << std::endl;
    }
    std::cout << "--- END OF LOG ---\n" << std::endl;
  } else {
    std::cout << "\nNote: No logs/crash.log found. The crash might have been "
                 "too early (DLL missing?)."
              << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "===================================================="
            << std::endl;
  std::cout << "   NetappScrollBench - Diagnostics Watchdog v1.0.7"
            << std::endl;
  std::cout << "===================================================="
            << std::endl;

  // Check if appScrollBench.exe exists in the same directory
  DWORD dwAttrib = GetFileAttributesA("appScrollBench.exe");
  if (dwAttrib == INVALID_FILE_ATTRIBUTES ||
      (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
    std::cerr << "ERROR: appScrollBench.exe not found in current directory!"
              << std::endl;
    std::cerr
        << "Please ensure this watchdog is in the same folder as the main app."
        << std::endl;
    std::system("pause");
    return 1;
  }

  // Build command line: pass through any arguments
  std::string cmdLine = "appScrollBench.exe";
  for (int i = 1; i < argc; ++i) {
    cmdLine += " \"";
    cmdLine += argv[i];
    cmdLine += "\"";
  }

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  std::cout << "[Watchdog] Starting process: " << cmdLine << std::endl;

  // Create the process
  if (!CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL,
                      NULL, &si, &pi)) {
    std::cerr << "[Watchdog] FAILED to launch process. Win32 Error: "
              << GetLastError() << std::endl;
    std::system("pause");
    return 1;
  }

  // Wait for the process to exit
  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  std::cout << "===================================================="
            << std::endl;
  if (exitCode == 0) {
    std::cout << "[Watchdog] Application exited normally." << std::endl;
  } else {
    std::cout << "[Watchdog] DETECTED CRASH OR ERROR (Exit Code: 0x" << std::hex
              << exitCode << std::dec << ")" << std::endl;

    // Attempt to dump logs
    dumpCrashLog();

    std::cout
        << "\n[Watchdog] Terminating watchdog. Holding terminal for review..."
        << std::endl;
    std::system("pause");
  }

  return (int)exitCode;
}
