#include <ctime> 
#include <iostream> 
using namespace std; 
int main() 
{  
	time_t tt; 
	struct tm* ti; 
	time(&tt); 
	ti = localtime(&tt); 
	cout << "Hola Mundo. Saludo de Juan Guillermo Gutierrez hoy "
		<< asctime(ti); 
	return 0; 
}
