# 🏫 Sistema Escolar - Mostrar Información Almacenada

Implementación de una API GET para leer los datos del archivo de persistencia y renderizar una tabla dinámica en el frontend consumiendo el JSON con `fetch()`.

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![JavaScript](https://img.shields.io/badge/JavaScript-F7DF1E?style=flat&logo=javascript&logoColor=black)
![JSON](https://img.shields.io/badge/Format-JSON-orange)
![Status](https://img.shields.io/badge/status-completado-success)

---

## 📌 Descripción

El proyecto ahora es un sistema **Full-Stack** completo. En esta actividad se agregó la ruta `GET /api/alumnos` en el servidor en C++, la cual lee el archivo `data/usuarios.txt`, construye un arreglo JSON y lo envía. En el frontend, JavaScript consume este JSON y pinta dinámicamente una tabla HTML con todos los alumnos registrados, actualizando también las tarjetas de resumen.

---

## ✨ Características

- 📤 **API GET `/api/alumnos`:** Devuelve la lista completa de alumnos en formato JSON.
- 📊 **Renderizado dinámico:** La tabla HTML se construye automáticamente a partir de los datos del servidor sin recargar la página.
- 🔄 **Botón "Actualizar":** Permite consultar los datos más recientes del servidor de forma manual.
- ⚡ **Sincronización automática:** Al registrar un alumno nuevo, la tabla se actualiza automáticamente.
- 📈 **Resumen en vivo:** Las tarjetas (Total, Promedio de edades, Último registrado) se calculan en el cliente a partir del JSON.
- 🛡️ **Protección XSS:** Uso de `textContent` en lugar de `innerHTML` para pintar los datos de los usuarios.

---

## 🛠️ Tecnologías

| Tecnología | Uso |
|------------|-----|
| C++17 | Lectura de archivos y construcción manual de JSON |
| Sockets TCP | Servidor HTTP |
| JavaScript | `fetch()` (GET), manipulación del DOM |
| HTML/CSS | Estructura de la tabla y tarjetas de resumen |

---

## 🧠 Conceptos aplicados

### Construcción manual de JSON en C++
- Se recorre el arreglo en memoria (`vector<Alumno>`).
- Se utiliza la función `escaparJSON()` para evitar que caracteres especiales (como comillas o saltos de línea) rompan el formato.
- Se respeta la sintaxis estricta: comas entre elementos, sin comas al final, corchetes `[ ]` para el arreglo y llaves `{ }` para los objetos.

### Flujo de carga en el Frontend
1. Al cargar la página (`DOMContentLoaded`), se ejecuta la función `cargarAlumnos()`.
2. Se hace un `fetch('/api/alumnos')`.
3. El servidor devuelve `application/json`.
4. El cliente convierte la respuesta con `await res.json()`.
5. La función `renderizarTabla()` limpia el `<tbody>` y crea las filas `<tr>` y celdas `<td>` necesarias.
6. La función `actualizarResumenDesdeServidor()` calcula los totales.

---

## 🚀 Cómo probarlo

### 1. Compilar y ejecutar
```bash
g++ servidor.cpp -o servidor.exe -std=c++17 -lws2_32
.\servidor.exe
```

### 2. Verificar la tabla
1. Abre `http://localhost:8080`.
2. Al cargar, verás la tabla "Alumnos registrados" llena con los datos guardados previamente en `data/usuarios.txt`.

### 3. Verificar la sincronización
1. Llena el formulario y presiona **Registrar**.
2. Verás que el mensaje de éxito aparece, y **sin recargar**, el alumno nuevo aparece al final de la tabla y las tarjetas de resumen se actualizan solas.

### 4. Prueba del botón "Actualizar"
1. Abre el archivo `data/usuarios.txt` en tu editor de texto.
2. Modifica manualmente un nombre o borra una línea y guarda el archivo.
3. Regresa al navegador y presiona el botón **Actualizar** de la tabla.
4. El DOM se reconstruirá reflejando los cambios hechos directamente en el archivo de texto.

### 5. Prueba directa de la API
Abre una nueva pestaña en el navegador y visita:
```text
http://localhost:8080/api/alumnos
```
Verás el JSON crudo generado por C++.

---

## 📁 Estructura del proyecto

```text
Sistema-Escolar/
├── servidor.cpp       <-- Backend en C++ (genera el JSON)
├── README.md
├── data/
│   └── usuarios.txt   <-- Base de datos plana (persistencia)
└── Public/
    ├── index.html     <-- Tabla y tarjetas en el DOM
    ├── app.js         <-- Fetch GET y renderizado dinámico
    └── styless.css
```

---

## 📄 Licencia

MIT License
