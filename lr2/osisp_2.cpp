#include <iostream>
#include <vector>
#include <chrono>
#include <windows.h>
#include <cstdlib>
#include <algorithm>

// Глобальная переменная — количество потоков
size_t NUM_THREADS = 12;

// Общее количество чисел для обработки
const size_t NUM_DOUBLES = 4 * 1024 * 1024;

// Структура для передачи данных в поток при использовании memory-mapped файлов
struct ThreadDataMMap {
    double* start;            // Указатель на начало данных для этого потока
    size_t length;            // Количество элементов для обработки
};

// Структура для передачи данных в поток при использовании традиционного ввода-вывода
struct ThreadDataTraditional {
    HANDLE file;              // Файловый дескриптор
    LARGE_INTEGER startOffset; // Начальное смещение в файле (в байтах)
    size_t length;            // Количество элементов для обработки
};

// Функция, выполняющая сортировку данных (для memory-mapped файлов)
DWORD WINAPI processChunkMMap(LPVOID param) {
    ThreadDataMMap* data = static_cast<ThreadDataMMap*>(param);

    // Сортировка данных
    std::sort(data->start, data->start + data->length);

    return 0;
}

// Функция, выполняющая сортировку данных (для традиционного файлового ввода-вывода)
DWORD WINAPI processChunkTraditional(LPVOID param) {
    ThreadDataTraditional* data = static_cast<ThreadDataTraditional*>(param);

    // Выделение буфера для данных
    std::vector<double> buffer(data->length);

    // Перемещение указателя файла на нужное смещение
    SetFilePointerEx(data->file, data->startOffset, NULL, FILE_BEGIN);

    // Чтение данных в буфер
    DWORD bytesRead;
    ReadFile(data->file, buffer.data(), static_cast<DWORD>(data->length * sizeof(double)), &bytesRead, NULL);

    // Сортировка данных
    std::sort(buffer.begin(), buffer.end());

    // Перемещение указателя файла обратно к началу
    SetFilePointerEx(data->file, data->startOffset, NULL, FILE_BEGIN);

    // Запись отсортированных данных обратно в файл
    DWORD bytesWritten;
    WriteFile(data->file, buffer.data(), static_cast<DWORD>(data->length * sizeof(double)), &bytesWritten, NULL);

    // Закрытие дескриптора файла для данного потока
    CloseHandle(data->file);

    return 0;
}

// Функция для заполнения файла случайными числами типа double
void fillFileWithDoubles(const char* filename, size_t numDoubles) {
    // Создание файла или перезапись существующего
    HANDLE file = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    // Генерация случайных данных
    std::vector<double> data(numDoubles);
    for (size_t i = 0; i < numDoubles; i++) {
        data[i] = static_cast<double>(rand()) / RAND_MAX * 1000; // Числа в диапазоне от 0 до 1000
    }

    // Запись данных в файл
    DWORD bytesWritten;
    WriteFile(file, data.data(), static_cast<DWORD>(numDoubles * sizeof(double)), &bytesWritten, nullptr);

    // Закрытие файла
    CloseHandle(file);
}

// Функция для сортировки файла с использованием memory-mapped файлов
void mmapSort(const char* filename) {
    HANDLE file = CreateFileA(
        filename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(file, &fileSize);

    // Создание объекта отображения файла
    HANDLE fileMapping = CreateFileMappingA(
        file,
        nullptr,
        PAGE_READWRITE,
        0,
        0,
        nullptr);

    double* data = static_cast<double*>(MapViewOfFile(
        fileMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0));

    // Подсчет количества элементов и разбиение на блоки для потоков
    size_t numDoubles = fileSize.QuadPart / sizeof(double);
    size_t chunkSize = numDoubles / NUM_THREADS;

    std::vector<HANDLE> threads(NUM_THREADS);
    std::vector<ThreadDataMMap> threadData(NUM_THREADS);

    // Создание потоков для сортировки
    for (size_t i = 0; i < NUM_THREADS; i++) {
        size_t start = i * chunkSize;
        size_t end = (i == NUM_THREADS - 1) ? numDoubles : (start + chunkSize);
        size_t length = end - start;

        threadData[i] = { data + start, length };

        threads[i] = CreateThread(
            nullptr,
            0,
            processChunkMMap,
            &threadData[i],
            0,
            nullptr);
    }

    // Ожидание завершения всех потоков
    WaitForMultipleObjects(static_cast<DWORD>(NUM_THREADS), threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }

    UnmapViewOfFile(data);
    CloseHandle(fileMapping);
    CloseHandle(file);
}

// Функция для сортировки файла с использованием традиционного ввода-вывода
void traditionalSort(const char* filename) {
    HANDLE file = CreateFileA(
        filename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(file, &fileSize);

    size_t numDoubles = fileSize.QuadPart / sizeof(double);
    size_t chunkSize = numDoubles / NUM_THREADS;

    std::vector<HANDLE> threads(NUM_THREADS);
    std::vector<ThreadDataTraditional> threadData(NUM_THREADS);

    // Создание потоков для сортировки
    for (size_t i = 0; i < NUM_THREADS; i++) {
        size_t start = i * chunkSize;
        size_t end = (i == NUM_THREADS - 1) ? numDoubles : (start + chunkSize);
        size_t length = end - start;

        HANDLE threadFileHandle;
        DuplicateHandle(
            GetCurrentProcess(),
            file,
            GetCurrentProcess(),
            &threadFileHandle,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

        LARGE_INTEGER startOffset;
        startOffset.QuadPart = start * sizeof(double);

        threadData[i] = { threadFileHandle, startOffset, length };

        threads[i] = CreateThread(
            nullptr,
            0,
            processChunkTraditional,
            &threadData[i],
            0,
            nullptr);
    }

    WaitForMultipleObjects(static_cast<DWORD>(NUM_THREADS), threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }

    CloseHandle(file);
}

int main() {
    setlocale(LC_ALL, "ru");
    const char* filename = "big_file.dat";

    for (NUM_THREADS = 1; NUM_THREADS <= 12; NUM_THREADS++) {
        std::cout << "Количество потоков: " << NUM_THREADS << '\n';

        // Заполнение файла случайными числами
        fillFileWithDoubles(filename, NUM_DOUBLES);

        // Измерение времени сортировки с использованием memory-mapped файлов
        auto startMmap = std::chrono::high_resolution_clock::now();
        mmapSort(filename);
        auto endMmap = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> durationMmap = endMmap - startMmap;
        std::cout << "Время сортировки через memory-mapped: " << durationMmap.count() << " секунд\n";

        // Заполнение файла случайными числами заново
        fillFileWithDoubles(filename, NUM_DOUBLES);

        // Измерение времени сортировки с использованием традиционного I/O
        auto startTraditional = std::chrono::high_resolution_clock::now();
        traditionalSort(filename);
        auto endTraditional = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> durationTraditional = endTraditional - startTraditional;
        std::cout << "Время сортировки через традиционный I/O: " << durationTraditional.count() << " секунд\n";

        // Рассчет ускорения
        double speedup = durationTraditional.count() / durationMmap.count();
        std::cout << "Ускорение (Memory-mapped / Традиционный): " << speedup << '\n';
    }

    return 0;
}
