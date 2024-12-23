#include "framework.h"
#include "osisp_1.h"
#include <windows.h>
#include <iostream>
#include <fstream>

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;  // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];  // имя класса главного окна
HWND hEdit;  // Дескриптор текстового поля

// Структура для сохранения рабочего состояния
struct AppState {    
    char userText[256];  // Текст, введенный пользователем
};

// Сохранение состояния в файл
void SaveState(AppState& state) {
    std::ofstream outFile("app_state.dat", std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<char*>(&state), sizeof(state));  // приведение типов
        outFile.close();
    }
}

// Загрузка состояния из файла
bool LoadState(AppState& state) {
    std::ifstream inFile("app_state.dat", std::ios::binary);
    if (inFile.is_open()) {
        inFile.read(reinterpret_cast<char*>(&state), sizeof(state));
        inFile.close();
        return true;
    }
    return false;
}

// Функция для создания копии процесса
void CreateNewProcess() {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Порождаем новый процесс
    if (!CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Ошибка создания процесса: " << GetLastError() << std::endl;
    }
    else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static AppState appState = { 0 };  // Начальное состояние

    switch (msg) {
    case WM_CREATE: {
        // Создаем текстовое поле
        hEdit = CreateWindowEx(0, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
            10, 10, 400, 25, hwnd, NULL, hInst, NULL);

        // Загрузка состояния после создания текстового поля
        if (LoadState(appState)) {
            SetWindowTextA(hEdit, appState.userText);  // Восстановление текста
        }
        break;
    }
    case WM_CLOSE: {
        // Сохранение состояния
        GetWindowTextA(hEdit, appState.userText, sizeof(appState.userText));
        SaveState(appState);

        // Порождение новой копии процесса
        CreateNewProcess();

        // Завершение текущего окна
        DestroyWindow(hwnd);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSISP1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_OSISP1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;  // Сохранить маркер экземпляра в глобальной переменной

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// Точка входа
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OSISP1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OSISP1));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
