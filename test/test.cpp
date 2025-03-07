#include <Arduino.h>
#include <unity.h>

// Función solo para validar que me corren los test unitarios
int suma(int a, int b) {
    return a + b;
}

// Prueba a cambiar
void test_suma() {
    TEST_ASSERT_EQUAL(5, suma(2, 3));  // Asegura que 2 + 3 sea igual a 5
}

void setup() {
    // Inicia el puerto serial para imprimir los resultados de la prueba
    Serial.begin(115200);
    delay(2000); // Para asegurarse de que el Serial se inicialice

    UNITY_BEGIN();    
    RUN_TEST(test_suma);  // test de prueba, cambiar luego de las consultas
    UNITY_END();      
}

void loop() {
    // No es necesario para las pruebas, pero se requiere en proyectos de Arduino
}
