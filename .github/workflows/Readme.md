# 📌 DevSecOps - PlatformIO CI/CD & SonarCloud Analysis

Este repositorio utiliza GitHub Actions para automatizar la integración y entrega continua (CI/CD) con PlatformIO, además de realizar análisis de código con SonarCloud.

## 📌 Configuración del Workflow

El workflow se activa en los siguientes eventos:
- **Push** a las ramas `main`, `src` y cualquier rama dentro de `releases/**`
- **Pull Request** hacia la rama `main`

##  Jobs Definidos

El archivo contiene dos `jobs`:
1. **Pipeline PlatformIO**
2. **SonarCloud Scan**

### 🔹 1. Pipeline PlatformIO
Este job realiza la compilación y análisis estático del código con PlatformIO.

#### 🔧 Pasos
1. **Clonar repositorio**
   - Descarga el código desde GitHub.
2. **Configurar Python**
   - Instala Python 3.x
3. **Instalar PlatformIO**
   - Instala PlatformIO y `pip-audit`.
4. **Comprobar dependencias desactualizadas**
   - Ejecuta `pio pkg outdated`.
5. **Actualizar setuptools**
   - Asegura que `setuptools` esté actualizado.
6. **Auditar dependencias**
   - Usa `pip-audit` para verificar vulnerabilidades.
7. **Compilar con PlatformIO**
   - Ejecuta `pio run` en el entorno `esp32thing`.
8. **Ejecutar Cppcheck**
   - Analiza el código con `cppcheck`.
9. **Aplicar correcciones con Clang-Tidy**
   - Usa `pio check` para corregir errores automáticamente.

### 🔹 2. SonarCloud Scan
Este job ejecuta un análisis de código con SonarCloud para detectar errores y mejorar la calidad del código.

#### 🔧 Pasos
1. **Clonar repositorio**
   - Descarga el código con `fetch-depth: 0` para análisis de historial completo.
2. **Configurar JDK 17**
   - Instala Java 17 con Temurin.
3. **Ejecutar SonarCloud Scan**
   - Usa `sonarcloud-github-action` para analizar el código con los siguientes parámetros:
     - Organiza el proyecto en SonarCloud.
     - Especifica la clave del proyecto.
     - Define la URL de SonarCloud.
     - Incluye los directorios `src, include, lib, test`.
     - Excluye `node_modules`, `.pio`, `.platformio`, `build`, `output`.
4. **Usa secretos para autenticación**
   - Se utiliza `${{ secrets.SONAR_TOKEN }}` para la autenticación segura en SonarCloud.


## 📌 Cómo Usarlo
1. **Configurar secretos en GitHub**
   - En `Settings > Secrets and variables > Actions`, agrega:
     - `SONAR_TOKEN`: Token de autenticación de SonarCloud.

2. **Realizar un commit o pull request**
   - Al hacer `push` a `main`, `src` o cualquier rama dentro de `releases/**`, el workflow se ejecutará automáticamente.

3. **Revisar los resultados en GitHub Actions**
   - Se debe ir a `Actions` en el repositorio para verificar el estado del workflow.

4. **Analizar la calidad del código en SonarCloud**
   - Ingresar a [SonarCloud](https://sonarcloud.io/) y revisar el reporte de análisis de código.


## 📌 Beneficios de este Workflow
✅ Automatiza la compilación y pruebas con PlatformIO.
✅ Detecta vulnerabilidades en dependencias.
✅ Mejora la calidad del código con análisis estático.
✅ Integra SonarCloud para métricas y reportes detallados.
✅ Facilita la integración y entrega continua (CI/CD).

