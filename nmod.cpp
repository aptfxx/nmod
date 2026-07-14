// nmod v1.6
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
#include <iomanip>
#include <cstring>

// map is not actually used anywhere but i'm scared to remove it
namespace fs = std::filesystem;

#ifdef _WIN32
  #include <windows.h>
  #include <wininet.h>
  #pragma comment(lib, "wininet.lib")
  #define PLATFORM_WINDOWS
#else
  #include <unistd.h>
  #include <pwd.h>
  #include <curl/curl.h>
  #define PLATFORM_LINUX
#endif

namespace col {
  const std::string reset  = "\033[0m";
  const std::string cyan   = "\033[96m";
  const std::string yellow = "\033[93m";
  const std::string green  = "\033[92m";
  const std::string red    = "\033[91m";
  const std::string grey   = "\033[90m";
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

// temp folder for web downloads. gets nuked after use
fs::path getWebTempDir() {
  return getHomeDir() / ".nmodtmp";
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
  size_t end   = s.find_last_not_of(" \t\r\n");
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
      fs::path target   = dst / relative;
      if (item.is_directory()) fs::create_directories(target);
      else fs::copy_file(item.path(), target, fs::copy_options::overwrite_existing);
    }
    return true;
  } catch (const std::exception& e) {
    //  get more disk space (or get a better disk idk)
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
  std::cout << "    _   ____  _______  ____ \n";
  std::cout << "   / | / /  |/  / __ \\/ __ \\\n";
  std::cout << "  /  |/ / /|_/ / / / / / / /\n";
  std::cout << " / /|  / /  / / /_/ / /_/ / \n";
  std::cout << "/_/ |_/_/  /_/\\____/_____/  \n";
  std::cout << col::reset;
  std::cout << "                                           - Nmod v1\n";
  std::cout << "\n";
}

struct NmodContext {
  std::string ver;
  std::string name;
  std::string tech;
  bool valid = false; 
};

// pulls the value out of lines like  VER:"1.20.1";
std::string extractValue(const std::string& line) {
  size_t open  = line.find('"');
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
    if (up.find("VER:")  != std::string::npos) ctx.ver  = extractValue(line);
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

  if (t == "FORGE")    return "Make sure Forge for "    + ver + " is installed!";
  if (t == "FABRIC")   return "Make sure Fabric for "   + ver + " is installed!";
  if (t == "NEOFORGE") return "Make sure NeoForge for " + ver + " is installed!";
  if (t == "QUILT")    return "Make sure Quilt for "    + ver + " is installed!";
  return "Make sure " + tech + " for " + ver + " is installed!"; 
}

void printContext(const NmodContext& ctx) {
  if (!ctx.ver.empty())
    std::cout << "Minecraft Version : " << col::yellow << ctx.ver << col::reset << "\n";
  if (!ctx.tech.empty())
    std::cout << "Technology        : " << col::yellow << ctx.tech << col::reset << "\n";
  if (!ctx.name.empty())
    std::cout << "Modpack Name      : " << col::cyan << ctx.name << col::reset << "\n";
  if (!ctx.tech.empty() && !ctx.ver.empty())
    std::cout << col::yellow << loaderReminder(ctx.tech, ctx.ver) << col::reset << "\n";
  std::cout << "\n";
}

void createContext() {
  clearScreen();
  printBanner();
  std::cout << "Create context.nmod\n";
  std::cout << "──────────────────────────────────\n\n";

  std::string ver  = trim(getInput("Minecraft version (e.g. 1.20.1): "));
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
  f << "     VER:\"" << ver << "\";\n";
  f << "     NAME:\"" << name << "\";\n";
  f << "     TECH:\"" << tech << "\";\n";
  f << "}\n";
  f.close();

  std::cout << col::green << "\ncontext.nmod created successfully at:\n" << col::reset << "  " << outPath.string() << "\n";
  waitForEnter();
}

void clearCache() {
  clearScreen();
  printBanner();
  std::cout << col::red << "WARNING: clearcache\n" << col::reset;
  std::cout << "──────────────────────────────────\n";
  std::cout << "This will permanently delete:\n";
  std::cout << "  - All backups in ~/nmodbackups/\n";
  std::cout << "  - context.nmod in the current directory\n\n";

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
    std::cout << "  [" << n << "] " << item.path().filename().string() << "\n";
  }
  std::cout << "──────────────────────────────────\n";
  std::cout << "  Total: " << n << " item(s)\n";
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
    fs::path modsDir   = getMinecraftModsDir();

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
    std::cout << "  [" << (i + 1) << "] " << backups[i].filename().string() << "\n";
  std::cout << "──────────────────────────────────\n\n";

  std::string choice = trim(getInput("Which modpack would you like to load? (enter number): "));

  int picked = 0;
  try { picked = std::stoi(choice); } catch (...) { picked = 0; } // ooooh oohhoooh du bist gut genug (i felt like it fuck off)

  if (picked < 1 || picked > (int)backups.size()) {
    std::cout << col::red << "Invalid selection." << col::reset << "\n";
    waitForEnter();
    return;
  }

  fs::path chosen  = backups[picked - 1];
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

// ─────────────────────────────────────────────────────────────────────────────
// web / selfhost stuff starts here. it's a bit of a beast but it works
// ─────────────────────────────────────────────────────────────────────────────

std::string resolvePackAlias(const std::string& alias) {
  // aptfx is a trusted shorthand. not advertised tho cus its private modpacks and you can fuck off (well since the source code is public u can now see my repository but its public anyway)
  if (iequals(alias, "aptfx")) return "https://github.com/aptfxx/mods/releases";
  return alias; // not an alias, pass it through as-is
}

// returns true if the URL resolves to the aptfx repo specifically.
// used to decide whether to show the "nmod cannot moderate" warning
bool isTrustedSource(const std::string& resolvedUrl) {
  return resolvedUrl.find("https://github.com/aptfxx/mods") != std::string::npos;
}

bool parsePackString(const std::string& input, std::string& outAlias, std::string& outPack) {
  size_t dot = input.rfind('.');
  if (dot == std::string::npos || dot == 0 || dot == input.size() - 1) return false;

  std::string raw = input.substr(0, dot);

  if (!raw.empty() && raw.front() == '[' && raw.back() == ']')
    raw = raw.substr(1, raw.size() - 2);

  outAlias = trim(raw);
  outPack  = trim(input.substr(dot + 1));
  return !outAlias.empty() && !outPack.empty();
}

std::string buildZipUrl(const std::string& releasesUrl, const std::string& packName) {

  std::string base = releasesUrl;
  if (!base.empty() && base.back() == '/') base.pop_back();

  size_t relPos = base.find("/releases");
  if (relPos != std::string::npos)
    base = base.substr(0, relPos + 9); // keep up to and including "/releases"

  return base + "/releases/latest/download/" + packName + ".zip";
}

// ── platform download wrappers ────────────────────────────────────────────────
// keeping them simple. not beautiful, but functional

#ifdef PLATFORM_WINDOWS

// wininet download. writes straight to fil
bool downloadFile(const std::string& url, const fs::path& dest) {
  HINTERNET hNet  = InternetOpenA("nmod/1", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hNet) return false;

  HINTERNET hUrl  = InternetOpenUrlA(hNet, url.c_str(), NULL, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
  if (!hUrl) { InternetCloseHandle(hNet); return false; }

  std::ofstream out(dest, std::ios::binary);
  if (!out.is_open()) { InternetCloseHandle(hUrl); InternetCloseHandle(hNet); return false; }

  char buf[8192];
  DWORD bytesRead = 0;
  while (InternetReadFile(hUrl, buf, sizeof(buf), &bytesRead) && bytesRead > 0)
    out.write(buf, bytesRead);

  InternetCloseHandle(hUrl);
  InternetCloseHandle(hNet);
  return true;
}

#else

static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
  std::ofstream* f = static_cast<std::ofstream*>(userdata);
  f->write(static_cast<char*>(ptr), size * nmemb);
  return size * nmemb;
}

bool downloadFile(const std::string& url, const fs::path& dest) {
  CURL* curl = curl_easy_init();
  if (!curl) return false;

  std::ofstream out(dest, std::ios::binary);
  if (!out.is_open()) { curl_easy_cleanup(curl); return false; }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow github redirects
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "nmod/1");

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  return res == CURLE_OK;
}

#endif

// ── zip extraction ─────────────────────────────────────────────────────────────

// ZIP format constants. these never change, which is nice
static const uint32_t ZIP_LOCAL_SIG   = 0x04034b50;
static const uint32_t ZIP_CENTRAL_SIG = 0x02014b50;
static const uint32_t ZIP_EOCD_SIG    = 0x06054b50;

// doing it manually so we don't have to worry about alignment
static uint16_t read16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t read32(const uint8_t* p) { return (uint32_t)(p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); }

struct ZipEntry {
  std::string name;
  uint32_t    compMethod;   // 0 = stored, 8 = deflated
  uint32_t    compSize;
  uint32_t    uncompSize;
  uint32_t    localOffset;  // byte offset of local file header
};

static bool findEOCD(std::ifstream& f, uint32_t& cdOffset, uint32_t& cdSize, uint16_t& numEntries) {
  f.seekg(0, std::ios::end);
  std::streamoff fileSize = f.tellg();
  // EOCD is at least 22 bytes, search up to 64KB back for zip comment tolerance
  std::streamoff searchStart = std::max((std::streamoff)0, fileSize - 65557);
  std::vector<uint8_t> tail((size_t)(fileSize - searchStart));
  f.seekg(searchStart);
  f.read((char*)tail.data(), (std::streamsize)tail.size());

  for (int i = (int)tail.size() - 22; i >= 0; i--) {
    if (read32(tail.data() + i) == ZIP_EOCD_SIG) {
      numEntries = read16(tail.data() + i + 10);
      cdSize     = read32(tail.data() + i + 12);
      cdOffset   = read32(tail.data() + i + 16);
      return true;
    }
  }
  return false; // couldn't find it. probably not a zip file
}

static std::vector<ZipEntry> parseCentralDir(std::ifstream& f, uint32_t cdOffset, uint16_t numEntries) {
  std::vector<ZipEntry> entries;
  f.seekg(cdOffset);

  for (int i = 0; i < numEntries; i++) {
    uint8_t hdr[46];
    if (!f.read((char*)hdr, 46)) break;
    if (read32(hdr) != ZIP_CENTRAL_SIG) break;

    ZipEntry e;
    e.compMethod  = read16(hdr + 10);
    e.compSize    = read32(hdr + 20);
    e.uncompSize  = read32(hdr + 24);
    e.localOffset = read32(hdr + 42);

    uint16_t nameLen  = read16(hdr + 28);
    uint16_t extraLen = read16(hdr + 30);
    uint16_t commLen  = read16(hdr + 32);

    std::vector<char> nameBuf(nameLen);
    f.read(nameBuf.data(), nameLen);
    e.name = std::string(nameBuf.begin(), nameBuf.end());

    f.seekg(extraLen + commLen, std::ios::cur); // skip extra + file comment
    entries.push_back(e);
  }

  return entries;
}


struct BitReader {
  const uint8_t* data;
  size_t         size;
  size_t         pos  = 0; 
  int            bits = 0; 
  uint32_t       buf  = 0;


  void refill() {
    while (bits <= 24 && pos < size) {
      buf  |= (uint32_t)data[pos++] << bits;
      bits += 8;
    }
  }

  uint32_t peek(int n) { refill(); return buf & ((1u << n) - 1); }

  uint32_t read(int n) {
    uint32_t v = peek(n);
    buf >>= n; bits -= n;
    return v;
  }

  uint32_t readByte() {
    // flush any partial byte
    int discard = bits & 7;
    buf >>= discard; bits -= discard;
    return read(8);
  }

  void alignByte() { int d = bits & 7; buf >>= d; bits -= d; }
};


struct HuffTree {
  static const int FAST_BITS = 9;
  uint16_t fast[1 << FAST_BITS]; // fast lookup for short codes
  uint16_t slow[1 << (15 - FAST_BITS)]; // overflow table
  int maxLen = 0;

  void build(const uint8_t* lens, int n) {
    memset(fast, 0, sizeof(fast));
    memset(slow, 0, sizeof(slow));
    maxLen = 0;

    int counts[16] = {};
    for (int i = 0; i < n; i++) if (lens[i]) { counts[lens[i]]++; maxLen = std::max(maxLen, (int)lens[i]); }

    int nextCode[16] = {};
    int code = 0;
    for (int l = 1; l <= 15; l++) { code = (code + counts[l-1]) << 1; nextCode[l] = code; }

    for (int i = 0; i < n; i++) {
      int l = lens[i];
      if (!l) continue;
      int c = nextCode[l]++;
      // reverse the bits 
      int rev = 0;
      for (int b = 0; b < l; b++) rev = (rev << 1) | ((c >> b) & 1);

      uint16_t entry = (uint16_t)((i << 4) | l);
      if (l <= FAST_BITS) {
        for (int j = rev; j < (1 << FAST_BITS); j += (1 << l))
          fast[j] = entry;
      } else {
        int hi = rev >> FAST_BITS;
        slow[hi] = entry; 
      }
    }
  }

  // decodes one symbol from the bit reader
  int decode(BitReader& br) const {
    br.refill();
    uint16_t e = fast[br.buf & ((1 << FAST_BITS) - 1)];
    if (!e) {
      // try slow table
      e = slow[(br.buf >> FAST_BITS) & ((1 << (15 - FAST_BITS)) - 1)];
      if (!e) return -1; // corrupt stream
    }
    int len = e & 0xf;
    br.buf >>= len; br.bits -= len;
    return e >> 4;
  }
};

// cheers RFC 1951 (minecraft rich text symbol)3.2.6
static void buildFixedTrees(HuffTree& lit, HuffTree& dist) {
  uint8_t ll[288], dl[32];
  for (int i =   0; i <= 143; i++) ll[i] = 8;
  for (int i = 144; i <= 255; i++) ll[i] = 9;
  for (int i = 256; i <= 279; i++) ll[i] = 7;
  for (int i = 280; i <= 287; i++) ll[i] = 8;
  for (int i = 0; i < 32; i++) dl[i] = 5;
  lit.build(ll, 288);
  dist.build(dl, 32);
}

// length/distance decode tables. straight from the RFC.
static const int LEN_BASE[29]  = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const int LEN_EXTRA[29] = {0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0};
static const int DST_BASE[30]  = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const int DST_EXTRA[30] = {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13};

static bool decodeBlock(BitReader& br, const HuffTree& lit, const HuffTree& dist,
                        std::vector<uint8_t>& out) {
  while (true) {
    int sym = lit.decode(br);
    if (sym < 0) return false;
    if (sym < 256) {
      out.push_back((uint8_t)sym); // literal byte
    } else if (sym == 256) {
      break; // end of block
    } else {
      // length/distance back-reference
      int lenIdx = sym - 257;
      if (lenIdx >= 29) return false;
      int length = LEN_BASE[lenIdx] + (int)br.read(LEN_EXTRA[lenIdx]);

      int distSym = dist.decode(br);
      if (distSym < 0 || distSym >= 30) return false;
      int distance = DST_BASE[distSym] + (int)br.read(DST_EXTRA[distSym]);

      size_t start = out.size();
      if ((size_t)distance > start) return false; 
      for (int i = 0; i < length; i++)
        out.push_back(out[start - distance + i]);
    }
  }
  return true;
}

static bool rawDeflate(const uint8_t* in, size_t inSize, std::vector<uint8_t>& out) {
  BitReader br{in, inSize};

  while (true) {
    int bfinal = (int)br.read(1);
    int btype  = (int)br.read(2);

    if (btype == 0) {
      // stored 
      br.alignByte();
      uint16_t len  = (uint16_t)br.read(16);
      uint16_t nlen = (uint16_t)br.read(16);
      if ((len ^ nlen) != 0xffff) return false; // you complementing me? oh wow teehe i love you too 😢🥹
      for (int i = 0; i < len; i++) {
        if (br.pos >= br.size) return false;
        out.push_back(br.data[br.pos++]);
      }
    } else if (btype == 1) {
      // fixed huffman
      HuffTree lit, dist;
      buildFixedTrees(lit, dist);
      if (!decodeBlock(br, lit, dist, out)) return false;
    } else if (btype == 2) {
     
      int hlit  = (int)br.read(5) + 257; // number of literal/length codes
      int hdist = (int)br.read(5) + 1;  
      int hclen = (int)br.read(4) + 4;  

      // the code-length codes are stored in a weird order. thanks, RFC 1951
      static const int CL_ORDER[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
      uint8_t clLens[19] = {};
      for (int i = 0; i < hclen; i++) clLens[CL_ORDER[i]] = (uint8_t)br.read(3);

      HuffTree clTree;
      clTree.build(clLens, 19);
      std::vector<uint8_t> allLens;
      allLens.reserve(hlit + hdist);
      while ((int)allLens.size() < hlit + hdist) {
        int sym = clTree.decode(br);
        if (sym < 0) return false;
        if (sym < 16) {
          allLens.push_back((uint8_t)sym);
        } else if (sym == 16) {
          if (allLens.empty()) return false;
          int rep = (int)br.read(2) + 3;
          for (int i = 0; i < rep; i++) allLens.push_back(allLens.back());
        } else if (sym == 17) {
          int rep = (int)br.read(3) + 3;
          for (int i = 0; i < rep; i++) allLens.push_back(0);
        } else {
          int rep = (int)br.read(7) + 11;
          for (int i = 0; i < rep; i++) allLens.push_back(0);
        }
      }

      HuffTree lit, dist;
      lit.build(allLens.data(), hlit);
      dist.build(allLens.data() + hlit, hdist);
      if (!decodeBlock(br, lit, dist, out)) return false;
    } else {
      return false; // btype == 3 is reserved and invalid
    }

    if (bfinal) break;
  }

  return true;
}

// inflates one zip entry. reads compSize bytes from src, writes uncompSize bytes to dst.
static bool inflateEntry(std::ifstream& src, std::ofstream& dst, uint32_t compSize, uint32_t uncompSize) {
  std::vector<uint8_t> inBuf(compSize);
  src.read((char*)inBuf.data(), compSize);

  std::vector<uint8_t> outBuf;
  outBuf.reserve(uncompSize);

  if (!rawDeflate(inBuf.data(), compSize, outBuf)) return false;
  if (outBuf.size() != uncompSize) return false;

  dst.write((char*)outBuf.data(), (std::streamsize)outBuf.size());
  return true;
}

bool extractZip(const fs::path& zipPath, const fs::path& destDir) {
  std::ifstream f(zipPath, std::ios::binary);
  if (!f.is_open()) {
    std::cerr << col::red << "Failed to open zip file." << col::reset << "\n";
    return false;
  }

  uint32_t cdOffset = 0, cdSize = 0;
  uint16_t numEntries = 0;
  if (!findEOCD(f, cdOffset, cdSize, numEntries)) {
    std::cerr << col::red << "Not a valid zip file (no EOCD found)." << col::reset << "\n";
    return false;
  }

  std::vector<ZipEntry> entries = parseCentralDir(f, cdOffset, numEntries);
  fs::create_directories(destDir);

  for (const ZipEntry& e : entries) {
    fs::path outPath = destDir / e.name;

    if (!e.name.empty() && e.name.back() == '/') {
      fs::create_directories(outPath); // directory entry
      continue;
    }

    fs::create_directories(outPath.parent_path());

    f.seekg(e.localOffset + 26);
    uint8_t lhExtra[4];
    f.read((char*)lhExtra, 4);
    uint16_t lNameLen  = read16(lhExtra);
    uint16_t lExtraLen = read16(lhExtra + 2);
    f.seekg(e.localOffset + 30 + lNameLen + lExtraLen);

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) continue; // skip if we can't write it. not worth dying over

    if (e.compMethod == 0) {
      std::vector<char> buf(e.compSize);
      f.read(buf.data(), e.compSize);
      out.write(buf.data(), e.compSize);
    } else if (e.compMethod == 8) {
      if (!inflateEntry(f, out, e.compSize, e.uncompSize))
        std::cerr << col::yellow << "Warning: failed to inflate " << e.name << col::reset << "\n";
    } else {
      std::cerr << col::yellow << "Warning: unsupported compression method " << e.compMethod
                << " for " << e.name << ", skipping." << col::reset << "\n";
    }
  }

  return true;
}

// ── SHA256 ─────────────────────────────────────────────────────────────────────
// fuck man its not AS scary as it looks but i still wont be able to come back here
// in a couple of months and expect to pick up where i left off
// well thats a me in a few months problem

static const uint32_t SHA256_K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

// right-rotate. the spec calls these Sigma functions
static inline uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

std::string sha256File(const fs::path& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) return "(unreadable)";

  // initial hash values (first 32 bits of the fractional parts of the square roots of the first 8 primes)
  uint32_t h[8] = {
    0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
    0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
  };

  uint64_t totalBits = 0;
  bool done = false;
  bool padded = false;

  uint8_t block[64];
  int blockLen = 0;
  auto processBlock = [&]() {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
      w[i] = ((uint32_t)block[i*4]   << 24) | ((uint32_t)block[i*4+1] << 16)
            |((uint32_t)block[i*4+2] <<  8) |  (uint32_t)block[i*4+3];
    }
    for (int i = 16; i < 64; i++) {
      uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15],18) ^ (w[i-15] >> 3);
      uint32_t s1 = rotr32(w[i-2], 17) ^ rotr32(w[i-2], 19) ^ (w[i-2] >> 10);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i = 0; i < 64; i++) {
      uint32_t S1   = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
      uint32_t ch   = (e & f) ^ (~e & g);
      uint32_t temp1= hh + S1 + ch + SHA256_K[i] + w[i];
      uint32_t S0   = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
      uint32_t maj  = (a & b) ^ (a & c) ^ (b & c);
      uint32_t temp2= S0 + maj;
      hh=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
  };

  while (!done) {
    uint8_t buf[4096];
    f.read((char*)buf, sizeof(buf));
    std::streamsize got = f.gcount();
    totalBits += (uint64_t)got * 8;

    for (std::streamsize i = 0; i < got; i++) {
      block[blockLen++] = buf[i];
      if (blockLen == 64) { processBlock(); blockLen = 0; }
    }

    if (f.eof() && !padded) {
      // append the 0x80 padding byte
      block[blockLen++] = 0x80;
      if (blockLen == 64) { processBlock(); blockLen = 0; }

      // pad with zeros until 56 bytes into the block
      while (blockLen != 56) {
        block[blockLen++] = 0x00;
        if (blockLen == 64) { processBlock(); blockLen = 0; }
      }

      // append the 64-bit big-endian bit length
      for (int i = 7; i >= 0; i--) block[blockLen++] = (uint8_t)(totalBits >> (i * 8));
      processBlock();
      padded = true;
      done = true;
    } else if (!f && !f.eof()) {
      return "(read error)"; // actual I/O error
    }
  }

  std::ostringstream ss;
  for (int i = 0; i < 8; i++)
    for (int j = 3; j >= 0; j--)
      ss << std::hex << std::setw(2) << std::setfill('0') << ((h[i] >> (j*8)) & 0xff);
  return ss.str();
}

// returns file size in MB, two decimal places. tiny files will just show 0.00 which is fine
double fileSizeMB(const fs::path& path) {
  try {
    return (double)fs::file_size(path) / (1024.0 * 1024.0);
  } catch (...) {
    return 0.0; // if it errors just return 0
  }
}

// ── web install shit ────────────────────────────────────────────────────────────

void webInstall() {
  clearScreen();
  printBanner();

  std::cout << "This setting will download the nmod folder from the internet.\n";
  std::cout << col::yellow << "Do NOT download any files from unknown sources." << col::reset << "\n";
  std::cout << "An extensive list of every item will be given before downloading anything.\n\n";
  std::cout << "Press Enter to continue...";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  clearScreen();
  printBanner();

  std::cout << "Enter pack address (format: [source].packname)\n";
  std::cout << col::grey << "Example: [source].modpack1" << col::reset << "\n\n";

  std::string packInput = trim(getInput("Pack address: "));
  if (packInput.empty()) {
    std::cout << col::red << "Cancelled." << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::string alias, packName;
  if (!parsePackString(packInput, alias, packName)) {
    std::cout << col::red << "Invalid format. Use [source].packname" << col::reset << "\n";
    waitForEnter();
    return;
  }

  std::string resolvedUrl = resolvePackAlias(alias);
  bool trusted = isTrustedSource(resolvedUrl);

  std::string zipUrl = buildZipUrl(resolvedUrl, packName);

  // temp (yo tengo yo tengo yo tengo get the ref please tell me ur a flamingo fan)
  fs::path tempDir = getWebTempDir();
  if (fs::exists(tempDir)) fs::remove_all(tempDir);
  fs::create_directories(tempDir);

  fs::path zipDest = tempDir / (packName + ".zip");
  fs::path extractDest = tempDir / "extracted";

  clearScreen();
  printBanner();
  std::cout << "Fetching pack: " << col::cyan << packName << col::reset << "\n";
  std::cout << col::grey << zipUrl << col::reset << "\n\n";
  std::cout << "Downloading...\n";

  if (!downloadFile(zipUrl, zipDest)) {
    std::cout << col::red << "Download failed. Check the source and pack name and try again." << col::reset << "\n";
    fs::remove_all(tempDir); // tidy up even on failure
    waitForEnter();
    return;
  }

  std::cout << "Extracting...\n";
  if (!extractZip(zipDest, extractDest)) {
    std::cout << col::red << "Extraction failed. The zip may be corrupted or missing." << col::reset << "\n";
    fs::remove_all(tempDir);
    waitForEnter();
    return;
  }

  // flatten the list for display
  std::vector<fs::path> extractedFiles;
  for (const auto& item : fs::recursive_directory_iterator(extractDest))
    if (item.is_regular_file()) extractedFiles.push_back(item.path());
  std::sort(extractedFiles.begin(), extractedFiles.end());

  if (extractedFiles.empty()) {
    std::cout << col::red << "The zip was empty or contained no files." << col::reset << "\n";
    fs::remove_all(tempDir);
    waitForEnter();
    return;
  }

  while (true) {
    clearScreen();
    printBanner();

    std::cout << "Contents of '" << packName << "':\n";
    std::cout << "──────────────────────────────────\n";
    int n = 0;
    for (const auto& f : extractedFiles) {
      n++;
      std::cout << "  [" << n << "] " << f.filename().string() << "\n";
    }
    std::cout << "──────────────────────────────────\n";
    std::cout << "  Total: " << n << " item(s)\n\n";

    std::cout << "  Install  |  Hashcheck  |  Delete\n\n";
    std::string action = trim(getInput("Choose an option: "));

    if (iequals(action, "delete") || iequals(action, "d")) {
      fs::remove_all(tempDir);
      std::cout << col::green << "Files deleted. Nothing was installed." << col::reset << "\n";
      waitForEnter();
      return;
    }

    if (iequals(action, "hashcheck") || iequals(action, "h") || iequals(action, "hash")) {
      clearScreen();
      printBanner();
      std::cout << "SHA256 checksums for '" << packName << "':\n";
      std::cout << "──────────────────────────────────\n";
      for (const auto& f : extractedFiles) {
        std::string hash = sha256File(f);
        double mb = fileSizeMB(f);
        // format: filename (X.XX MB)
        //         sha256: <hash>
        // keeping it readable rather than squashing it onto one line
        std::cout << "\n  " << col::cyan << f.filename().string() << col::reset;
        std::cout << " (" << std::fixed << std::setprecision(2) << mb << " MB)\n";
        std::cout << "  " << col::grey << hash << col::reset << "\n";
      }
      std::cout << "\n──────────────────────────────────\n";
      std::cout << "Cross-reference these against the mod's official release page before installing.\n";
      waitForEnter();
      continue; // back to file list
    }

    if (iequals(action, "install") || iequals(action, "i")) {
      // ok chacho im not reponsible if you get your shit HACKED
      if (!trusted) {
        clearScreen();
        printBanner();
        std::cout << col::yellow << "You are installing mods from the internet, where nmod cannot moderate.\n" << col::reset;
        std::cout << "Type 'I understand' to install anyway: ";
        std::string ack;
        std::getline(std::cin, ack);
        if (!iequals(trim(ack), "i understand")) {
          std::cout << col::yellow << "Cancelled." << col::reset << "\n";
          waitForEnter();
          continue; // back to the file list so they can still delete or try again
        }
      }

      fs::path modsDir = getMinecraftModsDir();
      std::cout << "\nClearing current mods...\n";
      if (!fs::exists(modsDir)) fs::create_directories(modsDir);
      else clearDir(modsDir);

      std::cout << "Installing...\n";
      if (!copyDir(extractDest, modsDir)) {
        std::cout << col::red << "Installation failed." << col::reset << "\n";
        fs::remove_all(tempDir);
        waitForEnter();
        return;
      }

      std::cout << col::green << "'" << packName << "' installed successfully!" << col::reset << "\n";
      fs::remove_all(tempDir); // bye
      waitForEnter();
      return;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────

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
    bool hasNmods = fs::exists(nmodsDir) && fs::is_directory(nmodsDir);

    if (!hasNmods) {
      // still show the menu
      std::cout << col::grey << "No local 'nmods' folder found. Local options unavailable." << col::reset << "\n\n";
      std::cout << "  Web\n\n";
      std::string choice = trim(getInput("Choose an option: "));
      if (iequals(choice, "w") || iequals(choice, "web")) webInstall();
      else if (iequals(choice, "createcontext"))           createContext();
      else if (iequals(choice, "clearcache"))              clearCache(); // hidden command, not shown in menu
      else {
        std::cout << col::red << "Invalid option." << col::reset << "\n";
        waitForEnter();
      }
      continue;
    }

    fs::path contextFile = fs::current_path() / "context.nmod";
    if (fs::exists(contextFile)) {
      NmodContext ctx = parseContext(contextFile);
      if (ctx.valid) printContext(ctx);
    }

    int count = countItems(nmodsDir);
    std::cout << count << " Items Found\n";
    std::cout << "  Install  |  List  |  Restore  |  Web\n\n";

    std::string choice = trim(getInput("Choose an option: "));

    if      (iequals(choice, "i") || iequals(choice, "install"))   installMods(nmodsDir);
    else if (iequals(choice, "l") || iequals(choice, "list"))       listMods(nmodsDir);
    else if (iequals(choice, "r") || iequals(choice, "restore"))    restoreMods();
    else if (iequals(choice, "w") || iequals(choice, "web"))        webInstall();
    else if (iequals(choice, "createcontext"))                       createContext();
    else if (iequals(choice, "clearcache"))                         clearCache(); // hidde command, not shown in menu
    else {
      std::cout << col::red << "Invalid option." << col::reset << "\n";
      waitForEnter();
    }
  }

  return 0; // never actually reached but the compiler likes it
}
