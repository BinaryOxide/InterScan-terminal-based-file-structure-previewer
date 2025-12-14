/*
    ============================================================
                        InterScan
    ============================================================
    Description:
    A lightweight terminal-based tool to visually display the
    file structure of any directory in a tree-like format.

    Features:
    - Recursive directory traversal (full recursion)
    - Optional ignored file extensions using `--ignore:` or `--ignore`
      (e.g. --ignore: .cpp .json .py)
    - Color-coded output for folders, files, extensions, and tree
    - Sanitized user input and robust ignore-parsing
    - Folder/File count displayed at the end
    - Windows console APIs (GetFileAttributesA, FindFirstFileA)
    ============================================================
*/

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <conio.h>

// Console color enum (Windows console attribute values)
enum ConsoleColor {
    /*
    ============================================================
    Windows Console Color Reference (Foreground)
    ------------------------------------------------------------
    Value | Color
    ------+----------------
      0   | Black
      1   | Blue
      2   | Green
      3   | Cyan
      4   | Red
      5   | Magenta
      6   | Yellow
      7   | Light Gray
      8   | Dark Gray
      9   | Light Blue
     10   | Light Green
     11   | Light Cyan
     12   | Light Red
     13   | Light Magenta
     14   | Light Yellow
     15   | White
    ============================================================
    */
    DEFAULT = 7,
    FOLDER = 1,
    EXTENSION = 4,
    TREE = 5,
    PROMPT = 4
};

// --- trim both ends
static inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\n\r");
    return s.substr(a, b - a + 1);
}

// --- lowercase copy
static inline std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// --- clear console
void clear_screen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD count;
    COORD home = { 0, 0 };

    FillConsoleOutputCharacterA(hConsole, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(hConsole, home);
}

// --- is directory
bool is_directory(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

// --- get directory entries sorted (case-insensitive)
std::vector<std::string> get_directory_entries(const std::string& path) {
    std::vector<std::string> entries;
    std::string search = trim(path);
    if (search.empty()) return entries;
    std::string pattern = search + "\\*";

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return entries;

    do {
        std::string name = fd.cFileName;
        if (name != "." && name != "..") entries.push_back(name);
    } while (FindNextFileA(h, &fd) != 0);

    FindClose(h);
    std::sort(entries.begin(), entries.end(), [](const std::string& a, const std::string& b) {
        return to_lower(a) < to_lower(b);
        });
    return entries;
}

// --- sanitize path (trim + remove surrounding quotes)
std::string sanitize_path(std::string input) {
    input = trim(input);
    if (!input.empty() && (input.front() == '"' || input.front() == '\'')) input.erase(0, 1);
    if (!input.empty() && (input.back() == '"' || input.back() == '\'')) input.pop_back();
    return trim(input);
}

// --- parse ignored extensions substring into normalized extensions (.ext lowercase)
std::vector<std::string> parse_ignored_extensions(const std::string& raw) {
    std::string s = raw;
    for (char& c : s) {
        if (c == ',' || c == '&') c = ' ';
    }

    std::stringstream ss(s);
    std::string tok;
    std::vector<std::string> out;
    while (ss >> tok) {
        tok = trim(tok);
        if (tok.empty()) continue;
        if (tok.front() != '.') tok = "." + tok;
        tok = to_lower(tok);
        out.push_back(tok);
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// --- check if filename's extension is ignored (case-insensitive)
bool is_ignored_extension(const std::string& filename, const std::vector<std::string>& ignored_exts) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) return false;
    std::string ext = filename.substr(pos);
    ext = to_lower(ext);
    return std::find(ignored_exts.begin(), ignored_exts.end(), ext) != ignored_exts.end();
}

// --- recursive tree printer (full recursion) with counters
void print_tree(const std::string& path, const std::string& prefix, const std::vector<std::string>& ignored_exts,
    int& folderCount, int& fileCount) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    auto items = get_directory_entries(path);
    for (size_t i = 0; i < items.size(); ++i) {
        bool isLast = (i == items.size() - 1);
        std::string item = items[i];
        std::string full = path + (path.back() == '\\' ? "" : "\\") + item;

        std::string branch = isLast ? "#-->" : "|-->";
        if (is_directory(full)) {
            folderCount++;

            SetConsoleTextAttribute(hConsole, TREE);
            std::cout << prefix << branch;
            SetConsoleTextAttribute(hConsole, FOLDER);
            std::cout << "[" << item << "]" << std::endl;
            SetConsoleTextAttribute(hConsole, DEFAULT);

            std::string newPrefix = prefix + (isLast ? "     " : "|    ");
            print_tree(full, newPrefix, ignored_exts, folderCount, fileCount);
        }
        else {
            if (is_ignored_extension(item, ignored_exts)) continue;

            fileCount++;

            SetConsoleTextAttribute(hConsole, TREE);
            std::cout << prefix << branch;
            SetConsoleTextAttribute(hConsole, DEFAULT);

            size_t dot = item.find_last_of('.');
            if (dot != std::string::npos) {
                std::cout << item.substr(0, dot);
                SetConsoleTextAttribute(hConsole, EXTENSION);
                std::cout << item.substr(dot) << std::endl;
            }
            else {
                std::cout << item << std::endl;
            }
            SetConsoleTextAttribute(hConsole, DEFAULT);
        }
    }
}

// --- main
int main() {
    SetConsoleOutputCP(CP_UTF8);
    clear_screen();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hConsole, PROMPT);
    std::cout << "Enter Path : ";
    SetConsoleTextAttribute(hConsole, DEFAULT);

    std::string line;
    std::getline(std::cin, line);
    line = trim(line);
    if (line.empty()) {
        std::cout << "No input provided. Exiting." << std::endl;
        _getch();
        return 0;
    }

    // --- detect --ignore or --ignore: (case-insensitive)
    std::string lowerLine = to_lower(line);
    size_t ignorePos = std::string::npos;
    size_t ignoreLen = 0;
    const std::string key1 = "--ignore:";
    const std::string key2 = "--ignore";

    for (size_t i = 0; i < lowerLine.size(); ++i) {
        if (lowerLine.compare(i, key1.size(), key1) == 0) { ignorePos = i; ignoreLen = key1.size(); break; }
        if (lowerLine.compare(i, key2.size(), key2) == 0) { ignorePos = i; ignoreLen = key2.size(); break; }
    }

    std::string pathPart;
    std::vector<std::string> ignored_exts;

    if (ignorePos != std::string::npos) {
        pathPart = line.substr(0, ignorePos);
        std::string rawIgnore = line.substr(ignorePos + ignoreLen);
        rawIgnore = trim(rawIgnore);
        if (!rawIgnore.empty() && rawIgnore.front() == ':') rawIgnore.erase(0, 1);
        ignored_exts = parse_ignored_extensions(rawIgnore);
    }
    else {
        pathPart = line;
    }

    std::string path = sanitize_path(pathPart);
    if (path.empty() || !is_directory(path)) {
        std::cout << "Invalid or inaccessible directory path. Exiting." << std::endl;
        _getch();
        return 0;
    }

    size_t lastSlash = path.find_last_of("/\\");
    std::string root = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    if (root.empty()) root = path;

    // Print root folder
    SetConsoleTextAttribute(hConsole, FOLDER);
    std::cout << "\n" << root << "\\" << std::endl;
    SetConsoleTextAttribute(hConsole, DEFAULT);

    // Show ignored extensions
    if (!ignored_exts.empty()) {
        SetConsoleTextAttribute(hConsole, TREE);
        std::cout << "(Ignoring extensions:";
        SetConsoleTextAttribute(hConsole, EXTENSION);
        for (size_t i = 0; i < ignored_exts.size(); ++i) {
            std::cout << " " << ignored_exts[i];
            if (i + 1 < ignored_exts.size()) std::cout << ",";
        }
        SetConsoleTextAttribute(hConsole, DEFAULT);
        std::cout << ")\n\n";
    }

    // --- folder/file counters
    int folderCount = 0;
    int fileCount = 0;

    // Print directory tree with counters
    print_tree(path, "", ignored_exts, folderCount, fileCount);

    // Display totals
    SetConsoleTextAttribute(hConsole, TREE);
    std::cout << "\nFolders: " << folderCount << "\nFiles  : " << fileCount << std::endl;
    SetConsoleTextAttribute(hConsole, DEFAULT);

    std::cout << "\nPress any key to exit..." << std::endl;
    _getch();
    return 0;
}
