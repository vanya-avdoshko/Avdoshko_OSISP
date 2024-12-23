#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <numeric>

// Параметры модели
constexpr size_t MEMORY_SIZE = 1000000; 
constexpr size_t BLOCK_SIZE = 100000;   
constexpr size_t THREAD_COUNT = 5;     

// Глобальные переменные
std::vector<int> sharedData(MEMORY_SIZE); 
std::vector<HANDLE> blockMutexes;         

// Структура для передачи параметров в поток
struct Task {
    size_t blockIndex;    
    int taskType;         // Тип задачи: 1 - среднее значение, 2 - минимальное значение, 3 - умножение первого числа
};

// Функция для обработки блока памяти
DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    Task* task = static_cast<Task*>(lpParam);
    size_t blockIndex = task->blockIndex;
    int taskType = task->taskType;

    // Синхронизация доступа к блоку
    WaitForSingleObject(blockMutexes[blockIndex], INFINITE);

    
    auto blockStart = sharedData.begin() + blockIndex * BLOCK_SIZE;
    auto blockEnd = blockStart + BLOCK_SIZE;

    if (taskType == 1) {
        long long sum = std::accumulate(blockStart, blockEnd, 0LL);
        double average = static_cast<double>(sum) / BLOCK_SIZE;
        std::cout << "Block " << blockIndex << " Average: " << average << std::endl;
    }
    else if (taskType == 2) {
        int minVal = *std::min_element(blockStart, blockEnd);
        std::cout << "Block " << blockIndex << " Min: " << minVal << std::endl;
    }
    else if (taskType == 3) {
        std::default_random_engine generator(static_cast<unsigned>(time(NULL)));
        std::uniform_int_distribution<int> distribution(1, 10);
        int randomMultiplier = distribution(generator);
        *blockStart *= randomMultiplier;
        std::cout << "Block " << blockIndex << " First number multiplied by " << randomMultiplier << ": " << *blockStart << std::endl;
    }

    ReleaseMutex(blockMutexes[blockIndex]);
    return 0;
}

int main() {
    for (size_t i = 0; i < MEMORY_SIZE; ++i) {
        sharedData[i] = static_cast<int>(i + 1);
    }

    size_t blockCount = MEMORY_SIZE / BLOCK_SIZE;
    blockMutexes.resize(blockCount);
    for (size_t i = 0; i < blockCount; ++i) {
        blockMutexes[i] = CreateMutex(NULL, FALSE, NULL);
    }

    std::vector<Task> tasks(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        tasks[i].blockIndex = i % blockCount;  // Каждому потоку назначается блок
        tasks[i].taskType = (i % 3) + 1;      // Тип задачи чередуется
    }

    std::vector<HANDLE> threads(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = CreateThread(NULL, 0, ThreadFunction, &tasks[i], 0, NULL);
    }

    WaitForMultipleObjects(THREAD_COUNT, threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }
    for (HANDLE mutex : blockMutexes) {
        CloseHandle(mutex);
    }

    return 0;
}
