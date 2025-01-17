// шрифт в Libreoffice лучше использовать Monospace Regular или Liberation Mono Regular

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <wctype.h> 
#include <wchar.h> 
#include <time.h> 
#include <libpq-fe.h>   // есть /usr/include/postgresql/
#include <ctype.h>
#include <iso646.h>
#include <stdbool.h>
#include <uchar.h>  //  /usr/include/uchar.h
#include <locale.h>
#include <math.h>
#include <threads.h> 
#include <unistd.h> // для sleep() read()
#include <errno.h> // return EXIT_SUCCESS; 


#include "глобальные_переменные.h" 
#include "main.h" 

#include "запросы_к_бд.h" 
#include "нагрузка_пк.h" 
#include "входные_данные.h" 
#include "ошибки_обработка.h" 
#include "системные_функции.h"
#include "файлы_действия.h"
#include "предстарт_наполнение.h"
#include "потоки.h"
 

 
 
/*
 cd /home/postgres/Документы/дом_зад_otus_курсы__си_/проект_оптимизация_скорости_бд/ускорение_бд_postgresql/build-Debug/bin/
./ускорение_бд_postgresql -db test_si_2023 -host localhost -num_threads 1 \
-port 5434 -user postgres -psw 123 -log_path /home/postgres/z_mmvb_temp/z_ускорение_бд.log -test 2
 */
 
int main(int argc, char **argv)  
{       
    int спешим_на_сек = fс_разница_сек_часовых_поясов(); // насколько секунд локальное время (Москва +2/+3 часа) больше времени UTC (+0)
    time_t время_индикатор;
    bool продолжаем_основн_цикл = true;
    int костыль_счётчик=0;
    
    struct timespec время_начала_программы;
    struct timespec время_старт_блока;
    struct timespec время_стоп;
    double работа_основн_цикла = 0.0; // продолжительность работы основного цикла = 0.03 - 0.24 сек
    timespec_get(&время_начала_программы, TIME_UTC);
    timespec_get(&время_старт_блока, TIME_UTC);
    char текущее_время[100];
    strftime(текущее_время, sizeof текущее_время, "%D %T", gmtime(&время_начала_программы.tv_sec));
    printf("Время старта программы: %s.%09ld UTC\n\n", текущее_время, время_начала_программы.tv_nsec);
    
    int ошибка_номер = 0;
    

    
    
    
    // ======================= анализ ключей на входе ======================
    //
    if (argc < 6) // ошибка, нехватка ключей для запуска программы. Как минимум их 1+8
    {
        printf("\n\nПрограмма НЕ имеет нужного количества параметров на входе. "
                "Сейчас указан(о) %i параметр(ов).\n", argc);
        fн_описание_ключей();
        return 0; // выход из программы
    }

    ошибка_номер = fн_разбор_ключей_из_входа(argc, argv);  
    if (ошибка_номер != 0)
    {
        printf("\n\nПри разборе ключей на входе программы, возникла ошибка. Проверьте ключи еще раз.");
        fн_описание_ключей();
        return 0; // выход из программы
    }
    else 
    {
        if (1== глоб_об_ключи_запуска.вывод_помощи)  
            {fн_описание_ключей();}
    }

    // =======================================================================================     
    
    
    
    
        
        
        
        
    
    // ======================= создаём независимый поток по анализу нагрузки на ПК ======================
    //
   
    thrd_t независ_поток_нагрузки_пк;
    
    ошибка_номер = thrd_create(&независ_поток_нагрузки_пк, (thrd_start_t)fп_нагрузка_пк_постоянно, NULL);
    if (ошибка_номер != thrd_success) 
    {
        printf("ERORR; thrd_create(независ_поток_нагрузки_пк) не создался. Номер ошибки = %i \n", ошибка_номер);
        exit(EXIT_FAILURE);  
    }
    
    // отсоединяем поток - теперь этот поток работает автономно!!!
    ошибка_номер = thrd_detach(независ_поток_нагрузки_пк);          //rc = thrd_join(независ_поток_нагрузки_пк, NULL);
    if (ошибка_номер != thrd_success) 
    {
        printf("ERORR; thrd_detach(независ_поток_нагрузки_пк) не сделался автономным. Ошибки  № %i \n", ошибка_номер);
        exit(EXIT_FAILURE);            
    }

    // =======================================================================================     
    
    
    
    
    
    

                      

    

    
    
    //  ============ Утреннее, предстартовое наполнение данными программы (там своё соединени). ===================
    //             
        // Утреннее, предстартовое наполнение данными программы. Отдельный кусок кода, который:
        // 1) проверяет работоспособность лог журнала
        // 2) утром наполняет глобальные структуры данными
        // 3) Тащит из БД нужные данные (коих до жути много)
    
    timespec_get(&время_старт_блока, TIME_UTC);
        // ВНИМАНИЕ, если время больше 09-59-00, то таймфрэймы по м* (минуткам) в этом блоке не заполняются НЕ ЗАПОЛНЯЮТСЯ!!! 
    ошибка_номер = fб2_предстартов_наполнение();
    
    if (ошибка_номер != 0)
    {
        printf("\n\nУтреннее наполнение предварительными данными для запуска программы НЕ УДАЛОСЬ. \nПрограмма прекращает работу.");
        return -1; // выход из программы 
    }   
    
    fф_сообщ_тесты_срезов_цен("m15", "GAZP", 20);
    //fф_сообщ_текущие_срезы_цен("m15", "GAZP");
    
    timespec_get(&время_стоп, TIME_UTC);    
    printf("Расчёт предварительных настроек всего: %6.2f сек. \n", (время_стоп.tv_sec + (float)время_стоп.tv_nsec/1000000000.0) - (время_старт_блока.tv_sec + (float)время_старт_блока.tv_nsec/1000000000.0) );
    printf("С момента запуска программы прошло: %8.2f сек. \n", (время_стоп.tv_sec + (float)время_стоп.tv_nsec/1000000000.0) - (время_начала_программы.tv_sec + (float)время_начала_программы.tv_nsec/1000000000.0) );
    printf("======================================================================================\n\n");

    // =========================================================================================
    
    
    
    
    
    
    
    
    
     // ====================================    Активация соединений с БД    ========================================
    //        
        // устанавливаем соединение с БД. Как глобальная переменная отказывается работать из-за ошибок 
        // при определении размера (пишет указана неполная структура)!!! 
    PGconn* связь_сервер_quik = fбд_соединение_бд(); // = 0.04-0.055 сек на установку каждого соединения и это уже 1,2 сек на 30 таймфрэймов!!!
    
    if (PQstatus(связь_сервер_quik) != CONNECTION_OK) 
        {  return -10;  }        
    // ================================================================================================================================== 
    
       

    
    

    
    
    
    
    
    
    
    
    
    //  ================== Утренний ПОЛНЫЙ/СТАРТОВЫЙ расчет всех индикаторов по всем таймфрэймам ================
    //  ================== Утренний ПОЛНЫЙ/СТАРТОВЫЙ расчет всех индикаторов по всем таймфрэймам ================
    //  ================== Утренний ПОЛНЫЙ/СТАРТОВЫЙ расчет всех индикаторов по всем таймфрэймам ================
    //    
    timespec_get(&время_старт_блока, TIME_UTC);
    
    // сперва делаем расчёт всех индикаторов
    // запуск стандартизированного блока по расчёту индикаторов
        // мы сюда могли прийти как через эктренное наполнение цен по индикаторам (после сбоя)
        // следующий шаг = первичный (полный) расчет индикаторов по ценам из структур, 
        // после чего мы переходим в штатный режим работы программы
    ошибка_номер = 0; //fб7_полный_расчет индикаторов();


    if (ошибка_номер != 0) 
    {
        printf("\nПолный расчет индикаторов не получился. Программа прекращает работу. \n");
        //return -1; 
    } 

    timespec_get(&время_стоп, TIME_UTC);    
    printf("Расчёт ВСЕХ индикаторов занял: %6.2f сек. \n", (время_стоп.tv_sec + (float)время_стоп.tv_nsec/1000000000.0) - (время_старт_блока.tv_sec + (float)время_старт_блока.tv_nsec/1000000000.0) );
    printf("С момента запуска программы прошло: %8.2f сек. \n", (время_стоп.tv_sec + (float)время_стоп.tv_nsec/1000000000.0) - (время_начала_программы.tv_sec + (float)время_начала_программы.tv_nsec/1000000000.0) );

    fс_дата_время_текст_значение();
    printf("Расчёт всех индикаторов завершили в: %s\n\n", глоб_время_как_текст);
    
    
    
    printf("\033[1mПрограмма запущена с количеством потоков = %i шт. Акций в работе = %i штук. \033[22m \n", 
    глоб_об_ключи_запуска.независимых_потоков, глоб_список_secid_штук);
    fс_сообщение_о_нагрузке(); 
    
    // =========================================================================================================   
    
    
    
    
    
    
    
    
    
    
    
    
    
    //  ========================== Работа программы в штатном режиме до 23-50-00 ========================================
    //    
    
    // временный указатель на массив из структур по указанному таймфрэйму, который передайтся во внешние функции 
    // struct таймфрэйм_шаблон *адрес_структуры; 
    
        /* акции, основная сексия фондового рынка
            основные торги с 09-50-00 до 18-39-59 (23-49-59)
            с 09-50-00 до 09-59-31 можно выставлять заявки, по которым формируется первая цена на 09-59-31 
            с 09-59-31 начинаются сами торги до 18-39-59
            вечерняя А3 с 18-40-00 до 18-45-00 сделки делать нельзя, но можно выставлять заявки
            с 18-45-00 до 18-50-00 можно делатать сделки по одной установленной цене (на основе предыдущих 5 минут). Другие цены отклоняются.
            с 19-00-01 до 19-04-59 - можно выставлять заявки
            с 19-05-00 до 23-49-59 - вечерняя торговая сессия                */ 
        // пауза до 09-59-32
        
    время_индикатор =  time(NULL) - time(NULL)%(24*3600) + (time_t)(10*3600 - 30 - спешим_на_сек); // > 09-59-30 по местному времени
    if (time(NULL) < время_индикатор)
    {
       // пауза до 09-59-00
        printf("Пауза в работе программы на %li секунд до 09-59-30 \n", (long int) время_индикатор-time(NULL));
        fс_пауза_вработе_программы(время_индикатор-time(NULL), 0.0); 
    }
       
    printf("\n\nЗапустили основной блок робота. -->> РОБОТ <<-- \n");

                
        // c 18-40-00 по местному времени  до 18-45-00
    время_индикатор =  (time(NULL) - time(NULL)%(24*3600)) + (time_t)(19*3600 - 20*60 +1 - спешим_на_сек); // время начала вечерней паузы на 5 минут

    
    //struct таймфрэйм_шаблон *адрес_структуры_анализ; // КОСТЫЛЬ!!!!!
    while (true == продолжаем_основн_цикл)
    {
        timespec_get(&время_старт_блока, TIME_UTC);
        
        // получение текущих цен по м1 и запись их в структуры   
        // этот код получает текущие котировки и обновляет все индикаторы в режиме реального времени
                /*
                ошибка_номер = fбд_текущие_котировки_m1(связь_сервер_quik);
                if (ошибка_номер != 0) 
                {
                    printf("\nПри обработке текущих котировок из БД по m1 произошла ошибка. \n");
                } 
                

                // переносим изменения из м1 в другие минутные таймфрэймы, // минутки с 02 по 30 включительно.  
                for (int номер=1; номер<11; номер++) 
                {
                    адрес_структуры = глоб_об_таймфрэймы_описание.адрес_в_памяти[номер];
                    ошибка_номер = fбд_текущее_наполн_цен_mm(
                                            глоб_об_таймфрэймы_описание.список_имен[номер],     
                                            адрес_структуры,                                   
                                            глоб_об_таймфрэймы_описание.шаг_вминутах[номер]);  
                    if (ошибка_номер !=0) 
                    {
                        printf(" Ошибка при переносе текущих данных из м1 в другие таймфрэймы.");
                        PQfinish(связь_сервер_quik);
                        return -1; 
                    }
                    fс_пауза_вработе_программы(0, 0.001);
                }       */
        
        
        // промежуточные расчеты индикаторов, если добавилась новая точка времени в таймфрэйм, то полный перерасчёт индикаторов
        //................

        

        
        // техническая пауза в работе программы (простои самой ММВБ биржи)  
        if (time(NULL) > время_индикатор)
        {
                // после 18-40-00
            if (time(NULL)%3600 > 40*60 and time(NULL)%3600 < 45*60 
                and time(NULL) < (time(NULL)-time(NULL)%(24*3600) + (time_t)(19*3600 - спешим_на_сек)))
            {
                printf("Пауза в работе программы на 5 минут до 18-45-00 \n");
                fс_пауза_вработе_программы(60*5, 0.0);
                
                    // следующая пауза в работе программы с 19-00-00 до 19-04-30
                время_индикатор =  (time(NULL) - time(NULL)%(24*3600)) + (time_t)(18*3600 +50*60 - спешим_на_сек);  
            } 
            
                // время после 18-50-00)
            if (time(NULL)%3600 > 50*60) 
            {
                // до 20-00
                if (time(NULL) < ((time(NULL)-time(NULL)%(24*3600)) + (time_t)(19*3600 - спешим_на_сек)))
                {
                    int aaa=time(NULL)-time(NULL)%(24*3600) + (time_t)(19*3600 + 4*60 +30- спешим_на_сек)-time(NULL);
                    
                    printf("Пауза в работе программы на 14,5 минут до 19-04-30 \n");
                    fс_пауза_вработе_программы(aaa, 0.0);

                        // следующая стоп метка в 23-50-00
                    время_индикатор =  ((time(NULL)-time(NULL)%(24*3600)) + (time_t)(24*3600 - 10*60 - спешим_на_сек));
                }
                
                // после 23-50
                if (time(NULL) > ((time(NULL)-time(NULL)%(24*3600)) + (time_t)(23*3600 + 50*60 - спешим_на_сек)))
                {
                    printf("Время 23-50. Программа завершила работу. \n");
                    продолжаем_основн_цикл = false; 
                    break;
                }
            }
        }
        
        timespec_get(&время_стоп, TIME_UTC);    
        работа_основн_цикла=(время_стоп.tv_sec + время_стоп.tv_nsec/1000000000.0) - (время_старт_блока.tv_sec + время_старт_блока.tv_nsec/1000000000.0);            
        
            // контрольный вывод сообщения о работе
        if ( 0== (time(NULL)+1) % 300 ) 
        {
            костыль_счётчик ++;
            if (0 == костыль_счётчик %2)
            {
                fс_сообщение_о_нагрузке(); 
                fф_сообщ_текущие_срезы_цен("m15", "GAZP");
                //fф_сообщ_тесты_срезов_цен("m15", "GAZP", 20);
                
                printf("\t\tЦикл занял: %6.4f сек. \n\n", работа_основн_цикла);
            }
        }

        fс_пауза_вработе_программы(0, 0.51-работа_основн_цикла); // основной цикл частота = 1 раз в секунду. чаще смысла нет вообще!
        
    } // while (true == продолжаем_основн_цикл)
    
     // =======================================================================================================      













    // =========================== ЗАВЕРШЕНИЕ РАБОТЫ ПРОГРАММЫ =================================================      
    //
    PQfinish(связь_сервер_quik); // закрытие подключения к БД и очистка памяти от подключения    
    
    timespec_get(&время_стоп, TIME_UTC);
    fс_дата_время_текст_значение();
    printf("-->>> Местное время завершения программы: %s\n", глоб_время_как_текст);
    printf("Затрачено всего времени: %2.6f\n", (время_стоп.tv_sec + (float)время_стоп.tv_nsec/1000000000.0) - (время_начала_программы.tv_sec + (float)время_начала_программы.tv_nsec/1000000000.0) );


    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m1);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m2);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m3);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m4);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m5);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m6);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m10);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m12);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m15);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m20);
    free((struct таймфрэйм_шаблон*) gl_ar_структ_таймфрэйм_m30);
    
    return 0; 
    
} 
