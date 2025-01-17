// торги которые протекают штатно. Запросы к одному источнику с котировками за сегодняшний день в режиме реального времени
// может быть запущен или просто после предторгового наполнения ли после экстренного старта.

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <wctype.h> 
#include <wchar.h> 
#include <time.h> 
#include <libpq-fe.h>
#include <ctype.h>
#include <iso646.h>
#include <stdbool.h>
#include <uchar.h> 
#include <locale.h>
#include <math.h>
#include <threads.h> 
#include <unistd.h>
#include <errno.h> 

#include "глобальные_переменные.h" 
#include "extern_активация.h"
#include "нагрузка_пк.h" 
#include "входные_данные.h" 
#include "ошибки_обработка.h" 
#include "системные_функции.h"
#include "файлы_действия.h"
#include "предстарт_наполнение.h"








//  ================== Текущее, сегодняшнее наполнение данными в шаттном режиме ========================
//  ================== Текущее, сегодняшнее наполнение данными в шаттном режиме ========================        
//  ================== Текущее, сегодняшнее наполнение данными в шаттном режиме ========================

// Этот запрос может использоваться только в однопоточном режиме, так выборка по БД по всем инструментам сразу
    // гораздо быстрее, чем отдельный запрос по 170 разным инструментам 
#if 1==2
int fбд_текущие_котировки_m1(PGconn* связь_сервер_quik) 
{  
        PGresult *результат_запроса;  // локальный    
        struct таймфрэйм_шаблон *адрес_структуры;
        адрес_структуры = глоб_об_таймфрэймы_описание.адрес_в_памяти[0];
        
        int row_number = 0;
        int max_элементов = глоб_об_ключи_запуска.точек_вданных -1;
        
        char secid_из_бд[16]; // RU000A1034U7    RU000A1027E5    RU000A101UK9    RU000A0JNUM1
        
        int номер_secid_предыдущий=0;
        int счётчик_номер = -1;
        int строк_для_изменений=1;
        int i_s=0; // номер блока для указанной акции внутри структуры по м1
        
        int время_эпоха=0;
        
            // получение данных сегодняшних котировок 
        // СТРОК с данными = 1868 (170*3~510). Затрачено всего времени:    0.022 сек    = вечером в 18-00 см внизе страницы расчеты
        char* п_запрос_1 = " select ctid, secid, цена_открытия, "
            " цена_максимум, цена_минимум, цена_закрытия, количво_лотов, "
            " div((EXTRACT(epoch FROM полное_время)),60) as время_эпоха "
            " from _репликация.t_m1_all_ru  "
            " where "
            " время >= current_time - interval '3 minute' " // установлен индекс по "время" (которое без даты, для уменьшения размера и скорости)
            " order by secid asc, время desc ";
        
        результат_запроса = PQexec(связь_сервер_quik, п_запрос_1);   
        
        if (PQresultStatus(результат_запроса) == PGRES_TUPLES_OK) // успешное завершение команды, которая возвращает данные (PGRES_COMMAND_OK=без возврата данных)
        {        
            // УДАЛЕНО
            // ..................

        }
        else if (PQresultStatus(результат_запроса) != PGRES_TUPLES_OK)
        {
            //char стр10[8]=""; 
            PQsetErrorContextVisibility(связь_сервер_quik,  PQSHOW_CONTEXT_ALWAYS);
            PQsetErrorVerbosity(связь_сервер_quik,  PQERRORS_VERBOSE);
            char er_msg[5000]="# ";
            strcat(er_msg, PQerrorMessage(связь_сервер_quik));  // ОДИНАКОВО!!! = strcat(er_msg, PQresultErrorMessage(результат_запроса)); 
            strcat(er_msg, " "); 
            strcat(er_msg, " Таймфрэйм m1'");
            strcat(er_msg, " ");
            //sprintf(стр10, "%i",  шаг_вминутах);
            //strcat(er_msg, стр10);
            strcat(er_msg, "'.");
            
            fо_текст_ошибки(er_msg, -140);    
            printf("\t ОШИБКА:  %s\n", er_msg); 
        }
        
        PQclear(результат_запроса); 
 
    return 0;
}
#endif






#if 1==2
// переносим изменения из м1 в другие минутные таймфрэймы м2-м30 
int fбд_текущее_наполн_цен_mm(char* таймфрэйм_имя, struct таймфрэйм_шаблон *адрес_структуры, int шаг_вминутах) 
{
    volatile struct таймфрэйм_шаблон *адрес_структуры_m1;
    адрес_структуры_m1 = глоб_об_таймфрэймы_описание.адрес_в_памяти[0];
    int max_point = глоб_об_ключи_запуска.точек_вданных;
    
    for (int i=0; i<глоб_список_secid_штук; i++)
    {  
        double ц_откр=0;
        double ц_макс=0;
        double ц_мин=0;
        double ц_закр=0;
        long int акций_шт=0;
        int счётчик=0;

        // сперва проверяем - были ли изменения по цене. Если не было, то данную акцию пропускаем.
        if (false == адрес_структуры_m1[i].нужно_обновить_индикаторы and false == адрес_структуры_m1[i].нужно_пересчитать_индикаторы)
        {
            continue;
        }
        
        
        
        // по м1 создана новая свеча, возможно, что по остальным таймфрэймам так же создаётся новая свеча
        if (true == адрес_структуры_m1[i].нужно_пересчитать_индикаторы 
                and адрес_структуры_m1[i].время_последнее_вминутах >= адрес_структуры[i].время_будущее_вминутах)         
        {            
            // нужно добавить новую  точку в массивы с данными
            // УДАЛЕНО
            // ..................
            адрес_структуры[i].нужно_пересчитать_индикаторы = true; 
        }
        else // новой свечи нет или она не является новой свечкой для указанного таймфрэйма (для м1 новая а для м4 нет)
        {
            // удалено
            // ............
            
            адрес_структуры[i].ard_ц_откр[max_point-1]   = ц_откр;
            адрес_структуры[i].ard_ц_макс[max_point-1]   = ц_макс;
            адрес_структуры[i].ard_ц_мин[max_point-1]    = ц_мин;
            адрес_структуры[i].ard_ц_закр[max_point-1]   = ц_закр;
            адрес_структуры[i].ari_акций_шт[max_point-1] = акций_шт;
            
            адрес_структуры[i].нужно_обновить_индикаторы = true; 
        }
    }
    
    return 0;
}
#endif

