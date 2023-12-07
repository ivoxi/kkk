#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <Windows.h>
const wchar_t* SharedMemoryName = L"GameSharedMemory";
const wchar_t* MutexName = L"GameMutex";

HANDLE hSharedMemory;
HANDLE hMutex;
const int maxCellsCount = 200;
const int defaultCellsCount = 4;
const int defaultWindowWidth = 320;
const int defaultWindowHeight = 240;

struct Settings {
    int cellsCount;
    int windowWidth;
    int windowHeight;
    COLORREF windowBgColor;
    COLORREF gridLineColor;
    COLORREF currentBgColor;
};

Settings appConfig = { defaultCellsCount, defaultWindowWidth, defaultWindowHeight, RGB(0, 0, 255), RGB(255, 0, 0), RGB(0, 0, 255) };

int grid[maxCellsCount][maxCellsCount] = { 0 };
int cellWidth, cellHeight;
HBRUSH bgBrush;

void LoadSettingsFileVars();
void SaveSettingsFileVars(const Settings& settings);
void LoadSettingsFromFileStream();
void SaveSettingsToFileStream(const Settings& settings);
void LoadSettingsMapping();
void SaveSettingsMapping(const Settings& settings);
void LoadSettingsWinApi();
void SaveSettingsWinApi(const Settings& settings);
void ModifyGridColor(HDC hdc);
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void LoadSettingsFileVars() {
    FILE* configFile;

    if (fopen_s(&configFile, "config.txt", "r") == 0) {
        if (fscanf_s(configFile, "CellsCount=%d\n", &appConfig.cellsCount) != 1) {
            appConfig.cellsCount = defaultCellsCount;
        }

        if (fscanf_s(configFile, "WindowSize=%d %d\n", &appConfig.windowWidth, &appConfig.windowHeight) != 2) {
            appConfig.windowWidth = defaultWindowWidth;
            appConfig.windowHeight = defaultWindowHeight;
        }

        int red, green, blue;

        if (fscanf_s(configFile, "WindowBgColor=%d %d %d\n", &red, &green, &blue) == 3) {
            appConfig.windowBgColor = RGB(red, green, blue);
            appConfig.currentBgColor = RGB(red, green, blue);
        }
        else {
            red = 255;
            green = 255;
            blue = 255;
            appConfig.windowBgColor = RGB(red, green, blue);
            appConfig.currentBgColor = RGB(red, green, blue);
        }

        if (fscanf_s(configFile, "GridLineColor=%d %d %d\n", &red, &green, &blue) == 3) {
            appConfig.gridLineColor = RGB(red, green, blue);
        }
        else {
            red = 255;
            green = 0;
            blue = 0;
            appConfig.gridLineColor = RGB(red, green, blue);
        }

        fclose(configFile);
    }
    else {
        std::cerr << "Failed to open config file. Using default settings." << std::endl;
    }
}

void SaveSettingsFileVars(const Settings& settings) {
    FILE* configFile;

    if (fopen_s(&configFile, "config.txt", "w") == 0) {
        fprintf(configFile, "CellsCount=%d\n", settings.cellsCount);
        fprintf(configFile, "WindowSize=%d %d\n", settings.windowWidth, settings.windowHeight);
        fprintf(configFile, "WindowBgColor=%d %d %d\n", GetRValue(settings.currentBgColor),
            GetGValue(settings.currentBgColor), GetBValue(settings.currentBgColor));
        fprintf(configFile, "GridLineColor=%d %d %d\n", GetRValue(settings.gridLineColor),
            GetGValue(settings.gridLineColor), GetBValue(settings.gridLineColor));

        fclose(configFile);
    }
    else {
        std::cerr << "Failed to create or open config file for writing." << std::endl;
    }
}
#include <fstream>
#include <iostream>

void LoadSettingsFromFileStream() {
    std::ifstream configFile("config.txt");

    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            std::istringstream iss(line);
            std::string key;
            if (std::getline(iss, key, '=')) {
                if (key == "CellsCount") {
                    iss >> appConfig.cellsCount;
                }
                else if (key == "WindowSize") {
                    iss >> appConfig.windowWidth >> appConfig.windowHeight;
                }
                else if (key == "WindowBgColor") {
                    int red, green, blue;
                    if (iss >> red >> green >> blue) {
                        appConfig.windowBgColor = RGB(red, green, blue);
                        appConfig.currentBgColor = RGB(red, green, blue);
                    }
                }
                else if (key == "GridLineColor") {
                    int red, green, blue;
                    if (iss >> red >> green >> blue) {
                        appConfig.gridLineColor = RGB(red, green, blue);
                    }
                }
            }
        }
        configFile.close();
    }
    else {
        std::cerr << "Failed to open config file. Using default settings." << std::endl;
    }
}

void SaveSettingsToFileStream(const Settings& settings) {
    std::ofstream configFile("config.txt");

    if (configFile.is_open()) {
        configFile << "CellsCount=" << settings.cellsCount << "\n";
        configFile << "WindowSize=" << settings.windowWidth << " " << settings.windowHeight << "\n";
        configFile << "WindowBgColor=" << static_cast<int>(GetRValue(settings.currentBgColor)) << " "
            << static_cast<int>(GetGValue(settings.currentBgColor)) << " " << static_cast<int>(GetBValue(settings.currentBgColor)) << "\n";
        configFile << "GridLineColor=" << static_cast<int>(GetRValue(settings.gridLineColor)) << " "
            << static_cast<int>(GetGValue(settings.gridLineColor)) << " " << static_cast<int>(GetBValue(settings.gridLineColor)) << "\n";

        configFile.close();
    }
    else {
        std::cerr << "Failed to create or open config file for writing." << std::endl;
    }
}

#include <Windows.h>
#include <iostream>

void LoadSettingsMapping() {
    HANDLE hFile = CreateFile(
        L"config.txt",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        HANDLE hMapFile = CreateFileMapping(
            hFile,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL
        );

        if (hMapFile != NULL) {
            char* fileContent = static_cast<char*>(MapViewOfFile(
                hMapFile,
                FILE_MAP_READ,
                0,
                0,
                0
            ));

            if (fileContent != NULL) {
                sscanf_s(fileContent, "CellsCount=%d\n", &appConfig.cellsCount);
                fileContent = strchr(fileContent, '\n') + 1;
                sscanf_s(fileContent, "WindowSize=%d %d\n", &appConfig.windowWidth, &appConfig.windowHeight);
                fileContent = strchr(fileContent, '\n') + 1;

                int red, green, blue;
                if (sscanf_s(fileContent, "WindowBgColor=%d %d %d\n", &red, &green, &blue) == 3) {
                    appConfig.windowBgColor = RGB(red, green, blue);
                    appConfig.currentBgColor = RGB(red, green, blue);
                }
                fileContent = strchr(fileContent, '\n') + 1;

                if (sscanf_s(fileContent, "GridLineColor=%d %d %d\n", &red, &green, &blue) == 3) {
                    appConfig.gridLineColor = RGB(red, green, blue);
                }

                UnmapViewOfFile(fileContent);
            }

            CloseHandle(hMapFile);
        }

        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open config file. Using default settings." << std::endl;
    }
}

void SaveSettingsMapping(const Settings& settings) {
    HANDLE hFile = CreateFile(
        L"config.txt",
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        char buffer[256];
        DWORD dwBytesWritten;

        sprintf_s(buffer, sizeof(buffer), "CellsCount=%d\n", settings.cellsCount);
        WriteFile(hFile, buffer, strlen(buffer), &dwBytesWritten, NULL);

        sprintf_s(buffer, sizeof(buffer), "WindowSize=%d %d\n", settings.windowWidth, settings.windowHeight);
        WriteFile(hFile, buffer, strlen(buffer), &dwBytesWritten, NULL);

        sprintf_s(buffer, sizeof(buffer), "WindowBgColor=%d %d %d\n",
            GetRValue(settings.windowBgColor), GetGValue(settings.windowBgColor), GetBValue(settings.windowBgColor));
        WriteFile(hFile, buffer, strlen(buffer), &dwBytesWritten, NULL);

        sprintf_s(buffer, sizeof(buffer), "GridLineColor=%d %d %d\n",
            GetRValue(settings.gridLineColor), GetGValue(settings.gridLineColor), GetBValue(settings.gridLineColor));
        WriteFile(hFile, buffer, strlen(buffer), &dwBytesWritten, NULL);

        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to create or open config file for writing." << std::endl;
    }
}


void LoadSettingsWinApi() {
    HANDLE hFile = CreateFile(
        L"config.txt",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize != INVALID_FILE_SIZE) {
            char* fileContent = new char[fileSize + 1];

            DWORD bytesRead;
            if (ReadFile(hFile, fileContent, fileSize, &bytesRead, NULL)) {
                fileContent[bytesRead] = '\0';

                char* currentPos = fileContent;
                int charsRead;

                if (sscanf_s(currentPos, "CellsCount=%d%n", &appConfig.cellsCount, &charsRead) == 1) {
                    currentPos += charsRead;
                }
                else {
                    appConfig.cellsCount = defaultCellsCount;
                }

                if (sscanf_s(currentPos, "WindowSize=%d %d%n", &appConfig.windowWidth, &appConfig.windowHeight, &charsRead) == 2) {
                    currentPos += charsRead;
                }
                else {
                    appConfig.windowWidth = defaultWindowWidth;
                    appConfig.windowHeight = defaultWindowHeight;
                }

                int red, green, blue;
                if (sscanf_s(currentPos, "WindowBgColor=%d %d %d%n", &red, &green, &blue, &charsRead) == 3) {
                    appConfig.windowBgColor = RGB(red, green, blue);
                    appConfig.currentBgColor = RGB(red, green, blue);
                    currentPos += charsRead;
                }
                else {
                    appConfig.windowBgColor = RGB(255, 255, 255);
                    appConfig.currentBgColor = RGB(255, 255, 255);
                }

                if (sscanf_s(currentPos, "GridLineColor=%d %d %d%n", &red, &green, &blue, &charsRead) == 3) {
                    appConfig.gridLineColor = RGB(red, green, blue);
                    currentPos += charsRead;
                }
                else {
                    appConfig.gridLineColor = RGB(255, 0, 0);
                }
            }

            delete[] fileContent;
        }

        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to open config file. Using default settings." << std::endl;
    }
}

void SaveSettingsWinApi(const Settings& settings) {
    HANDLE hFile = CreateFile(
        L"config.txt",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        char buffer[MAX_PATH];
        DWORD bytesWritten;

        sprintf_s(buffer, MAX_PATH, "CellsCount=%d\n", settings.cellsCount);
        WriteFile(hFile, buffer, static_cast<DWORD>(strlen(buffer)), &bytesWritten, NULL);

        sprintf_s(buffer, MAX_PATH, "WindowSize=%d %d\n", settings.windowWidth, settings.windowHeight);
        WriteFile(hFile, buffer, static_cast<DWORD>(strlen(buffer)), &bytesWritten, NULL);

        sprintf_s(buffer, MAX_PATH, "WindowBgColor=%d %d %d\n",
            GetRValue(settings.currentBgColor), GetGValue(settings.currentBgColor), GetBValue(settings.currentBgColor));
        WriteFile(hFile, buffer, static_cast<DWORD>(strlen(buffer)), &bytesWritten, NULL);

        sprintf_s(buffer, MAX_PATH, "GridLineColor=%d %d %d\n",
            GetRValue(settings.gridLineColor), GetGValue(settings.gridLineColor), GetBValue(settings.gridLineColor));
        WriteFile(hFile, buffer, static_cast<DWORD>(strlen(buffer)), &bytesWritten, NULL);

        CloseHandle(hFile);
    }
    else {
        std::cerr << "Failed to create or open config file for writing." << std::endl;
    }
}






void ModifyGridColor(HDC hdc) {
    HPEN newPen = CreatePen(PS_SOLID, 1, appConfig.gridLineColor);
    if (newPen == nullptr) {
        std::cerr << "Failed to create grid color pen." << std::endl;
        return;
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, newPen);

    for (int i = 1; i < appConfig.cellsCount; i++) {
        int y = i * cellHeight;
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, appConfig.windowWidth, y);
    }

    for (int i = 1; i < appConfig.cellsCount; i++) {
        int x = i * cellWidth;
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, appConfig.windowHeight);
    }

    SelectObject(hdc, oldPen);
    if (!DeleteObject(newPen)) {
        std::cerr << "Failed to delete grid color pen." << std::endl;
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int configMode = 1; // По умолчанию используем файлы

    if (lpCmdLine && lpCmdLine[0] != '\0') {
        int cellsCount = atoi(lpCmdLine);
        cellsCount = min(maxCellsCount, max(cellsCount, 1));
        appConfig.cellsCount = cellsCount;

        if (lpCmdLine[2] != '\0') {
            configMode = atoi(lpCmdLine + 2);
        }
    }

    // Выбор функций в зависимости от configMode
    void (*loadSettingsFunction)() = nullptr;
    void (*saveSettingsFunction)(const Settings&) = nullptr;

    switch (configMode) {
    case 1:
        loadSettingsFunction = LoadSettingsFileVars;
        saveSettingsFunction = SaveSettingsFileVars;
        break;
    case 2:
        loadSettingsFunction = LoadSettingsFromFileStream;
        saveSettingsFunction = SaveSettingsToFileStream;
        break;
    case 3:
        loadSettingsFunction = LoadSettingsMapping;
        saveSettingsFunction = SaveSettingsMapping;
        break;
    case 4:
        loadSettingsFunction = LoadSettingsWinApi;
        saveSettingsFunction = SaveSettingsWinApi;
        break;
    default:
        std::cerr << "Invalid configMode. Using default file functions." << std::endl;
        loadSettingsFunction = LoadSettingsFileVars;
        saveSettingsFunction = SaveSettingsFileVars;
        break;
    }

    loadSettingsFunction();

    if (lpCmdLine && lpCmdLine[0] != '\0') {
        int cellsCount = atoi(lpCmdLine);
        cellsCount = min(maxCellsCount, max(cellsCount, 1));
        appConfig.cellsCount = cellsCount;
    }

    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"WinAPIAppClass";

    if (!RegisterClassEx(&windowClass)) {
        MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindow(windowClass.lpszClassName, L"Lab 3", WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME,
        100, 100, appConfig.windowWidth, appConfig.windowHeight, nullptr, nullptr, hInstance, nullptr);

    cellWidth = appConfig.windowWidth / appConfig.cellsCount;
    cellHeight = appConfig.windowHeight / appConfig.cellsCount;

    if (hwnd == nullptr) {
        MessageBox(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 0;
    }

    bgBrush = CreateSolidBrush(appConfig.windowBgColor);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    hMutex = CreateMutex(NULL, FALSE, MutexName);
    if (hMutex == NULL) {
        MessageBox(nullptr, L"Failed to create mutex!", L"Error", MB_ICONERROR);
        return 0;
    }

    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * maxCellsCount * maxCellsCount, SharedMemoryName);
    if (hSharedMemory == NULL) {
        MessageBox(nullptr, L"Failed to create shared memory!", L"Error", MB_ICONERROR);
        CloseHandle(hMutex);
        return 0;
    }

    MSG msg = { nullptr };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnmapViewOfFile(hSharedMemory);
    CloseHandle(hSharedMemory);
    CloseHandle(hMutex);

    saveSettingsFunction(appConfig);
    DeleteObject(bgBrush);

    return msg.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 100, nullptr); // 100 ms timer
        break;
    case WM_TIMER:
        WaitForSingleObject(hMutex, INFINITE);  // lock mutex

        int* sharedMemory = static_cast<int*>(MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0));

        for (int i = 0; i < appConfig.cellsCount; i++) {
            for (int j = 0; j < appConfig.cellsCount; j++) {
                int value = sharedMemory[i * maxCellsCount + j];
                if (grid[i][j] != value) {
                    grid[i][j] = value;
                    InvalidateRect(hwnd, nullptr, TRUE);
                }
            }
        }

        UnmapViewOfFile(sharedMemory);

        ReleaseMutex(hMutex);  // unlock mutex
    break;
    case WM_CLOSE:
        PostQuitMessage(0);
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, bgBrush);

        HPEN redPen = CreatePen(PS_SOLID, 1, appConfig.gridLineColor);
        if (redPen == nullptr) {
            std::cerr << "Failed to create grid color pen for painting." << std::endl;
            EndPaint(hwnd, &ps);
            return 0;
        }

        HPEN oldPen = (HPEN)SelectObject(hdc, redPen);

        for (int i = 1; i < appConfig.cellsCount; i++) {
            int y = i * cellHeight;
            MoveToEx(hdc, 0, y, nullptr);
            LineTo(hdc, ps.rcPaint.right, y);
        }

        for (int i = 1; i < appConfig.cellsCount; i++) {
            int x = i * cellWidth;
            MoveToEx(hdc, x, 0, nullptr);
            LineTo(hdc, x, ps.rcPaint.bottom);
        }

        SelectObject(hdc, oldPen);
        if (!DeleteObject(redPen)) {
            std::cerr << "Failed to delete grid color pen after painting." << std::endl;
        }

        // Определение размера разделяемой памяти
        int sharedMemorySize = sizeof(int) * maxCellsCount * maxCellsCount;

        // Попытка прочитать разделяемую память
        int* sharedMemoryGrid = static_cast<int*>(MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, sharedMemorySize));

        if (sharedMemoryGrid != nullptr) {
            for (int row = 0; row < appConfig.cellsCount; row++) {
                for (int col = 0; col < appConfig.cellsCount; col++) {
                    int cellValue = sharedMemoryGrid[row * maxCellsCount + col];
                    if (cellValue == 1) {
                        Ellipse(hdc, col * cellWidth, row * cellHeight, (col + 1) * cellWidth, (row + 1) * cellHeight);
                    }
                    else if (cellValue == 2) {
                        MoveToEx(hdc, col * cellWidth, row * cellHeight, nullptr);
                        LineTo(hdc, (col + 1) * cellWidth, (row + 1) * cellHeight);
                        MoveToEx(hdc, (col + 1) * cellWidth, row * cellHeight, nullptr);
                        LineTo(hdc, col * cellWidth, (row + 1) * cellHeight);
                    }
                }
            }

            // Освобождение разделяемой памяти
            UnmapViewOfFile(sharedMemoryGrid);
        }

        EndPaint(hwnd, &ps);
    }
    break;


    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || (GetKeyState(VK_LCONTROL) & 0x8000 && wParam == 'Q')) {
            PostQuitMessage(0);
        }
        else if (GetKeyState(VK_SHIFT) & 0x8000 && wParam == 'C') {
            ShellExecute(nullptr, L"open", L"notepad.exe", nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN) {
            int red = rand() % 256;
            int green = rand() % 256;
            int blue = rand() % 256;
            appConfig.currentBgColor = RGB(red, green, blue);
            DeleteObject(bgBrush);
            bgBrush = CreateSolidBrush(appConfig.currentBgColor);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int red = GetRValue(appConfig.gridLineColor);
        int green = GetGValue(appConfig.gridLineColor);
        int blue = GetBValue(appConfig.gridLineColor);

        if (delta > 0) {
            red = min(255, red + 2);
            green = min(255, green + 2);
            blue = min(255, blue + 2);
        }
        else {
            red = max(0, red - 2);
            green = max(0, green - 2);
            blue = max(0, blue - 2);
        }
        appConfig.gridLineColor = RGB(red, green, blue);
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    break;
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int col = cellWidth == 0 ? 0 : x / cellWidth;
        int row = cellHeight == 0 ? 0 : y / cellHeight;

        WaitForSingleObject(hMutex, INFINITE);  //  

        if (col < appConfig.cellsCount && row < appConfig.cellsCount && grid[row][col] == 0) {
            grid[row][col] = 1;

            //   
            int* sharedMemory = static_cast<int*>(MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0));
            sharedMemory[row * maxCellsCount + col] = 1;
            UnmapViewOfFile(sharedMemory);

            InvalidateRect(hwnd, nullptr, TRUE);
        }

        ReleaseMutex(hMutex);  //  
    }
    break;
    case WM_RBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int col = cellWidth == 0 ? 0 : x / cellWidth;
        int row = cellHeight == 0 ? 0 : y / cellHeight;

        WaitForSingleObject(hMutex, INFINITE);  //  

        if (col < appConfig.cellsCount && row < appConfig.cellsCount && grid[row][col] == 0) {
            grid[row][col] = 2;

            //   
            int* sharedMemory = static_cast<int*>(MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0));
            sharedMemory[row * maxCellsCount + col] = 2;
            UnmapViewOfFile(sharedMemory);

            InvalidateRect(hwnd, nullptr, TRUE);
        }

        ReleaseMutex(hMutex);  //  
    }
    break;


    case WM_SIZE:
    {
        RECT rect;
        GetWindowRect(hwnd, &rect);

        appConfig.windowWidth = rect.right - rect.left;
        appConfig.windowHeight = rect.bottom - rect.top;

        cellWidth = appConfig.cellsCount == 0 ? 0 : appConfig.windowWidth / appConfig.cellsCount;
        cellHeight = appConfig.cellsCount == 0 ? 0 : appConfig.windowHeight / appConfig.cellsCount;
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    break;

    case WM_SIZING:
    {
        RECT* rect = (RECT*)lParam;
        int newWidth = rect->right - rect->left;
        int newHeight = rect->bottom - rect->top;

        newWidth = max(newWidth, cellWidth * appConfig.cellsCount);
        newHeight = max(newHeight, cellHeight * appConfig.cellsCount);

        rect->right = rect->left + newWidth;
        rect->bottom = rect->top + newHeight;

        return TRUE;
    }
    break;

    case WM_NCHITTEST:
    {
        LRESULT hitTest = DefWindowProc(hwnd, message, wParam, lParam);

        if (hitTest == HTBOTTOM || hitTest == HTBOTTOMLEFT || hitTest == HTBOTTOMRIGHT) {
            return HTBOTTOM;
        }

        return hitTest;
    }
    break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

