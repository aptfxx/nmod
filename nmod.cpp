
// nmood

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>
#include <thread>
#include <chrono>

// map is not actually used anywhere but i'm scared to remove it
namespace fs = std::filesystem;

#ifdef _WIN32
  #include <windows.h>
  #define PLATFORM_WINDOWS
#else
  #include <unistd.h>
  #include <pwd.h>
  #define PLATFORM_LINUX
#endif

namespace col {
  const std::string reset  = "\033[0m";
  const std::string cyan   = "\033[96m";
  const std::string yellow = "\033[93m";
  const std::string green  = "\033[92m";
  const std::string red    = "\033[91m";
  const std::string grey   = "\033[90m";
}

// tries a few different ways to find home. if all else fails, /tmp.
// hopefully it doesn't come to that or else uhhhh
fs::path getHomeDir() {
#ifdef PLATFORM_WINDOWS
  const char* home = std::getenv("USERPROFILE");
  if (home) return fs::path(home);
  return fs::path("C:/Users/User"); // last resort. sorry if your username isn't User
#else
  const char* home = std::getenv("HOME");
  if (home) return fs::path(home);
  struct passwd* pw = getpwuid(getuid());
  if (pw) return fs::path(pw->pw_dir);
  return fs::path("/tmp"); // giving up
#endif
}

fs::path getMinecraftModsDir() {
#ifdef PLATFORM_WINDOWS
  const char* appdata = std::getenv("APPDATA");
  if (appdata) return fs::path(appdata) / ".minecraft" / "mods";
  return getHomeDir() / "AppData" / "Roaming" / ".minecraft" / "mods"; // fallback, probably wrong
#else
  return getHomeDir() / ".minecraft" / "mods";
#endif
}

fs::path getBackupRoot() {
  // least elegant thing ive written all dey
  return getHomeDir() / "nmodbackups";
}

void clearScreen() {
  // yes, system("clear") is bad practice. yes, i'm doing it anyway
#ifdef PLATFORM_WINDOWS
  system("cls");
#else
  system("clear");
#endif
}

void waitForEnter() {
  std::cout << "\nPress Enter to go back...";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getInput(const std::string& prompt) {
  std::cout << prompt;
  std::string input;
  std::getline(std::cin, input);
  return input;
}

// strips whitespace from both ends. does this need a comment? probably not
std::string trim(const std::string& s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  size_t end   = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  return s.substr(start, end - start + 1);
}

// i know there's probably a better way to do this
bool iequals(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  for (int i = 0; i < (int)a.size(); i++)
    if (tolower(a[i]) != tolower(b[i])) return false;
  return true;
}

// iterates the whole directory just to count items. slightly stupid, i know
int countItems(const fs::path& dir) {
  int n = 0;
  for (const auto& item : fs::directory_iterator(dir)) n++;
  return n;
}

bool copyDir(const fs::path& src, const fs::path& dst) {
  try {
    fs::create_directories(dst);
    for (const auto& item : fs::recursive_directory_iterator(src)) {
      fs::path relative = fs::relative(item.path(), src);
      fs::path target   = dst / relative;
      if (item.is_directory()) fs::create_directories(target);
      else fs::copy_file(item.path(), target, fs::copy_options::overwrite_existing);
    }
    return true;
  } catch (const std::exception& e) {
    //  get more disk space (or get a better disk idk)
    std::cerr << col::red << "Copy error: " << e.what() << col::reset << "\n";
    return false;
  }
}

// nukes everything inside the folder but keeps the folder itself
void clearDir(const fs::path& dir) {
  for (const auto& item : fs::directory_iterator(dir))
    fs::remove_all(item.path());
}

void printBanner() {
  // ascii art generated online. it's the letters N M O D if you squint (title card drop
  std::cout << col::cyan;
  std::cout << "\n";
  std::cout << "    _   ____  _______  ____ \n";
  std::cout << "   / | / /  |/  / __ \\/ __ \\\n";
  std::cout << "  /  |/ / /|_/ / / / / / / /\n";
  std::cout << " / /|  / /  / / /_/ / /_/ / \n";
  std::cout << "/_/ |_/_/  /_/\\____/_____/  \n";
  std::cout << col::reset;
  std::cout << "                                           - Nmod v1\n";
  std::cout << "\n";
}

struct NmodContext {
  std::string ver;
  std::string name;
  std::string tech;
  bool valid = false; 
};

// pulls the value out of lines like  VER:"1.20.1";
std::string extractValue(const std::string& line) {
  size_t open  = line.find('"');
  size_t close = line.rfind('"');
  if (open == std::string::npos || close == std::string::npos || open == close)
    return "";
  return line.substr(open + 1, close - open - 1);
}

NmodContext parseContext(const fs::path& contextFile) {
  NmodContext ctx;
  std::ifstream f(contextFile);
  if (!f.is_open()) return ctx;

  std::string content((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());

  auto toUpper = [](std::string s) {
    for (int i = 0; i < (int)s.size(); i++) s[i] = (char)toupper((unsigned char)s[i]);
    return s;
  };

  std::istringstream ss(content);
  std::string line;
  while (std::getline(ss, line)) {
    std::string up = toUpper(trim(line));
    if (up.find("VER:")  != std::string::npos) ctx.ver  = extractValue(line);
    if (up.find("NAME:") != std::string::npos) ctx.name = extractValue(line);
    if (up.find("TECH:") != std::string::npos) ctx.tech = extractValue(line);
  }

  ctx.valid = !ctx.ver.empty() || !ctx.name.empty() || !ctx.tech.empty();
  return ctx;
}

// just returns a string reminding the user to install the right loader.
// probably obvious but people forget
std::string loaderReminder(const std::string& tech, const std::string& ver) {
  std::string t = tech;
  for (int i = 0; i < (int)t.size(); i++) t[i] = (char)toupper((unsigned char)t[i]);

  if (t == "FORGE")    return "Make sure Forge for "    + ver + " is installed!";
  if (t == "FABRIC")   return "Make sure Fabric for "   + ver + " is installed!";
  if (t == "NEOFORGE") return "Make sure NeoForge for " + ver + " is installed!";
  if (t == "QUILT")    return "Make sure Quilt for "    + ver + " is installed!";
  return "Make sure " + tech + " for " + ver + " is installed!"; 
}

void printContext(const NmodContext& ctx) {
  if (!ctx.ver.empty())
    std::cout << "Minecraft Version : " << col::yellow << ctx.ver << col::reset << "\n";
  if (!ctx.tech.empty())
    std::cout << "Technology        : " << col::yellow << ctx.tech << col::reset << "\n";
  if (!ctx.name.empty())
    std::cout << "Modpack Name      : " << col::cyan << ctx.name << col::reset << "\n";
  if (!ctx.tech.empty() && !ctx.ver.empty())
    std::cout << col::yellow << loaderReminder(ctx.tech, ctx.ver) << col::reset << "\n";
  std::cout << "\n";
}

void createContext() {
  clearScreen();
  printBanner();
  std::cout << "Create context.nmod\n";
  std::cout << "──────────────────────────────────\n\n";

  std::string ver  = trim(getInput("Minecraft version (e.g. 1.20.1): "));
  std::string name = trim(getInput("Modpack name: "));
  std::string tech = trim(getInput("Mod technology (Forge / Fabric / NeoForge / Quilt / etc): "));

  if (ver.empty() || name.empty() || tech.empty()) {
    std::cout << col::red << "\nAll fields are required. context.nmod was not created." << col::reset << "\n";
    waitForEnter();
    return;
  }

  fs::path outPath = fs::current_path() / "context.nmod";

  std::ofstream f(outPath);
  if (!f.is_open()) {
    std::cout << col::red << "Failed to write context.nmod." << col::reset << "\n";
    waitForEnter();
    return;
  }

  // the format is a bit silly but it's mine and i like it
  f << "nmod{\n";
  f << "     VER:\"" << ver << "\";\n";
  f << "     NAME:\"" << name << "\";\n";
  f << "     TECH:\"" << tech << "\";\n";
  f << "}\n";
  f.close();

  std::cout << col::green << "\ncontext.nmod created successfully at:\n" << col::reset << "  " << outPath.string() << "\n";
  waitForEnter();
}

void clearCache() {
  clearScreen();
  printBanner();
  std::cout << col::red << "WARNING: clearcache\n" << col::reset;
  std::cout << "──────────────────────────────────\n";
  std::cout << "This will permanently delete:\n";
  std::cout << "  - All backups in ~/nmodbackups/\n";
  std::cout << "  - context.nmod in the current directory\n\n";

  // making them type the full word so they can't accidentally nuke everything
  std::string confirm = trim(getInput("Type 'yes' to confirm: "));
  if (!iequals(confirm, "yes")) {
    std::cout << col::yellow << "Cancelled." << col::reset << "\n";
    waitForEnter();
    return;
  }

  fs::path backupRoot = getBackupRoot();
  if (fs::exists(backupRoot)) {
    fs::remove_all(backupRoot); // goodbye forever
    std::cout << col::green << "Backups cleared." << col::reset << "\n";
  } else {
    std::cout << col::grey << "No backup folder found — skipping." << col::reset << "\n";
  }

  fs::path ctx = fs::current_path() / "context.nmod";
  if (fs::exists(ctx)) {
    fs::remove(ctx);
    std::cout << col::green << "context.nmod removed." << col::reset << "\n";
  } else {
    std::cout << col::grey << "No context.nmod found — skipping." << col::reset << "\n";
  }

  std::cout << col::green << "\nCache cleared. Restarting...\n" << col::reset;
  std::this_thread::sleep_for(std::chrono::milliseconds(1200)); // small pause so it feels intentional
}

void listMods(const fs::path& nmodsDir) {
  clearScreen();
  printBanner();
  std::cout << "Contents of nmods/\n";
  std::cout << "──────────────────────────────────\n";
  int n = 0;
  for (const auto& item : fs::directory_iterator(nmodsDir)) {
    n++;
    std::cout << "  [" << n << "] " << item.path().filename().string() << "\n";
  }
  std::cout << "──────────────────────────────────\n";
  std::cout << "  Total: " << n << " item(s)\n";
  waitForEnter();
}

void installMods(const fs::path& nmodsDir) {
  clearScreen();
  printBanner();
  std::cout << "Installing new NMOD mods will remove all currently installed mods.\n\n";

  std::string backupChoice = trim(getInput("Would you like to make a backup? (Y/N): "));

  if (iequals(backupChoice, "y") || iequals(backupChoice, "yes")) {
    std::string backupName = trim(getInput("Enter a name for this backup: "));
    if (backupName.empty()) {
      std::cout << col::red << "Backup name cannot be empty." << col::reset << "\n";
      waitForEnter();
      return;
    }

    fs::path backupDst = getBackupRoot() / backupName;
    fs::path modsDir   = getMinecraftModsDir();

    std::cout << "\nCreating backup at: " << backupDst.string() << "\n";

    if (!fs::exists(modsDir)) {
      // no mods folder means nothing to back up, which is fine
      std::cout << col::yellow << "No mods folder found, skipping backup (nothing to back up)." << col::reset << "\n";
    } else {
      if (!copyDir(modsDir, backupDst)) {
        std::cout << col::red << "Backup failed." << col::reset << "\n";
        waitForEnter();
        return;
      }
      std::cout << col::green << "Backup '" << backupName << "' saved successfully." << col::reset << "\n\n";
    }
  }

  // re-fetch modsDir here because the backup block above has its own scope.
  // a bit redundant but it keeps things readable
  fs::path modsDir = getMinecraftModsDir();
  std::cout << "Clearing current mods...\n";
  if (!fs::exists(modsDir)) fs::create_directories(modsDir);
  else clearDir(modsDir);

  std::cout << "Installing mods from nmods/...\n";
  if (!copyDir(nmodsDir, modsDir)) {
    std::cout << col::red << "Installation failed." << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::cout << col::green << "Mods installed successfully!" << col::reset << "\n";
  waitForEnter();
}

void restoreMods() {
  clearScreen();
  printBanner();

  fs::path backupRoot = getBackupRoot();

  if (!fs::exists(backupRoot)) {
    std::cout << col::red << "No backups found. '" << backupRoot.string() << "' does not exist." << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::vector<fs::path> backups;
  for (const auto& item : fs::directory_iterator(backupRoot))
    if (item.is_directory()) backups.push_back(item.path());
  std::sort(backups.begin(), backups.end()); 

  if (backups.empty()) {
    std::cout << col::yellow << "No backup folders found." << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::cout << "Available backups:\n";
  std::cout << "──────────────────────────────────\n";
  for (int i = 0; i < (int)backups.size(); i++)
    std::cout << "  [" << (i + 1) << "] " << backups[i].filename().string() << "\n";
  std::cout << "──────────────────────────────────\n\n";

  std::string choice = trim(getInput("Which modpack would you like to load? (enter number): "));

  int picked = 0;
  try { picked = std::stoi(choice); } catch (...) { picked = 0; } // ooooh oohhoooh du bist gut genug (i felt like it fuck off)

  if (picked < 1 || picked > (int)backups.size()) {
    std::cout << col::red << "Invalid selection." << col::reset << "\n";
    waitForEnter();
    return;
  }

  fs::path chosen  = backups[picked - 1];
  fs::path modsDir = getMinecraftModsDir();

  std::cout << "\nRestoring '" << chosen.filename().string() << "' to " << modsDir.string() << "...\n";

  if (!fs::exists(modsDir)) fs::create_directories(modsDir);
  else clearDir(modsDir);

  if (!copyDir(chosen, modsDir)) {
    std::cout << col::red << "Restore failed." << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::cout << col::green << "'" << chosen.filename().string() << "' restored successfully!" << col::reset << "\n";
  waitForEnter();
}

int main() {
#ifdef PLATFORM_WINDOWS
  SetConsoleOutputCP(CP_UTF8);
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hOut, &mode);
  SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

  fs::path nmodsDir = fs::current_path() / "nmods";

  while (true) {
    clearScreen();
    printBanner();

    if (!fs::exists(nmodsDir) || !fs::is_directory(nmodsDir)) {

      std::cout << col::red << "Error: No 'nmods' folder found in the current directory." << col::reset << "\n";
      std::cout << "Make sure you run this from the folder containing 'nmods'.\n\n";
      waitForEnter();
      return 1;
    }

    fs::path contextFile = fs::current_path() / "context.nmod";
    if (fs::exists(contextFile)) {
      NmodContext ctx = parseContext(contextFile);
      if (ctx.valid) printContext(ctx);
    }

    int count = countItems(nmodsDir);
    std::cout << count << " Items Found\n";
    std::cout << "  Install  |  List  |  Restore\n\n";

    std::string choice = trim(getInput("Choose an option: "));

    if      (iequals(choice, "i") || iequals(choice, "install"))   installMods(nmodsDir);
    else if (iequals(choice, "l") || iequals(choice, "list"))       listMods(nmodsDir);
    else if (iequals(choice, "r") || iequals(choice, "restore"))    restoreMods();
    else if (iequals(choice, "createcontext"))                       createContext();
    else if (iequals(choice, "clearcache"))                         clearCache(); // hidden command, not shown in menu
    else {
      std::cout << col::red << "Invalid option." << col::reset << "\n";
      waitForEnter();
    }
  }

  return 0; // never actually reached but the compiler likes it
}
