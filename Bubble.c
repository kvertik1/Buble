#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <time.h>

#pragma comment(lib, "Comdlg32.lib")

#define MAX_ARRAY 100000
#define MAX_PATH_LEN 260

double source[MAX_ARRAY];  /* исходный массив */
double sorted[MAX_ARRAY];  /* отсортированный массив */
int    n = 0;      /* количество элементов */
int    hasSorted = 0;      /* флаг: была ли сортировка */
char   filePath[MAX_PATH_LEN] = "";

/* форматируем число: целое - без точки, дробное - без экспоненты */
void FormatNumber(double v, char* buf, size_t bufSize)
{
    if (v == floor(v)) {
        sprintf(buf, "%.0f", v);
    }
    else {
        char* dot;
        sprintf(buf, "%.10f", v);
        dot = strchr(buf, '.');
        if (dot) {
            char* end = buf + strlen(buf) - 1;
            while (end > dot + 1 && *end == '0') *end-- = '\0';
        }
    }
}

void PrintArray(const double* arr, int count)
{
    int i;
    char buf[64];
    for (i = 0; i < count; i++) {
        FormatNumber(arr[i], buf, sizeof(buf));
        printf("%s", buf);
        if (i < count - 1) printf(", ");
        if ((i + 1) % 10 == 0 && i < count - 1) printf("\n");
    }
    printf("\n");
}

/* режем строку на числа по разделителям */
int ParseArray(char* text)
{
    int count = 0;
    char* token = strtok(text, ",; \r\n\t");

    while (token) {
        char* end = NULL;
        double value;

        if (count >= MAX_ARRAY) {
            printf("Превышен лимит (%d чисел), загружено %d.\n", MAX_ARRAY, count);
            break;
        }

        value = strtod(token, &end);
        if (end == token || *end != '\0') {
            printf("Пропущен некорректный токен: \"%s\"\n", token);
            token = strtok(NULL, ",; \r\n\t");
            continue;
        }

        source[count++] = value;
        token = strtok(NULL, ",; \r\n\t");
    }

    n = count;
    hasSorted = 0;
    return count;
}

int LoadFromFile(const char* path)
{
    FILE* f;
    long size;
    char* buf;
    size_t readCount;
    int result;

    f = fopen(path, "r");
    if (!f) {
        printf("Ошибка: не удалось открыть файл.\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);

    buf = (char*)calloc(size + 1, 1);
    if (!buf) {
        fclose(f);
        printf("Ошибка: нехватка памяти.\n");
        return 0;
    }

    readCount = fread(buf, 1, size, f);
    fclose(f);
    buf[readCount] = '\0';

    result = ParseArray(buf);
    free(buf);
    return result;
}

void WriteArray(FILE* f, const double* arr, int count)
{
    int i;
    char buf[64];
    for (i = 0; i < count; i++) {
        FormatNumber(arr[i], buf, sizeof(buf));
        if (i > 0) fprintf(f, ", ");
        fprintf(f, "%s", buf);
    }
    fprintf(f, "\n");
}

/* пузырьковая сортировка по возрастанию */
void BubbleSort(const double* src, double* dst, int count,
    long long* comparisons, long long* swaps)
{
    int i, j;
    int swapped;

    for (i = 0; i < count; i++) dst[i] = src[i];
    *comparisons = 0;
    *swaps = 0;

    for (i = 0; i < count - 1; i++) {
        swapped = 0;
        for (j = 0; j < count - i - 1; j++) {
            (*comparisons)++;
            if (dst[j] > dst[j + 1]) {
                double tmp = dst[j];
                dst[j] = dst[j + 1];
                dst[j + 1] = tmp;
                (*swaps)++;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

/* диалог выбора файла для открытия */
int DialogOpenFile(char* outPath)
{
    OPENFILENAMEA ofn;
    char buf[MAX_PATH_LEN] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Текстовые файлы (*.txt;*.csv)\0*.txt;*.csv\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        strcpy(outPath, buf);
        return 1;
    }
    return 0;
}

/* диалог сохранения файла */
int DialogSaveFile(char* outPath)
{
    OPENFILENAMEA ofn;
    char buf[MAX_PATH_LEN] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Текстовые файлы (*.txt)\0*.txt\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.lpstrDefExt = "txt";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn)) {
        strcpy(outPath, buf);
        return 1;
    }
    return 0;
}

void PrintSep(void)
{
    printf("\n--------------------------------------------------------------------\n\n");
}

void MenuLoad(void)
{
    char path[MAX_PATH_LEN] = "";

    printf("Открывается диалог выбора файла...\n");
    if (!DialogOpenFile(path)) {
        printf("Файл не выбран.\n");
        return;
    }

    if (LoadFromFile(path) > 0) {
        strcpy(filePath, path);
        printf("Загружено %d чисел из \"%s\".\n", n, path);
    }
    else {
        printf("Файл не содержит корректных чисел.\n");
    }
}

void MenuShow(void)
{
    if (n == 0) {
        printf("Массив не загружен.\n");
        return;
    }

    printf("Исходный массив (%d эл.):\n", n);
    PrintArray(source, n);

    if (hasSorted) {
        printf("\nОтсортированный массив:\n");
        PrintArray(sorted, n);
    }
}

void MenuSort(void)
{
    long long comparisons, swaps;
    clock_t t1, t2;
    double ms;

    if (n == 0) {
        printf("Сначала загрузите массив (пункт 1).\n");
        return;
    }

    t1 = clock();
    BubbleSort(source, sorted, n, &comparisons, &swaps);
    t2 = clock();

    ms = (double)(t2 - t1) / CLOCKS_PER_SEC;
    hasSorted = 1;

    printf("Сортировка завершена.\n");
    PrintSep();
    printf("Элементов:     %d\n", n);
    printf("Сравнений:     %lld\n", comparisons);
    printf("Перестановок:  %lld\n", swaps);
    printf("Время:         %.6f сек\n", ms);
    PrintSep();
    printf("Нажмите Enter для просмотра отсортированного массива...\n");
    getchar();
    PrintSep();
    printf("Отсортированный массив:\n");
    PrintArray(sorted, n);
    PrintSep();
}

void MenuSave(void)
{
    int choice;
    char path[MAX_PATH_LEN];
    FILE* f;

    if (!hasSorted) {
        printf("Сначала выполните сортировку (пункт 3).\n");
        return;
    }

    printf("Куда сохранить?\n");
    printf("  1. Перезаписать текущий файл (%s)\n",
        filePath[0] ? filePath : "не задан");
    printf("  2. Добавить в конец текущего файла\n");
    printf("  3. Сохранить в новый файл\n");
    printf("Ваш выбор: ");

    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n');
        printf("Некорректный ввод.\n");
        return;
    }
    while (getchar() != '\n');

    /* если текущий файл не задан — идём в диалог */
    if ((choice == 1 || choice == 2) && filePath[0] == '\0') {
        printf("Текущий файл не задан, открывается диалог сохранения...\n");
        choice = 3;
    }

    if (choice == 3) {
        printf("Открывается диалог сохранения...\n");
        if (!DialogSaveFile(path)) {
            printf("Файл не выбран.\n");
            return;
        }
    }
    else {
        strcpy(path, filePath);
    }

    if (choice == 2) {
        f = fopen(path, "a");
        if (!f) { printf("Ошибка открытия файла.\n"); return; }
        fprintf(f, "\n--------------------\n");
        WriteArray(f, sorted, n);
        fclose(f);
    }
    else {
        f = fopen(path, "w");
        if (!f) { printf("Ошибка открытия файла.\n"); return; }
        WriteArray(f, sorted, n);
        fclose(f);
    }

    printf("Сохранено в \"%s\".\n", path);
}

int main(void)
{
    int choice;

    setlocale(LC_ALL, "Russian");

    printf("==========================================\n");
    printf("      Пузырьковая сортировка (Си)\n");
    printf("==========================================\n");

    for (;;) {
        printf("\n");
        PrintSep();
        if (n > 0)
            printf("Загружен массив: %d эл. | файл: %s\n", n, filePath);
        else
            printf("Массив не загружен.\n");
        PrintSep();
        printf("1. Загрузить массив из файла\n");
        printf("2. Показать массив\n");
        printf("3. Сортировать\n");
        printf("4. Сохранить результат\n");
        printf("0. Выход\n");
        printf("Ваш выбор: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Введите цифру от 0 до 4.\n");
            continue;
        }
        while (getchar() != '\n');

        printf("\n");

        switch (choice) {
        case 1: MenuLoad(); break;
        case 2: MenuShow(); break;
        case 3: MenuSort(); break;
        case 4: MenuSave(); break;
        case 0:
            printf("До свидания.\n");
            return 0;
        default:
            printf("Неверный пункт. Введите от 0 до 4.\n");
        }
    }
}