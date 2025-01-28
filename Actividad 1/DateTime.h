#ifndef DATETIME_H
#define DATETIME_H
#include <ctime>
#include <iostream>
#include <string>

class DateTime {
public:
    DateTime() {
        time_t tt;
        ti = localtime(&tt);
    }

    void showDateTime() const {
        std::cout << "Hola Mundo. Saludo de Juan Guillermo Gutierrez hoy " << asctime(ti);
    }

private:
    struct tm* ti;
};

#endif
