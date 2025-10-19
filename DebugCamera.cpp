
#include "pch.h"
#include "DebugCamera.h"
#include "MemoryManager.h"

#include <windows.h>
#include <psapi.h>        // Для GetModuleInformation
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>
#include <iomanip>
#include <limits>         // Для std::numeric_limits
#include <algorithm>      // Для std::min
#include <conio.h>        // Для _kbhit() и _getch()
#include <cstdlib>        // Для rand()
#include <ctime>          // Для time()
#include <map>            // Для std::map

#undef min
#undef max

// Функция для удаления пробельных символов с начала и конца строки.
std::string Trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Функция для надежного чтения из консоли
std::string ReadConsoleInput() {
    char buffer[256];
    DWORD bytesRead;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    
    if (ReadConsoleA(hStdin, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        // Убираем символы переноса строки
        for (DWORD i = 0; i < bytesRead; i++) {
            if (buffer[i] == '\r' || buffer[i] == '\n') {
                buffer[i] = '\0';
                break;
            }
        }
        return std::string(buffer);
    }
    return "";
}

// Функция для чтения трех float чисел
bool ReadThreeFloats(float& x, float& y, float& z) {
    std::string input = ReadConsoleInput();
    std::istringstream iss(input);
    if (iss >> x >> y >> z) {
        return true;
    }
    return false;
}

// Проверяет, состоит ли строка только из шестнадцатеричных цифр.
bool IsValidHex(const std::string& str) {
    if (str.empty())
        return false;
    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

// Преобразует шестнадцатеричную строку в число типа uintptr_t (поддерживается префикс "0x").
bool HexStringToAddress(const std::string& hexString, uintptr_t& address) {
    std::string str = Trim(hexString);
    if (str.size() >= 2 && (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X"))
        str = str.substr(2);
    if (!IsValidHex(str))
        return false;
    std::stringstream ss;
    ss << std::hex << str;
    ss >> address;
    return true;
}

// Преобразует шестнадцатеричную строку в последовательность байтов.
std::string HexStringToBytes(const std::string& hexStr) {
    std::string bytes;
    if (hexStr.length() % 2 != 0)
        return "";
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byteString = hexStr.substr(i, 2);
        char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Сканирует память указанного модуля в поиске шаблона с заданной маской,
// возвращая все найденные адреса.
std::vector<uintptr_t> FindAllPatterns(HMODULE hModule, const char* pattern, const char* mask) {
    std::vector<uintptr_t> addresses;
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    size_t patternLength = strlen(mask);

    for (uintptr_t current = startAddress; current < endAddress - patternLength; current++) {
        bool found = true;
        for (size_t i = 0; i < patternLength; i++) {
            if (mask[i] == 'x' && *reinterpret_cast<char*>(current + i) != pattern[i]) {
                found = false;
                break;
            }
        }
        if (found)
            addresses.push_back(current);
    }
    return addresses;
}

// Структура для отслеживания изменений значений
struct MemorySnapshot {
    uintptr_t address;
    float values[3];
    int changeCount;
};

// Структура для хранения найденных адресов персонажа и камеры
struct GameAddresses {
    std::vector<uintptr_t> playerAddresses;
    std::vector<uintptr_t> cameraAddresses;
};

// Многократное сканирование для точного определения типа адреса
struct ScanPhase {
    std::string name;
    int duration;
    std::string instruction;
};

GameAddresses MultipassAutoDetect() {
    GameAddresses addresses;
    std::cout << "[*] Запуск МНОГОФАЗНОГО СКАНИРОВАНИЯ для точного определения адресов..." << std::endl;
    
    // Определяем фазы сканирования
    std::vector<ScanPhase> phases = {
        {"ФАЗА 1: СТАТИЧЕСКАЯ", 5, "НЕ ДВИГАЙТЕСЬ, стойте неподвижно"},
        {"ФАЗА 2: ДВИЖЕНИЕ ИГРОКА", 8, "ДВИГАЙТЕСЬ (W/A/S/D), НЕ поворачивайте камеру"},
        {"ФАЗА 3: ПОВОРОТ КАМЕРЫ", 8, "НЕ двигайтесь, только ПОВОРАЧИВАЙТЕ КАМЕРУ мышью"},
        {"ФАЗА 4: СМЕШАННОЕ", 6, "Свободно ДВИГАЙТЕСЬ И ПОВОРАЧИВАЙТЕ КАМЕРУ"}
    };
    
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    
    // Структура для отслеживания адресов через все фазы
    struct AddressTracker {
        uintptr_t address;
        std::vector<std::vector<CameraCoordinates>> phaseSnapshots;
        std::vector<int> phaseChanges;
        float totalVariance;
        int totalChanges;
        std::string detectedType;
    };
    
    std::vector<AddressTracker> candidates;
    
    // Первоначальный поиск кандидатов
    std::cout << "[*] Поиск потенциальных адресов..." << std::endl;
    for (uintptr_t addr = startAddress; addr < endAddress - 12; addr += 16) {
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            if (abs(ptr[0]) < 10000.0f && abs(ptr[1]) < 10000.0f && abs(ptr[2]) < 10000.0f &&
                ptr[0] != 0.0f && ptr[1] != 0.0f && ptr[2] != 0.0f) {
                
                AddressTracker tracker;
                tracker.address = addr;
                tracker.phaseSnapshots.resize(phases.size());
                tracker.phaseChanges.resize(phases.size(), 0);
                tracker.totalVariance = 0.0f;
                tracker.totalChanges = 0;
                tracker.detectedType = "НЕИЗВЕСТНО";
                candidates.push_back(tracker);
            }
        } catch (...) {
            continue;
        }
    }
    
    std::cout << "[*] Найдено " << candidates.size() << " кандидатов для многофазного анализа" << std::endl;
    
    // Выполняем каждую фазу сканирования
    for (size_t phaseIdx = 0; phaseIdx < phases.size(); phaseIdx++) {
        const ScanPhase& phase = phases[phaseIdx];
        
        std::cout << "\n" << phase.name << " (" << phase.duration << " сек)" << std::endl;
        std::cout << "ИНСТРУКЦИЯ: " << phase.instruction << std::endl;
        std::cout << "Подготовьтесь... Начинаем через 3 секунды..." << std::endl;
        Sleep(3000);
        
        const int SNAPSHOTS_PER_PHASE = phase.duration * 2; // 2 снимка в секунду
        const int SLEEP_MS = 500;
        
        // Сохраняем начальное состояние для этой фазы
        for (auto& candidate : candidates) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(candidate.address);
                candidate.phaseSnapshots[phaseIdx].push_back(coords);
            } catch (...) {
                continue;
            }
        }
        
        // Мониторинг изменений в этой фазе
        for (int snapshot = 0; snapshot < SNAPSHOTS_PER_PHASE; snapshot++) {
            Sleep(SLEEP_MS);
            std::cout << "•";
            std::cout.flush();
            
            for (auto& candidate : candidates) {
                try {
                    CameraCoordinates coords = ReadCameraCoordinates(candidate.address);
                    
                    if (!candidate.phaseSnapshots[phaseIdx].empty()) {
                        CameraCoordinates prev = candidate.phaseSnapshots[phaseIdx].back();
                        float deltaX = abs(coords.x - prev.x);
                        float deltaY = abs(coords.y - prev.y);
                        float deltaZ = abs(coords.z - prev.z);
                        
                        if (deltaX > 0.01f || deltaY > 0.01f || deltaZ > 0.01f) {
                            candidate.phaseChanges[phaseIdx]++;
                        }
                    }
                    
                    candidate.phaseSnapshots[phaseIdx].push_back(coords);
                } catch (...) {
                    continue;
                }
            }
        }
        
        std::cout << " ЗАВЕРШЕНО" << std::endl;
    }
    
    std::cout << "\n[*] Анализ результатов многофазного сканирования..." << std::endl;
    
    // Анализ паттернов поведения по фазам
    for (auto& candidate : candidates) {
        candidate.totalChanges = 0;
        for (int changes : candidate.phaseChanges) {
            candidate.totalChanges += changes;
        }
        
        // Определяем тип на основе паттернов активности по фазам
        int staticPhase = candidate.phaseChanges[0];    // Фаза 1: статическая
        int playerPhase = candidate.phaseChanges[1];    // Фаза 2: движение игрока
        int cameraPhase = candidate.phaseChanges[2];    // Фаза 3: поворот камеры
        int mixedPhase = candidate.phaseChanges[3];     // Фаза 4: смешанное
        
        // Улучшенная классификация на основе многофазного анализа
        if (staticPhase <= 1 && playerPhase >= 3 && cameraPhase <= 2) {
            // Изменяется при движении игрока, но не при вращении камеры
            candidate.detectedType = "ИГРОК";
            addresses.playerAddresses.push_back(candidate.address);
        } else if (staticPhase <= 1 && cameraPhase >= 4 && (playerPhase <= 2 || mixedPhase >= 6)) {
            // Изменяется при вращении камеры
            candidate.detectedType = "КАМЕРА";
            addresses.cameraAddresses.push_back(candidate.address);
        } else if (playerPhase >= 3 && cameraPhase >= 3 && mixedPhase >= 5) {
            // Изменяется при любом движении - может быть универсальным адресом
            candidate.detectedType = "УНИВЕРСАЛЬНЫЙ";
            addresses.cameraAddresses.push_back(candidate.address); // Добавляем как камеру
        } else if (candidate.totalChanges >= 5) {
            candidate.detectedType = "ДИНАМИЧЕСКИЙ";
        } else {
            candidate.detectedType = "СТАТИЧЕСКИЙ";
        }
    }
    
    // Сортируем по общей активности
    std::sort(candidates.begin(), candidates.end(), 
              [](const AddressTracker& a, const AddressTracker& b) {
                  return a.totalChanges > b.totalChanges;
              });
    
    // Отображаем результаты анализа
    std::cout << "\n[+] РЕЗУЛЬТАТЫ МНОГОФАЗНОГО АНАЛИЗА:" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    int shown = 0;
    for (const auto& candidate : candidates) {
        if (candidate.totalChanges > 0 && shown < 20) { // Показываем только активные адреса
            std::cout << candidate.detectedType << " 0x" << std::hex << candidate.address << std::dec;
            std::cout << " [Статич:" << candidate.phaseChanges[0];
            std::cout << ", Игрок:" << candidate.phaseChanges[1];
            std::cout << ", Камера:" << candidate.phaseChanges[2];
            std::cout << ", Смеш:" << candidate.phaseChanges[3];
            std::cout << ", Всего:" << candidate.totalChanges << "]" << std::endl;
            shown++;
        }
    }
    
    std::cout << "\n[+] Классифицировано:" << std::endl;
    std::cout << "- Адресов ИГРОКА: " << addresses.playerAddresses.size() << std::endl;
    std::cout << "- Адресов КАМЕРЫ: " << addresses.cameraAddresses.size() << std::endl;
    
    return addresses;
}

// Расширенный анализ адресов для определения их точного назначения
struct ExtendedAddressInfo {
    uintptr_t address;
    std::string primaryType;      // ИГРОК, КАМЕРА, НЕИЗВЕСТНО
    std::string secondaryType;    // ПОЗИЦИЯ, УГЛЫ, НАПРАВЛЕНИЕ, СКОРОСТЬ
    std::string detailedType;     // Подробное описание
    float avgX, avgY, avgZ;       // Средние значения
    float varX, varY, varZ;       // Дисперсия
    int changeFrequency;
    float stability;
    bool isAngular;               // Содержит углы
    bool isPosition;              // Содержит позицию
    bool isGroundLevel;           // На уровне земли
    bool isHighFrequency;         // Высокочастотные изменения
};

std::vector<ExtendedAddressInfo> AnalyzeAddressTypes(const std::vector<uintptr_t>& addresses) {
    std::vector<ExtendedAddressInfo> results;
    std::cout << "[*] Запуск РАСШИРЕННОГО АНАЛИЗА типов адресов..." << std::endl;
    std::cout << "[*] Это займет 12 секунд. Активно двигайтесь и поворачивайте камеру!" << std::endl;
    
    const int ANALYSIS_DURATION = 12;
    const int SNAPSHOTS = 24;
    const int SLEEP_MS = (ANALYSIS_DURATION * 1000) / SNAPSHOTS;
    
    // Инициализация результатов
    for (uintptr_t addr : addresses) {
        ExtendedAddressInfo info;
        info.address = addr;
        info.primaryType = "НЕИЗВЕСТНО";
        info.secondaryType = "ДАННЫЕ";
        info.detailedType = "Неклассифицировано";
        info.avgX = info.avgY = info.avgZ = 0.0f;
        info.varX = info.varY = info.varZ = 0.0f;
        info.changeFrequency = 0;
        info.stability = 0.0f;
        info.isAngular = false;
        info.isPosition = false;
        info.isGroundLevel = false;
        info.isHighFrequency = false;
        results.push_back(info);
    }
    
    // Сбор данных
    std::vector<std::vector<CameraCoordinates>> snapshots(addresses.size());
    
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "•";
        std::cout.flush();
        
        for (size_t i = 0; i < addresses.size(); i++) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
                
                // Подсчет изменений
                if (!snapshots[i].empty()) {
                    CameraCoordinates prev = snapshots[i].back();
                    if (abs(coords.x - prev.x) > 0.01f || 
                        abs(coords.y - prev.y) > 0.01f || 
                        abs(coords.z - prev.z) > 0.01f) {
                        results[i].changeFrequency++;
                    }
                }
                
                snapshots[i].push_back(coords);
            } catch (...) {
                continue;
            }
        }
    }
    
    std::cout << std::endl << "[*] Анализ паттернов данных..." << std::endl;
    
    // Детальный анализ каждого адреса
    for (size_t i = 0; i < results.size(); i++) {
        ExtendedAddressInfo& info = results[i];
        
        if (snapshots[i].size() < 3) continue; // Недостаточно данных
        
        // Вычисляем статистики
        for (const auto& coords : snapshots[i]) {
            info.avgX += coords.x;
            info.avgY += coords.y;
            info.avgZ += coords.z;
        }
        info.avgX /= snapshots[i].size();
        info.avgY /= snapshots[i].size();
        info.avgZ /= snapshots[i].size();
        
        // Вычисляем дисперсию
        for (const auto& coords : snapshots[i]) {
            info.varX += (coords.x - info.avgX) * (coords.x - info.avgX);
            info.varY += (coords.y - info.avgY) * (coords.y - info.avgY);
            info.varZ += (coords.z - info.avgZ) * (coords.z - info.avgZ);
        }
        info.varX /= snapshots[i].size();
        info.varY /= snapshots[i].size();
        info.varZ /= snapshots[i].size();
        
        info.stability = 1.0f / (1.0f + info.varX + info.varY + info.varZ);
        
        // Определяем характеристики данных
        info.isAngular = (abs(info.avgX) <= 6.28f && abs(info.avgY) <= 6.28f && abs(info.avgZ) <= 6.28f) ||
                        (abs(info.avgX) <= 360.0f && abs(info.avgY) <= 360.0f && abs(info.avgZ) <= 360.0f);
        
        info.isPosition = (abs(info.avgX) < 5000.0f && abs(info.avgY) < 1000.0f && abs(info.avgZ) < 5000.0f);
        info.isGroundLevel = (info.avgY > -100.0f && info.avgY < 200.0f);
        info.isHighFrequency = (info.changeFrequency >= 10);
        
        // ПРОДВИНУТАЯ КЛАССИФИКАЦИЯ
        if (info.isAngular && info.changeFrequency >= 5) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "УГЛЫ";
            if (info.varX > 0.5f || info.varY > 0.5f) {
                info.detailedType = "Углы поворота камеры (активные)";
            } else {
                info.detailedType = "Углы поворота камеры (статические)";
            }
        }
        else if (info.isPosition && info.isGroundLevel && info.changeFrequency >= 3 && info.changeFrequency < 10) {
            info.primaryType = "ИГРОК";
            info.secondaryType = "ПОЗИЦИЯ";
            if (info.varX > 10.0f || info.varZ > 10.0f) {
                info.detailedType = "Позиция игрока (активное движение)";
            } else {
                info.detailedType = "Позиция игрока (медленное движение)";
            }
        }
        else if (info.isPosition && !info.isGroundLevel && info.isHighFrequency) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "ПОЗИЦИЯ";
            if (abs(info.avgY) > 200.0f) {
                info.detailedType = "Позиция свободной камеры";
            } else {
                info.detailedType = "Позиция камеры следования";
            }
        }
        else if (info.changeFrequency >= 15) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "ВЫСОКОЧАСТОТНЫЕ";
            info.detailedType = "Высокочастотные данные камеры";
        }
        else if (info.isPosition && info.changeFrequency >= 8) {
            info.primaryType = "УНИВЕРСАЛЬНЫЙ";
            info.secondaryType = "КООРДИНАТЫ";
            info.detailedType = "Универсальные координаты (игрок+камера)";
        }
        else if (info.changeFrequency >= 3) {
            info.primaryType = "ДИНАМИЧЕСКИЙ";
            info.secondaryType = "НЕИЗВЕСТНО";
            info.detailedType = "Динамические данные (неопределено)";
        }
        else {
            info.primaryType = "СТАТИЧЕСКИЙ";
            info.secondaryType = "ДАННЫЕ";
            info.detailedType = "Статические или медленно изменяющиеся данные";
        }
    }
    
    // Сортируем по релевантности (частота изменений + стабильность)
    std::sort(results.begin(), results.end(), 
              [](const ExtendedAddressInfo& a, const ExtendedAddressInfo& b) {
                  float scoreA = a.changeFrequency + a.stability * 5;
                  float scoreB = b.changeFrequency + b.stability * 5;
                  return scoreA > scoreB;
              });
    
    return results;
}

// Умный автоматический тест всех найденных адресов с детальной классификацией
struct AddressTestResult {
    uintptr_t address;
    bool isResponsive;
    bool isPlayer;
    bool isCamera;
    float stability;
    int changeFrequency;
    std::string type;
};

std::vector<AddressTestResult> SmartTestAllAddresses(const std::vector<uintptr_t>& addresses) {
    std::vector<AddressTestResult> results;
    std::cout << "[*] Запуск УМНОГО ТЕСТА всех " << addresses.size() << " адресов..." << std::endl;
    std::cout << "[*] Это займет 15 секунд. Пожалуйста, АКТИВНО ДВИГАЙТЕСЬ в игре!" << std::endl;
    
    const int TEST_DURATION = 15; // секунд
    const int SNAPSHOTS = 30; // количество снимков
    const int SLEEP_MS = (TEST_DURATION * 1000) / SNAPSHOTS;
    
    // Инициализация результатов
    for (uintptr_t addr : addresses) {
        AddressTestResult result;
        result.address = addr;
        result.isResponsive = false;
        result.isPlayer = false;
        result.isCamera = false;
        result.stability = 0.0f;
        result.changeFrequency = 0;
        result.type = "НЕИЗВЕСТНО";
        results.push_back(result);
    }
    
    // Сохраняем начальные значения
    std::vector<std::vector<CameraCoordinates>> snapshots(addresses.size());
    for (size_t i = 0; i < addresses.size(); i++) {
        try {
            CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
            snapshots[i].push_back(coords);
        } catch (...) {
            continue;
        }
    }
    
    // Мониторинг изменений
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "Снимок [" << (snapshot + 1) << "/" << SNAPSHOTS << "] ";
        std::cout.flush();
        
        for (size_t i = 0; i < addresses.size(); i++) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
                
                // Проверяем изменения с предыдущим снимком
                if (!snapshots[i].empty()) {
                    CameraCoordinates prev = snapshots[i].back();
                    float deltaX = abs(coords.x - prev.x);
                    float deltaY = abs(coords.y - prev.y);
                    float deltaZ = abs(coords.z - prev.z);
                    
                    if (deltaX > 0.01f || deltaY > 0.01f || deltaZ > 0.01f) {
                        results[i].changeFrequency++;
                    }
                }
                
                snapshots[i].push_back(coords);
            } catch (...) {
                continue;
            }
        }
        
        if ((snapshot + 1) % 10 == 0) {
            std::cout << std::endl;
        }
    }
    
    std::cout << std::endl << "[*] Анализ результатов..." << std::endl;
    
    // Анализ и классификация адресов
    for (size_t i = 0; i < results.size(); i++) {
        AddressTestResult& result = results[i];
        
        if (result.changeFrequency >= 3) {
            result.isResponsive = true;
            
            // Анализ паттернов движения для классификации
            if (!snapshots[i].empty()) {
                float avgX = 0, avgY = 0, avgZ = 0;
                float varianceX = 0, varianceY = 0, varianceZ = 0;
                
                // Вычисляем средние значения
                for (const auto& coords : snapshots[i]) {
                    avgX += coords.x;
                    avgY += coords.y;
                    avgZ += coords.z;
                }
                avgX /= snapshots[i].size();
                avgY /= snapshots[i].size();
                avgZ /= snapshots[i].size();
                
                // Вычисляем дисперсию
                for (const auto& coords : snapshots[i]) {
                    varianceX += (coords.x - avgX) * (coords.x - avgX);
                    varianceY += (coords.y - avgY) * (coords.y - avgY);
                    varianceZ += (coords.z - avgZ) * (coords.z - avgZ);
                }
                varianceX /= snapshots[i].size();
                varianceY /= snapshots[i].size();
                varianceZ /= snapshots[i].size();
                
                result.stability = 1.0f / (1.0f + varianceX + varianceY + varianceZ);
                
                // Улучшенная эвристика классификации
                if (abs(avgY) < 100.0f && abs(avgX) < 1000.0f && abs(avgZ) < 1000.0f &&
                    varianceY < 50.0f && result.changeFrequency >= 5) {
                    result.isPlayer = true;
                    result.type = "ИГРОК";
                } else if (result.changeFrequency >= 8 || varianceX > 100.0f || varianceZ > 100.0f) {
                    result.isCamera = true;
                    result.type = "КАМЕРА";
                } else if (result.changeFrequency >= 3) {
                    result.type = "ДИНАМИЧЕСКИЙ";
                }
            }
        } else {
            result.type = "СТАТИЧЕСКИЙ";
        }
    }
    
    // Сортируем результаты по частоте изменений
    std::sort(results.begin(), results.end(), 
              [](const AddressTestResult& a, const AddressTestResult& b) {
                  return a.changeFrequency > b.changeFrequency;
              });
    
    return results;
}

// Реальный автодетект камеры и персонажа по изменяющимся координатам
GameAddresses RealAutoDetectAll() {
    GameAddresses addresses;
    std::cout << "[*] Запуск РЕАЛЬНОГО автоопределения для ИГРОКА и КАМЕРЫ..." << std::endl;
    std::cout << "[*] Это займет 10 секунд. Пожалуйста, ДВИГАЙТЕСЬ и ПОВОРАЧИВАЙТЕ КАМЕРУ в игре!" << std::endl;
    std::cout.flush();
    
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    
    std::cout << "[*] Сканирование диапазона памяти: 0x" << std::hex << startAddress << " - 0x" << endAddress << std::endl;
    std::cout << "[*] Поиск изменяющихся float значений..." << std::endl;
    std::cout.flush();
    
    std::vector<MemorySnapshot> candidates;
    const int SCAN_DURATION = 10; // секунд
    const int SNAPSHOTS = 20; // количество снимков
    const int SLEEP_MS = (SCAN_DURATION * 1000) / SNAPSHOTS;
    
    // Первоначальный поиск float значений
    for (uintptr_t addr = startAddress; addr < endAddress - 12; addr += 16) { // шаг 16 для оптимизации
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            // Проверяем, что это похоже на координаты (разумные значения)
            if (abs(ptr[0]) < 10000.0f && abs(ptr[1]) < 10000.0f && abs(ptr[2]) < 10000.0f &&
                ptr[0] != 0.0f && ptr[1] != 0.0f && ptr[2] != 0.0f) {
                
                MemorySnapshot snapshot;
                snapshot.address = addr;
                snapshot.values[0] = ptr[0];
                snapshot.values[1] = ptr[1];
                snapshot.values[2] = ptr[2];
                snapshot.changeCount = 0;
                candidates.push_back(snapshot);
            }
        }
        catch (...) {
            // Игнорируем недоступные адреса
            continue;
        }
    }
    
    std::cout << "[*] Найдено " << candidates.size() << " потенциальных float троек" << std::endl;
    std::cout << "[*] Отслеживание изменений..." << std::endl;
    std::cout.flush();
    
    // Мониторинг изменений
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "[" << (snapshot + 1) << "/" << SNAPSHOTS << "] ";
        std::cout.flush();
        
        for (auto& candidate : candidates) {
            try {
                float* ptr = reinterpret_cast<float*>(candidate.address);
                
                // Проверяем изменились ли значения
                if (abs(ptr[0] - candidate.values[0]) > 0.01f ||
                    abs(ptr[1] - candidate.values[1]) > 0.01f ||
                    abs(ptr[2] - candidate.values[2]) > 0.01f) {
                    
                    candidate.changeCount++;
                    candidate.values[0] = ptr[0];
                    candidate.values[1] = ptr[1];
                    candidate.values[2] = ptr[2];
                }
            }
            catch (...) {
                // Адрес стал недоступен
                continue;
            }
        }
        
        if ((snapshot + 1) % 5 == 0) {
            std::cout << std::endl;
            std::cout.flush();
        }
    }
    
    std::cout << std::endl << "[*] Анализ завершен! Поиск часто изменяющихся адресов..." << std::endl;
    
    // Анализ результатов - ищем адреса с частыми изменениями
    std::vector<std::pair<uintptr_t, int>> bestCandidates;
    for (const auto& candidate : candidates) {
        if (candidate.changeCount >= 3) { // Изменялись хотя бы 3 раза
            bestCandidates.push_back({candidate.address, candidate.changeCount});
        }
    }
    
    // Сортируем по количеству изменений
    std::sort(bestCandidates.begin(), bestCandidates.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "[*] Найдено " << bestCandidates.size() << " динамических адресов:" << std::endl;
    std::cout << "[*] Анализ адресов для разделения ИГРОКА от КАМЕРЫ..." << std::endl;
    
    for (size_t i = 0; i < std::min(bestCandidates.size(), size_t(15)); i++) {
        uintptr_t addr = bestCandidates[i].first;
        int changes = bestCandidates[i].second;
        
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            float x = ptr[0], y = ptr[1], z = ptr[2];
            
            // Эвристика для определения типа адреса
            bool isPlayer = false;
            bool isCamera = false;
            
            // УЛУЧШЕННАЯ ЭВРИСТИКА для точного определения типа адреса
            
            // Анализ диапазонов значений для более точной классификации
            bool hasReasonablePlayerCoords = (abs(y) < 200.0f && abs(x) < 2000.0f && abs(z) < 2000.0f);
            bool hasGroundLevelY = (y > -50.0f && y < 100.0f); // Игрок обычно на уровне земли
            bool hasHighVariability = (changes >= 10); // Очень активные изменения
            bool hasModerateActivity = (changes >= 5 && changes < 10); // Умеренная активность
            bool hasLowActivity = (changes >= 3 && changes < 5); // Низкая активность
            
            // Определяем углы камеры (обычно в диапазоне -PI до +PI или 0-360 градусов)
            bool likelyAngles = ((abs(x) <= 6.28f || abs(x) <= 360.0f) && 
                               (abs(y) <= 6.28f || abs(y) <= 360.0f) && 
                               (abs(z) <= 6.28f || abs(z) <= 360.0f));
            
            // Проверка на возможные координаты позиции игрока
            if (hasReasonablePlayerCoords && hasGroundLevelY && hasModerateActivity && !likelyAngles) {
                isPlayer = true;
                std::string subtype = "ПОЗИЦИЯ";
                if (changes >= 8) subtype = "ПОЗИЦИЯ+ДВИЖЕНИЕ";
                type = "[ИГРОК:" + subtype + "]";
            }
            // Проверка на координаты камеры (обычно более активные и могут быть выше)
            else if (hasHighVariability && !hasGroundLevelY && !likelyAngles) {
                isCamera = true;
                std::string subtype = "3D-ПОЗИЦИЯ";
                if (abs(y) > 100.0f) subtype = "СВОБОДНАЯ-КАМЕРА";
                type = "[КАМЕРА:" + subtype + "]";
            }
            // Проверка на углы поворота камеры
            else if (likelyAngles && changes >= 5) {
                isCamera = true;
                type = "[КАМЕРА:УГЛЫ]";
            }
            // Проверка на смешанные координаты (может быть универсальным адресом)
            else if (hasReasonablePlayerCoords && hasHighVariability) {
                isCamera = true; // Добавляем как камеру для универсальности
                type = "[УНИВЕРСАЛЬНЫЙ]";
            }
            // Статические или слабо активные адреса
            else if (hasLowActivity) {
                std::string subtype = "СТАТИЧЕСКИЙ";
                if (hasReasonablePlayerCoords) subtype = "МЕДЛЕННЫЙ-ИГРОК";
                type = "[" + subtype + "]";
            }
            // Активные адреса с неясным назначением
            else if (changes >= 5) {
                type = "[ДИНАМИЧЕСКИЙ]";
                isCamera = true; // По умолчанию добавляем как камеру
            }
            else {
                type = "[НЕИЗВЕСТНО]";
            }
            
            std::cout << "   " << type << " 0x" << std::hex << addr << " (изменилось " << std::dec << changes << " раз)" << std::endl;
            std::cout << "     Значения: " << std::fixed << std::setprecision(2) << x << ", " << y << ", " << z << std::endl;
            
            if (isPlayer) {
                addresses.playerAddresses.push_back(addr);
            } else {
                addresses.cameraAddresses.push_back(addr);
            }
        }
        catch (...) {
            std::cout << "   [ОШИБКА] 0x" << std::hex << addr << " - Невозможно прочитать значения" << std::endl;
        }
    }
    
    std::cout << "[+] Найдено " << addresses.playerAddresses.size() << " потенциальных адресов ИГРОКА" << std::endl;
    std::cout << "[+] Найдено " << addresses.cameraAddresses.size() << " потенциальных адресов КАМЕРЫ" << std::endl;
    
    return addresses;
}

// Автоматический поиск камеры (для обратной совместимости)
std::vector<uintptr_t> AutoDetectCameraAddresses() {
    GameAddresses addresses = RealAutoDetectAll();
    std::vector<uintptr_t> combined;
    
    // Объединяем адреса персонажа и камеры
    combined.insert(combined.end(), addresses.playerAddresses.begin(), addresses.playerAddresses.end());
    combined.insert(combined.end(), addresses.cameraAddresses.begin(), addresses.cameraAddresses.end());
    
    return combined;
}

// Ищет адреса камеры с использованием заданного шаблона и маски.
// Возвращает вектор найденных адресов.
std::vector<uintptr_t> FindCameraAddresses(const std::string& currentPattern, const std::string& currentMask) {
    HMODULE hModule = GetModuleHandle(nullptr);
    std::vector<uintptr_t> addresses;
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    addresses = FindAllPatterns(hModule, currentPattern.c_str(), currentMask.c_str());
    if (addresses.empty())
        std::cerr << "Не удалось найти адреса камеры по заданному шаблону." << std::endl;
    else {
        // Фильтруем найденные адреса, оставляя только те, где значение magic соответствует 0x12345678.
        std::vector<uintptr_t> validAddresses;
        for (auto addr : addresses) {
            if (IsValidCameraAddress(addr))
                validAddresses.push_back(addr);
        }
        if (validAddresses.empty())
            std::cerr << "Не найдено допустимых адресов камеры." << std::endl;
        else {
            std::cout << "Найдены допустимые адреса камеры (показаны первые 3):" << std::endl;
            for (size_t i = 0; i < std::min(validAddresses.size(), size_t(3)); i++) {
                std::cout << "0x" << std::hex << validAddresses[i] << " ";
            }
            std::cout << std::endl;
        }
        return validAddresses;
    }
    return addresses;
}

CDebugCamera::CDebugCamera() {
    // Дополнительная инициализация, если требуется.
}

extern "C" {

    DEBUGCAMERA_API int fnDebugCamera(void) {
        return 0;
    }

    DEBUGCAMERA_API DWORD WINAPI MainThread(LPVOID lpParam) {
        static bool initialized = false;
        if (initialized) {
            MessageBox(NULL, L"MainThread already initialized!", L"Info", MB_OK);
            return 0;
        }
        initialized = true;

        // Подробное логирование инициализации
        MessageBox(NULL, L"Starting DLL initialization...", L"Debug", MB_OK);

        if (!AllocConsole()) {
            DWORD error = GetLastError();
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"AllocConsole failed! Error code: %lu", error);
            MessageBox(NULL, errorMsg, L"Error", MB_ICONERROR);
            return 1;
        }
        SetConsoleTitle(L"Debug Console for DebugCamera");
        
        MessageBox(NULL, L"Console allocated successfully!", L"Debug", MB_OK);

        // Перенаправление stdout
        FILE* fpOut = nullptr;
        errno_t result = freopen_s(&fpOut, "CONOUT$", "w", stdout);
        if (result != 0) {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to redirect stdout! Error: %d", result);
            MessageBox(NULL, errorMsg, L"Error", MB_ICONERROR);
        } else {
            MessageBox(NULL, L"stdout redirected successfully!", L"Debug", MB_OK);
        }
        
        // Более надежное перенаправление stdin
        FILE* fpIn = nullptr;
        result = freopen_s(&fpIn, "CONIN$", "r", stdin);
        if (result != 0) {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to redirect stdin! Error: %d", result);
            MessageBox(NULL, errorMsg, L"Error", MB_ICONERROR);
        } else {
            MessageBox(NULL, L"stdin redirected successfully!", L"Debug", MB_OK);
        }
        
        // Дополнительная настройка консоли для ввода
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        
        if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE) {
            MessageBox(NULL, L"Failed to get console handles", L"Error", MB_ICONERROR);
            return 1;
        }
        
        // Включаем режим ввода
        DWORD mode;
        if (!GetConsoleMode(hStdin, &mode)) {
            MessageBox(NULL, L"Failed to get console mode", L"Error", MB_ICONERROR);
        } else {
            if (!SetConsoleMode(hStdin, mode | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT)) {
                MessageBox(NULL, L"Failed to set console mode", L"Error", MB_ICONERROR);
            } else {
                MessageBox(NULL, L"Console mode set successfully!", L"Debug", MB_OK);
            }
        }

        // Настройка цветной консоли
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        
        std::cout << "========================================" << std::endl;
        std::cout << "   ОТЛАДКА КАМЕРЫ DARK SOULS REMASTERED   " << std::endl;
        std::cout << "========================================" << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        
        MessageBox(NULL, L"Debug Camera Menu activated!", L"Debug", MB_OK);

        // Более специфичный шаблон: три float (1.0f, 2.0f, 3.0f) и int magic (0x12345678).
        std::string currentPattern = "\x00\x00\x80\x3F"  // 1.0f
            "\x00\x00\x00\x40"  // 2.0f
            "\x00\x00\x40\x40"  // 3.0f
            "\x78\x56\x34\x12"; // 0x12345678 (little-endian)
        std::string currentMask = "xxxxxxxxxxxxxxxx"; // 16 символов 'x'

        std::cout << "[*] Запуск процесса обнаружения ИГРОКА и КАМЕРЫ..." << std::endl;
        std::cout.flush();
        
        // Попробуем автоопределение адресов персонажа и камеры
        GameAddresses gameAddresses = RealAutoDetectAll();
        std::vector<uintptr_t> cameraAddresses = gameAddresses.cameraAddresses;
        std::vector<uintptr_t> playerAddresses = gameAddresses.playerAddresses;
        
        // Если автоопределение не сработало, используем стандартный поиск
        if (cameraAddresses.empty()) {
            std::cout << "[!] Автоопределение не удалось, пробуем стандартный поиск по шаблону..." << std::endl;
            std::cout.flush();
            cameraAddresses = FindCameraAddresses(currentPattern, currentMask);
        }
        
        if (cameraAddresses.empty()) {
            std::cerr << "[X] Адреса камеры не найдены." << std::endl;
            std::cerr << "[*] Это может быть нормально, если игра еще загружается." << std::endl;
            std::cerr << "[*] Попробуйте команды 'scan' или 'autoscan' после загрузки игры." << std::endl;
            std::cout.flush();
            
            // Не выходим, позволяем пользователю попробовать команды
            cameraAddresses.push_back(0); // Dummy address для продолжения
        } else {
            std::cout << "[+] Найдено " << cameraAddresses.size() << " адрес(ов) камеры!" << std::endl;
            for (size_t i = 0; i < std::min(cameraAddresses.size(), size_t(3)); i++) {
                std::cout << "   Адрес " << (i+1) << ": 0x" << std::hex << cameraAddresses[i] << std::endl;
            }
            std::cout.flush();
        }

        // Используем первый найденный адрес камеры или персонажа
        uintptr_t cameraAddress = cameraAddresses.empty() ? 
            (playerAddresses.empty() ? 0 : playerAddresses[0]) : cameraAddresses[0];
        int activeCameraIndex = 0; // Индекс активного адреса
        bool usePlayerAddresses = cameraAddresses.empty(); // Флаг использования адресов персонажа
        
        std::vector<uintptr_t>* currentAddresses = usePlayerAddresses ? &playerAddresses : &cameraAddresses;
        std::string currentType = usePlayerAddresses ? "PLAYER" : "CAMERA";
        
        // Переменные для сохранения позиции
        CameraCoordinates savedPosition = {0.0f, 0.0f, 0.0f, 0};
        bool hasSavedPosition = false;

        // Функция для отображения меню
        auto ShowMenu = [&hConsole]() {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            std::cout << "\n+-------------------------------------+" << std::endl;
            std::cout << "|           КОМАНДЫ КАМЕРЫ           |" << std::endl;
            std::cout << "+-------------------------------------+" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            std::cout << "| 1. showpos   - Показать позицию камеры |" << std::endl;
            std::cout << "| 2. setpos    - Установить позицию камеры  |" << std::endl;
            std::cout << "| 3. addpos    - Добавить смещение к камере |" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
            std::cout << "| 4. scan      - Пересканировать память        |" << std::endl;
            std::cout << "| 5. autoscan  - Автоопределение камеры   |" << std::endl;
            std::cout << "| 6. random    - Случайная тестовая позиция |" << std::endl;
            std::cout << "| 7. arrows    - Управление стрелками    |" << std::endl;
            std::cout << "| 8. switch    - Переключить адрес        |" << std::endl;
            std::cout << "| 9. playertp  - Случайный телепорт игрока|" << std::endl;
            std::cout << "| 10.toggletype- Переключить ИГРОК/КАМЕРА |" << std::endl;
            std::cout << "| 11.save      - Сохранить текущую позицию|" << std::endl;
            std::cout << "| 12.restore   - Восстановить сохр. позицию|" << std::endl;
            std::cout << "| 13.testall   - Протестировать все адреса    |" << std::endl;
            std::cout << "| 14.listall   - Показать все адреса    |" << std::endl;
            std::cout << "| 15.testid    - Протестировать адрес по ID    |" << std::endl;
            std::cout << "| 16.gotoid    - Перейти к адресу по ID   |" << std::endl;
            std::cout << "| 17.pattern   - Изменить шаблон поиска|" << std::endl;
            std::cout << "| 18.showpattern - Показать текущий шаблон|" << std::endl;
            std::cout << "| 19.smarttest - Умный тест всех адресов  |" << std::endl;
            std::cout << "| 20.multiscan - Многофазное сканирование |" << std::endl;
            std::cout << "| 21.analyze   - Детальный анализ адресов |" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << "| 22.exit      - Выход из программы         |" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            std::cout << "+-------------------------------------+" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        };

        ShowMenu();
        
        // Инициализация генератора случайных чисел
        srand(static_cast<unsigned int>(time(nullptr)));

        while (true) {
            std::string input;
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            std::cout << "\n>>> ";
            std::cout.flush(); // Принудительно выводим промпт
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            
            input = ReadConsoleInput();
            if (input.empty()) {
                Sleep(100);
                continue;
            }
            
            std::string trimmed = Trim(input);
            if (trimmed.empty()) {
                Sleep(100);
                continue;
            }

            if (trimmed == "exit")
                break;

            std::istringstream iss(trimmed);
            std::string command;
            iss >> command;

            if (command == "pattern") {
                std::string newHex, newMask;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                std::cout << "Введите новый шаблон в hex (например 3F800000400000004040000078563412): ";
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                newHex = ReadConsoleInput();
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                std::cout << "Введите новую маску (например xxxxxxxxxxxxxxxx): ";
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                newMask = ReadConsoleInput();
                newHex = Trim(newHex);
                newMask = Trim(newMask);
                std::string newPattern = HexStringToBytes(newHex);
                if (newPattern.empty() || newPattern.length() != newMask.length()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Неверный шаблон или маска." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else {
                    currentPattern = newPattern;
                    currentMask = newMask;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Новый шаблон и маска установлены." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "showpattern") {
                std::cout << "Текущий шаблон (hex): ";
                for (unsigned char c : currentPattern) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
                }
                std::cout << std::endl;
                std::cout << "Текущая маска: " << currentMask << std::endl;
            }
            else if (command == "scan") {
                cameraAddresses = FindCameraAddresses(currentPattern, currentMask);
                if (cameraAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Не удалось найти адреса камеры с текущим шаблоном." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Найдено " << cameraAddresses.size() << " адрес(ов) камеры:" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
                    for (size_t i = 0; i < std::min(cameraAddresses.size(), size_t(3)); i++) {
                        std::cout << "   0x" << std::hex << cameraAddresses[i] << std::endl;
                    }
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    cameraAddress = cameraAddresses[0];
                }
            }
            else if (command == "autoscan") {
                cameraAddresses = AutoDetectCameraAddresses();
                if (cameraAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Автоопределение не удалось." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Автоопределено " << cameraAddresses.size() << " адрес(ов) камеры!" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
                    for (size_t i = 0; i < std::min(cameraAddresses.size(), size_t(3)); i++) {
                        std::cout << "   0x" << std::hex << cameraAddresses[i] << std::endl;
                    }
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    cameraAddress = cameraAddresses[0];
                }
            }
            else if (command == "showpos") {
                CameraCoordinates coords = ReadCameraCoordinates(cameraAddress);
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << "[i] Позиция " << currentType << " (Адрес " << (activeCameraIndex + 1) << "/";
                std::cout << currentAddresses->size() << " - 0x" << std::hex << cameraAddress << "):" << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                std::cout << "   X = " << std::dec << std::fixed << std::setprecision(3) << coords.x << std::endl;
                std::cout << "   Y = " << coords.y << std::endl;
                std::cout << "   Z = " << coords.z << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else if (command == "setpos") {
                CameraCoordinates newCoords;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                std::cout << "Введите новые координаты камеры (X Y Z): ";
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                if (ReadThreeFloats(newCoords.x, newCoords.y, newCoords.z)) {
                    newCoords.magic = 0; // Не используем magic
                    WriteCameraCoordinates(cameraAddress, newCoords);
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Позиция камеры обновлена!" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Неверный формат координат!" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "addpos") {
                CameraCoordinates offset;
                std::cout << "Введите смещение для камеры (dX dY dZ): ";
                if (ReadThreeFloats(offset.x, offset.y, offset.z)) {
                    AddCameraCoordinates(cameraAddress, offset);
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Смещение применено." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Неверный формат смещения!" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "random") {
                // Генерируем случайные координаты для тестирования
                CameraCoordinates randomCoords;
                randomCoords.x = (rand() % 200) - 100.0f; // -100 до 100
                randomCoords.y = (rand() % 200) - 100.0f;
                randomCoords.z = (rand() % 200) - 100.0f;
                randomCoords.magic = 0;
                
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << "[*] Testing random position: " << randomCoords.x << ", " << randomCoords.y << ", " << randomCoords.z << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                
                WriteCameraCoordinates(cameraAddress, randomCoords);
                Sleep(100); // Небольшая пауза
                
                // Проверяем результат
                CameraCoordinates result = ReadCameraCoordinates(cameraAddress);
                if (abs(result.x - randomCoords.x) < 0.1f && abs(result.y - randomCoords.y) < 0.1f && abs(result.z - randomCoords.z) < 0.1f) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] SUCCESS! Camera responded to changes!" << std::endl;
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No response. Try 'switch' to change camera address." << std::endl;
                }
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else if (command == "switch") {
                if (currentAddresses->size() > 1) {
                    activeCameraIndex = (activeCameraIndex + 1) % currentAddresses->size();
                    cameraAddress = (*currentAddresses)[activeCameraIndex];
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Switched to " << currentType << " address " << (activeCameraIndex + 1) << "/";
                    std::cout << currentAddresses->size() << " (0x" << std::hex << cameraAddress << ")" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No other " << currentType << " addresses available." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "toggletype") {
                if (!playerAddresses.empty() && !cameraAddresses.empty()) {
                    usePlayerAddresses = !usePlayerAddresses;
                    currentAddresses = usePlayerAddresses ? &playerAddresses : &cameraAddresses;
                    currentType = usePlayerAddresses ? "PLAYER" : "CAMERA";
                    activeCameraIndex = 0;
                    cameraAddress = (*currentAddresses)[0];
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Switched to " << currentType << " mode! ";
                    std::cout << "(" << currentAddresses->size() << " addresses available)" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Both PLAYER and CAMERA addresses needed to toggle." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "playertp") {
                if (!playerAddresses.empty()) {
                    // Сначала сохраняем текущую позицию
                    CameraCoordinates currentPos = ReadCameraCoordinates(playerAddresses[0]);
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "[*] Current position: " << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << std::endl;
                    
                    // Генерируем БЕЗОПАСНЫЕ координаты для телепорта
                    CameraCoordinates tpCoords;
                    // Используем текущую позицию как базу и делаем небольшое смещение
                    tpCoords.x = currentPos.x + ((rand() % 20) - 10.0f); // ±10 от текущей позиции
                    tpCoords.y = currentPos.y + ((rand() % 6) - 3.0f);   // ±3 по высоте
                    tpCoords.z = currentPos.z + ((rand() % 20) - 10.0f); // ±10 от текущей позиции
                    tpCoords.magic = 0;
                    
                    std::cout << "[*] SAFE teleport to: " << tpCoords.x << ", " << tpCoords.y << ", " << tpCoords.z << std::endl;
                    std::cout << "[*] Testing on first address only..." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    // Применяем только к ПЕРВОМУ адресу для безопасности
                    try {
                        WriteCameraCoordinates(playerAddresses[0], tpCoords);
                        Sleep(100);
                        
                        // Проверяем, сработало ли
                        CameraCoordinates result = ReadCameraCoordinates(playerAddresses[0]);
                        if (abs(result.x - tpCoords.x) < 1.0f && abs(result.z - tpCoords.z) < 1.0f) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            std::cout << "[+] SAFE teleport successful! Position changed." << std::endl;
                        } else {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                            std::cout << "[X] No position change detected." << std::endl;
                        }
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                    catch (...) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] Error during teleport! Address may be invalid." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No player addresses found. Run 'autoscan' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "save") {
                if (cameraAddress != 0) {
                    savedPosition = ReadCameraCoordinates(cameraAddress);
                    hasSavedPosition = true;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Position saved: " << savedPosition.x << ", " << savedPosition.y << ", " << savedPosition.z << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No valid address to save from." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "restore") {
                if (hasSavedPosition && cameraAddress != 0) {
                    try {
                        WriteCameraCoordinates(cameraAddress, savedPosition);
                        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                        std::cout << "[+] Position restored to: " << savedPosition.x << ", " << savedPosition.y << ", " << savedPosition.z << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                    catch (...) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] Error during position restore!" << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No saved position available. Use 'save' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "testall") {
                // Тестируем ВСЕ найденные адреса (и player, и camera)
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No addresses found. Run 'autoscan' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "[*] Testing ALL " << allAddresses.size() << " addresses for responsiveness..." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    std::vector<uintptr_t> workingAddresses;
                    
                    for (size_t i = 0; i < allAddresses.size(); i++) {
                        uintptr_t addr = allAddresses[i];
                        
                        try {
                            // Читаем текущие координаты
                            CameraCoordinates current = ReadCameraCoordinates(addr);
                            
                            // БЕЗОПАСНОЕ тестирование - только чтение и очень маленькое изменение
                            std::cout << "   Testing 0x" << std::hex << addr << " - Current: " 
                                     << std::fixed << std::setprecision(1) << current.x << "," << current.y << "," << current.z;
                            
                            // Делаем МИКРОСКОПИЧЕСКОЕ изменение только по X
                            CameraCoordinates test = current;
                            test.x += 0.1f; // Очень маленькое изменение
                            
                            // Сначала проверяем, можем ли мы безопасно писать
                            try {
                                WriteCameraCoordinates(addr, test);
                                Sleep(100); // Больше времени на обработку
                                
                                // Проверяем результат
                                CameraCoordinates result = ReadCameraCoordinates(addr);
                                
                                bool responsive = (abs(result.x - test.x) < 0.2f);
                                
                                // СРАЗУ восстанавливаем исходные координаты
                                WriteCameraCoordinates(addr, current);
                                Sleep(50);
                                
                                std::cout << " -> " << (responsive ? "RESPONSIVE" : "No response") << std::endl;
                                
                                std::string addrType = "UNKNOWN";
                                if (std::find(playerAddresses.begin(), playerAddresses.end(), addr) != playerAddresses.end()) {
                                    addrType = "PLAYER";
                                } else if (std::find(cameraAddresses.begin(), cameraAddresses.end(), addr) != cameraAddresses.end()) {
                                    addrType = "CAMERA";
                                }
                                
                                if (responsive) {
                                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                                    std::cout << "[+] " << addrType << " RESPONSIVE!" << std::endl;
                                    workingAddresses.push_back(addr);
                                }
                                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                                
                            } catch (...) {
                                std::cout << " -> WRITE ERROR" << std::endl;
                            }
                            
                        } catch (...) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                            std::cout << "[E] 0x" << std::hex << addr << " - Error during test" << std::endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                    }
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[+] Found " << workingAddresses.size() << " working addresses!" << std::endl;
                    
                    if (!workingAddresses.empty()) {
                        std::cout << "[*] Switching to first working address..." << std::endl;
                        cameraAddress = workingAddresses[0];
                        
                        // Обновляем тип и индекс
                        if (std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) != playerAddresses.end()) {
                            usePlayerAddresses = true;
                            currentAddresses = &playerAddresses;
                            currentType = "PLAYER";
                            activeCameraIndex = std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) - playerAddresses.begin();
                        } else {
                            usePlayerAddresses = false;
                            currentAddresses = &cameraAddresses;
                            currentType = "CAMERA";
                            activeCameraIndex = std::find(cameraAddresses.begin(), cameraAddresses.end(), cameraAddress) - cameraAddresses.begin();
                        }
                        
                        std::cout << "[+] Now using " << currentType << " address 0x" << std::hex << cameraAddress << std::endl;
                    }
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "listall") {
                // БЕЗОПАСНОЕ просто чтение всех адресов без изменений
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No addresses found. Run 'autoscan' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "[*] Listing ALL " << allAddresses.size() << " addresses (READ-ONLY):" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    for (size_t i = 0; i < allAddresses.size(); i++) {
                        uintptr_t addr = allAddresses[i];
                        
                        try {
                            CameraCoordinates coords = ReadCameraCoordinates(addr);
                            
                            std::string addrType = "UNKNOWN";
                            if (std::find(playerAddresses.begin(), playerAddresses.end(), addr) != playerAddresses.end()) {
                                addrType = "PLAYER";
                                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                            } else if (std::find(cameraAddresses.begin(), cameraAddresses.end(), addr) != cameraAddresses.end()) {
                                addrType = "CAMERA";
                                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
                            }
                            
                            std::cout << "[" << (i+1) << "] " << addrType << " 0x" << std::hex << addr << " = ";
                            std::cout << std::fixed << std::setprecision(2) << coords.x << ", " << coords.y << ", " << coords.z << std::endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                            
                        } catch (...) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                            std::cout << "[" << (i+1) << "] ERROR 0x" << std::hex << addr << " - Cannot read" << std::endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                    }
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "[*] To switch to specific address, use 'switch' or manually test with 'random'/'playertp'" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "testid") {
                // Тестируем конкретный адрес по ID из listall
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No addresses found. Run 'autoscan' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "Enter address ID (1-" << std::hex << allAddresses.size() << "): ";
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    std::string idInput = ReadConsoleInput();
                    int id = 0;
                    try {
                        id = std::stoi(idInput);
                    } catch (...) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] Invalid ID format." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        goto skiptest;
                    }
                    
                    if (id < 1 || id > (int)allAddresses.size()) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] ID out of range (1-" << allAddresses.size() << ")." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    } else {
                        uintptr_t testAddr = allAddresses[id-1];
                        
                        std::string addrType = "UNKNOWN";
                        if (std::find(playerAddresses.begin(), playerAddresses.end(), testAddr) != playerAddresses.end()) {
                            addrType = "PLAYER";
                        } else if (std::find(cameraAddresses.begin(), cameraAddresses.end(), testAddr) != cameraAddresses.end()) {
                            addrType = "CAMERA";
                        }
                        
                        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                        std::cout << "[*] Testing " << addrType << " address [" << id << "] 0x" << std::hex << testAddr << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        
                        try {
                            // Сохраняем текущие координаты
                            CameraCoordinates current = ReadCameraCoordinates(testAddr);
                            std::cout << "Current: " << std::fixed << std::setprecision(2) 
                                     << current.x << ", " << current.y << ", " << current.z << std::endl;
                            
                            // ОЧЕНЬ маленькое безопасное изменение
                            CameraCoordinates test = current;
                            test.x += 0.05f; // Микроскопическое изменение
                            
                            std::cout << "Applying tiny change (+0.05 to X)..." << std::endl;
                            WriteCameraCoordinates(testAddr, test);
                            Sleep(200); // Больше времени
                            
                            // Проверяем результат
                            CameraCoordinates result = ReadCameraCoordinates(testAddr);
                            bool changed = (abs(result.x - current.x) > 0.01f);
                            
                            // НЕМЕДЛЕННО восстанавливаем
                            WriteCameraCoordinates(testAddr, current);
                            Sleep(100);
                            
                            if (changed) {
                                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                                std::cout << "[+] SUCCESS! Address responds to changes!" << std::endl;
                                std::cout << "[+] This address is SAFE to use!" << std::endl;
                            } else {
                                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                                std::cout << "[X] No response - address may be read-only." << std::endl;
                            }
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                            
                        } catch (...) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                            std::cout << "[!] CRASH RISK! This address caused an error!" << std::endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                    }
                }
                skiptest:;
            }
            else if (command == "gotoid") {
                // Переключаемся на конкретный адрес по ID
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] No addresses found. Run 'autoscan' first." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "Enter address ID to switch to (1-" << std::hex << allAddresses.size() << "): ";
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    std::string idInput = ReadConsoleInput();
                    int id = 0;
                    try {
                        id = std::stoi(idInput);
                    } catch (...) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] Invalid ID format." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        goto skipgoto;
                    }
                    
                    if (id < 1 || id > (int)allAddresses.size()) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        std::cout << "[X] ID out of range (1-" << allAddresses.size() << ")." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    } else {
                        cameraAddress = allAddresses[id-1];
                        
                        // Определяем тип и обновляем переменные
                        if (std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) != playerAddresses.end()) {
                            usePlayerAddresses = true;
                            currentAddresses = &playerAddresses;
                            currentType = "PLAYER";
                            activeCameraIndex = std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) - playerAddresses.begin();
                        } else {
                            usePlayerAddresses = false;
                            currentAddresses = &cameraAddresses;
                            currentType = "CAMERA";
                            activeCameraIndex = std::find(cameraAddresses.begin(), cameraAddresses.end(), cameraAddress) - cameraAddresses.begin();
                        }
                        
                        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                        std::cout << "[+] Switched to " << currentType << " address [" << id << "] 0x" << std::hex << cameraAddress << std::endl;
                        std::cout << "[*] Now you can use 'showpos', 'random', 'playertp', etc." << std::endl;
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                }
                skipgoto:;
            }
            else if (command == "smarttest") {
                // Новая команда для умного теста всех адресов
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Адреса не найдены. Сначала выполните 'autoscan'." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "[*] Запуск УМНОГО АНАЛИЗА " << allAddresses.size() << " адресов..." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    std::vector<AddressTestResult> testResults = SmartTestAllAddresses(allAddresses);
                    
                    // Отображение результатов
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\n[+] РЕЗУЛЬТАТЫ УМНОГО АНАЛИЗА:" << std::endl;
                    std::cout << "====================================" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    int playerCount = 0, cameraCount = 0, dynamicCount = 0, staticCount = 0;
                    
                    for (size_t i = 0; i < testResults.size(); i++) {
                        const auto& result = testResults[i];
                        
                        // Цветовое кодирование по типу
                        if (result.type == "ИГРОК") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            playerCount++;
                        } else if (result.type == "КАМЕРА") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                            cameraCount++;
                        } else if (result.type == "ДИНАМИЧЕСКИЙ") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                            dynamicCount++;
                        } else {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                            staticCount++;
                        }
                        
                        std::cout << "[" << (i+1) << "] " << result.type << " 0x" << std::hex << result.address;
                        std::cout << " (изм:" << std::dec << result.changeFrequency;
                        if (result.isResponsive) {
                            std::cout << ", стаб:" << std::fixed << std::setprecision(2) << result.stability;
                        }
                        std::cout << ")" << std::endl;
                        
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        
                        // Показываем только первые 15 результатов для читаемости
                        if (i >= 14) {
                            std::cout << "... и еще " << (testResults.size() - 15) << " адресов" << std::endl;
                            break;
                        }
                    }
                    
                    // Статистика
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\nСТАТИСТИКА:" << std::endl;
                    std::cout << "- Игроков: " << playerCount << std::endl;
                    std::cout << "- Камер: " << cameraCount << std::endl;
                    std::cout << "- Динамических: " << dynamicCount << std::endl;
                    std::cout << "- Статических: " << staticCount << std::endl;
                    
                    // Рекомендации
                    if (playerCount > 0 || cameraCount > 0) {
                        std::cout << "\n[РЕКОМЕНДАЦИЯ] Используйте команду 'gotoid' для переключения на лучший адрес!" << std::endl;
                        
                        // Автоматически переключаемся на лучший адрес
                        if (!testResults.empty() && testResults[0].isResponsive) {
                            cameraAddress = testResults[0].address;
                            
                            // Обновляем тип и индекс
                            if (std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) != playerAddresses.end()) {
                                usePlayerAddresses = true;
                                currentAddresses = &playerAddresses;
                                currentType = "ИГРОК";
                                activeCameraIndex = std::find(playerAddresses.begin(), playerAddresses.end(), cameraAddress) - playerAddresses.begin();
                            } else {
                                usePlayerAddresses = false;
                                currentAddresses = &cameraAddresses;
                                currentType = "КАМЕРА";
                                activeCameraIndex = std::find(cameraAddresses.begin(), cameraAddresses.end(), cameraAddress) - cameraAddresses.begin();
                            }
                            
                            std::cout << "[+] Автоматически переключено на лучший адрес: " << testResults[0].type;
                            std::cout << " 0x" << std::hex << cameraAddress << std::endl;
                        }
                    } else {
                        std::cout << "\n[!] Активные адреса не найдены. Попробуйте больше двигаться в игре." << std::endl;
                    }
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "multiscan") {
                // Многофазное сканирование для точного определения типов адресов
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << "[*] Запуск МНОГОФАЗНОГО СКАНИРОВАНИЯ..." << std::endl;
                std::cout << "[!] Это займет около 30 секунд. Приготовьтесь следовать инструкциям!" << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                
                GameAddresses newAddresses = MultipassAutoDetect();
                
                if (!newAddresses.playerAddresses.empty() || !newAddresses.cameraAddresses.empty()) {
                    // Обновляем найденные адреса
                    playerAddresses = newAddresses.playerAddresses;
                    cameraAddresses = newAddresses.cameraAddresses;
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\n[+] МНОГОФАЗНОЕ СКАНИРОВАНИЕ ЗАВЕРШЕНО!" << std::endl;
                    std::cout << "[+] Обновлено адресов ИГРОКА: " << playerAddresses.size() << std::endl;
                    std::cout << "[+] Обновлено адресов КАМЕРЫ: " << cameraAddresses.size() << std::endl;
                    
                    // Автоматически выбираем лучший адрес
                    if (!playerAddresses.empty()) {
                        usePlayerAddresses = true;
                        currentAddresses = &playerAddresses;
                        currentType = "ИГРОК";
                        activeCameraIndex = 0;
                        cameraAddress = playerAddresses[0];
                        std::cout << "[+] Автоматически выбран адрес ИГРОКА: 0x" << std::hex << cameraAddress << std::endl;
                    } else if (!cameraAddresses.empty()) {
                        usePlayerAddresses = false;
                        currentAddresses = &cameraAddresses;
                        currentType = "КАМЕРА";
                        activeCameraIndex = 0;
                        cameraAddress = cameraAddresses[0];
                        std::cout << "[+] Автоматически выбран адрес КАМЕРЫ: 0x" << std::hex << cameraAddress << std::endl;
                    }
                    
                    std::cout << "[*] Теперь вы можете использовать команды 'showpos', 'setpos', 'smarttest' и другие!" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "\n[X] Многофазное сканирование не обнаружило активных адресов." << std::endl;
                    std::cout << "[*] Убедитесь, что игра загружена и вы находитесь в игровом мире." << std::endl;
                    std::cout << "[*] Попробуйте повторить сканирование, более активно следуя инструкциям." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "analyze") {
                // Детальный анализ адресов для определения точного назначения
                std::vector<uintptr_t> allAddresses;
                allAddresses.insert(allAddresses.end(), playerAddresses.begin(), playerAddresses.end());
                allAddresses.insert(allAddresses.end(), cameraAddresses.begin(), cameraAddresses.end());
                
                if (allAddresses.empty()) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    std::cout << "[X] Адреса не найдены. Сначала выполните 'autoscan' или 'multiscan'." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                } else {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    std::cout << "[*] Запуск ДЕТАЛЬНОГО АНАЛИЗА " << allAddresses.size() << " адресов..." << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    std::vector<ExtendedAddressInfo> analysis = AnalyzeAddressTypes(allAddresses);
                    
                    // Отображение результатов детального анализа
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\n[+] РЕЗУЛЬТАТЫ ДЕТАЛЬНОГО АНАЛИЗА:" << std::endl;
                    std::cout << "================================================" << std::endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    
                    // Группируем по типам для статистики
                    std::map<std::string, int> typeCount;
                    
                    for (size_t i = 0; i < analysis.size() && i < 25; i++) { // Показываем только первые 25
                        const auto& info = analysis[i];
                        
                        // Цветовое кодирование по основному типу
                        if (info.primaryType == "ИГРОК") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                        } else if (info.primaryType == "КАМЕРА") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                        } else if (info.primaryType == "УНИВЕРСАЛЬНЫЙ") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                        } else if (info.primaryType == "ДИНАМИЧЕСКИЙ") {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                        } else {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                        }
                        
                        std::cout << "[" << (i+1) << "] " << info.primaryType << ":" << info.secondaryType;
                        std::cout << " 0x" << std::hex << info.address << std::dec;
                        std::cout << " (изм:" << info.changeFrequency;
                        std::cout << ", стаб:" << std::fixed << std::setprecision(2) << info.stability << ")" << std::endl;
                        
                        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
                        std::cout << "    " << info.detailedType << std::endl;
                        std::cout << "    Средние: " << std::fixed << std::setprecision(1) 
                                  << info.avgX << ", " << info.avgY << ", " << info.avgZ;
                        std::cout << " | Дисперсия: " << std::setprecision(1) 
                                  << info.varX << ", " << info.varY << ", " << info.varZ << std::endl;
                        
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        
                        typeCount[info.primaryType]++;
                        
                        if (i == 14 && analysis.size() > 15) {
                            std::cout << "\n... и еще " << (analysis.size() - 15) << " адресов (используйте 'listall' для полного списка)" << std::endl;
                            break;
                        }
                    }
                    
                    // Статистика и рекомендации
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    std::cout << "\nСТАТИСТИКА АНАЛИЗА:" << std::endl;
                    for (const auto& pair : typeCount) {
                        std::cout << "- " << pair.first << ": " << pair.second << std::endl;
                    }
                    
                    // Автоматические рекомендации
                    std::cout << "\nРЕКОМЕНДАЦИИ:" << std::endl;
                    if (typeCount["ИГРОК"] > 0) {
                        std::cout << "✓ Найдены адреса позиции игрока - используйте их для телепортации" << std::endl;
                    }
                    if (typeCount["КАМЕРА"] > 0) {
                        std::cout << "✓ Найдены адреса камеры - используйте их для свободной камеры" << std::endl;
                    }
                    if (typeCount["УНИВЕРСАЛЬНЫЙ"] > 0) {
                        std::cout << "✓ Найдены универсальные адреса - могут работать для игрока и камеры" << std::endl;
                    }
                    
                    // Автоматически выбираем лучший адрес на основе анализа
                    if (!analysis.empty() && analysis[0].changeFrequency > 0) {
                        const auto& best = analysis[0];
                        cameraAddress = best.address;
                        
                        if (best.primaryType == "ИГРОК") {
                            usePlayerAddresses = true;
                            currentType = "ИГРОК";
                        } else {
                            usePlayerAddresses = false;
                            currentType = "КАМЕРА";
                        }
                        
                        std::cout << "\n[+] Автоматически выбран ЛУЧШИЙ адрес: " << best.primaryType;
                        std::cout << " 0x" << std::hex << best.address << std::endl;
                        std::cout << "[+] Описание: " << best.detailedType << std::endl;
                    }
                    
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else if (command == "arrows") {
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << "[*] Arrow key control mode. ESC to exit." << std::endl;
                std::cout << "[*] Use arrow keys to move camera. WASD for up/down." << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                
                bool arrowMode = true;
                while (arrowMode) {
                    if (_kbhit()) {  // Проверяем нажатие клавиши
                        int key = _getch();
                        CameraCoordinates coords = ReadCameraCoordinates(cameraAddress);
                        CameraCoordinates newCoords = coords;
                        float step = 1.0f;
                        
                        switch (key) {
                            case 72: // Стрелка вверх
                                newCoords.z += step;
                                break;
                            case 80: // Стрелка вниз  
                                newCoords.z -= step;
                                break;
                            case 75: // Стрелка влево
                                newCoords.x -= step;
                                break;
                            case 77: // Стрелка вправо
                                newCoords.x += step;
                                break;
                            case 'w': case 'W':
                                newCoords.y += step;
                                break;
                            case 's': case 'S':
                                newCoords.y -= step;
                                break;
                            case 27: // ESC
                                arrowMode = false;
                                break;
                        }
                        
                        if (arrowMode && (newCoords.x != coords.x || newCoords.y != coords.y || newCoords.z != coords.z)) {
                            WriteCameraCoordinates(cameraAddress, newCoords);
                            std::cout << "\rPos: " << std::fixed << std::setprecision(1) 
                                     << newCoords.x << ", " << newCoords.y << ", " << newCoords.z << "    ";
                        }
                    }
                    Sleep(50);
                }
                std::cout << std::endl << "[+] Exited arrow control mode." << std::endl;
            }
            else {
                std::cout << "Invalid command. Type a command or see menu above." << std::endl;
            }
        }

        std::cout << "Press any key to exit..." << std::endl;
        ReadConsoleInput();

        if (fpOut) fclose(fpOut);
        if (fpIn) fclose(fpIn);
        return 0;
    }

} // extern "C"
