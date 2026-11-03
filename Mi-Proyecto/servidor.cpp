#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <ctime>
#include <vector>
#include <sstream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socket_t = SOCKET;
  #define CLOSE_SOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  using socket_t = int;
  #define CLOSE_SOCKET close
  #define INVALID_SOCKET (-1)
#endif

using namespace std;
namespace fs = std::filesystem;

const int PUERTO = 8080;
const string CARPETA_PUBLICA = "Public";
const string ARCHIVO_DATOS = "data/usuarios.txt";

struct Alumno {
    string nombre;
    int edad = 0;
    string matricula;
    string correo;
};

vector<Alumno> alumnos;

void cargarAlumnos() {
    alumnos.clear();
    ifstream archivo(ARCHIVO_DATOS);
    if (!archivo.is_open()) {
        cout << "[DATA] No hay archivo previo. Arrancando vacio." << endl;
        return;
    }

    string linea;
    while (getline(archivo, linea)) {
        if (linea.empty()) continue;

        stringstream ss(linea);
        string nombre, edadStr, matricula, correo;

        getline(ss, nombre, '|');
        getline(ss, edadStr, '|');
        getline(ss, matricula, '|');
        getline(ss, correo, '|');

        if (nombre.empty() || edadStr.empty() || matricula.empty()) {
            cerr << "[DATA] Linea invalida ignorada: " << linea << endl;
            continue;
        }

        try {
            int edad = stoi(edadStr);
            alumnos.push_back({nombre, edad, matricula, correo});
        } catch (...) {
            cerr << "[DATA] Edad no numerica en: " << linea << endl;
        }
    }

    archivo.close();
    cout << "[DATA] " << alumnos.size() << " alumnos cargados." << endl;
}

bool guardarAlumnos() {
    ofstream archivo(ARCHIVO_DATOS);
    if (!archivo.is_open()) {
        cerr << "[DATA] No se pudo abrir " << ARCHIVO_DATOS << " para escritura" << endl;
        return false;
    }

    for (const auto& a : alumnos) {
        archivo << a.nombre << "|" << a.edad << "|" << a.matricula << "|" << a.correo << "\n";
    }

    archivo.close();
    return true;
}

string escaparJSON(const string& s) {
    string r;
    for (char c : s) {
        if (c == '"') r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else if (c == '\t') r += "\\t";
        else r += c;
    }
    return r;
}

string alumnosAJSON() {
    string json = "[";
    for (size_t i = 0; i < alumnos.size(); i++) {
        if (i > 0) json += ",";
        const auto& a = alumnos[i];
        json += "{";
        json += "\"nombre\":\"" + escaparJSON(a.nombre) + "\",";
        json += "\"edad\":" + to_string(a.edad) + ",";
        json += "\"matricula\":\"" + escaparJSON(a.matricula) + "\",";
        json += "\"correo\":\"" + escaparJSON(a.correo) + "\"";
        json += "}";
    }
    json += "]";
    return json;
}

string leerArchivo(const string& ruta) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo.is_open()) return "";
    string contenido((istreambuf_iterator<char>(archivo)), istreambuf_iterator<char>());
    return contenido;
}

string obtenerMIME(const string& ruta) {
    size_t punto = ruta.find_last_of('.');
    if (punto == string::npos) return "application/octet-stream";

    string ext = ruta.substr(punto);
    static const map<string, string> mimes = {
        {".html", "text/html; charset=utf-8"}, {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"}, {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"}, {".txt", "text/plain; charset=utf-8"},
        {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
        {".gif", "image/gif"}, {".svg", "image/svg+xml"}, {".ico", "image/x-icon"}
    };

    auto it = mimes.find(ext);
    return (it != mimes.end()) ? it->second : "application/octet-stream";
}

string construirRespuesta(int codigo, const string& mime, const string& cuerpo) {
    string estado;
    if (codigo == 200) estado = "200 OK";
    else if (codigo == 400) estado = "400 Bad Request";
    else if (codigo == 404) estado = "404 Not Found";
    else if (codigo == 405) estado = "405 Method Not Allowed";
    else if (codigo == 409) estado = "409 Conflict";
    else estado = "500 Internal Server Error";

    return "HTTP/1.1 " + estado + "\r\n"
         + "Content-Type: " + mime + "\r\n"
         + "Content-Length: " + to_string(cuerpo.size()) + "\r\n"
         + "Connection: close\r\n"
         + "\r\n"
         + cuerpo;
}

string extraerValorJSON(const string& cuerpo, const string& clave) {
    string buscar = "\"" + clave + "\"";
    size_t pos = cuerpo.find(buscar);
    if (pos == string::npos) return "";

    size_t dosPuntos = cuerpo.find(':', pos + buscar.size());
    if (dosPuntos == string::npos) return "";

    size_t i = dosPuntos + 1;
    while (i < cuerpo.size() && isspace((unsigned char)cuerpo[i])) i++;

    if (i < cuerpo.size() && cuerpo[i] == '"') {
        size_t fin = cuerpo.find('"', i + 1);
        if (fin == string::npos) return "";
        return cuerpo.substr(i + 1, fin - i - 1);
    }

    size_t fin = i;
    while (fin < cuerpo.size() && cuerpo[fin] != ',' && cuerpo[fin] != '}') fin++;
    string valor = cuerpo.substr(i, fin - i);
    while (!valor.empty() && isspace((unsigned char)valor.back())) valor.pop_back();
    return valor;
}

Alumno parsearAlumno(const string& cuerpo) {
    Alumno a;
    a.nombre    = extraerValorJSON(cuerpo, "nombre");
    a.matricula = extraerValorJSON(cuerpo, "matricula");
    a.correo    = extraerValorJSON(cuerpo, "correo");

    string edadStr = extraerValorJSON(cuerpo, "edad");
    try { a.edad = stoi(edadStr); } catch (...) { a.edad = 0; }

    return a;
}

string leerPeticionCompleta(socket_t cliente) {
    string peticion;
    char buffer[4096];

    while (peticion.find("\r\n\r\n") == string::npos) {
        int bytes = recv(cliente, buffer, sizeof(buffer), 0);
        if (bytes <= 0) return peticion;
        peticion.append(buffer, bytes);
    }

    size_t posCabecerasFin = peticion.find("\r\n\r\n");
    string cabeceras = peticion.substr(0, posCabecerasFin);

    int contentLength = 0;
    size_t p = cabeceras.find("Content-Length:");
    if (p != string::npos) {
        size_t fin = cabeceras.find("\r\n", p);
        if (fin == string::npos) fin = cabeceras.size();
        string valor = cabeceras.substr(p + 15, fin - (p + 15));
        size_t i = valor.find_first_not_of(" \t");
        if (i != string::npos) valor = valor.substr(i);
        try { contentLength = stoi(valor); } catch (...) { contentLength = 0; }
    }

    size_t bodyInicio = posCabecerasFin + 4;
    size_t bytesCuerpo = peticion.size() - bodyInicio;

    while (bytesCuerpo < (size_t)contentLength) {
        int bytes = recv(cliente, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        peticion.append(buffer, bytes);
        bytesCuerpo = peticion.size() - bodyInicio;
    }

    return peticion;
}

string extraerCuerpo(const string& peticion) {
    size_t pos = peticion.find("\r\n\r\n");
    if (pos == string::npos) return "";
    return peticion.substr(pos + 4);
}

string manejarAPI(const string& ruta, const string& metodo, const string& cuerpo) {
    if (metodo == "GET") {
        if (ruta == "/api/hola") return construirRespuesta(200, "text/plain; charset=utf-8", "Hola desde C++");
        if (ruta == "/api/version") return construirRespuesta(200, "text/plain; charset=utf-8", "Servidor v1.0.0");
        if (ruta == "/api/status") return construirRespuesta(200, "text/plain; charset=utf-8", "OK");
        if (ruta == "/api/hora") {
            time_t ahora = time(nullptr);
            tm* local = localtime(&ahora);
            char buffer[64];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
            return construirRespuesta(200, "text/plain; charset=utf-8", string(buffer));
        }
        if (ruta == "/api/alumnos") {
            string json = alumnosAJSON();
            cout << "[GET] /api/alumnos -> " << alumnos.size() << " registros" << endl;
            return construirRespuesta(200, "application/json; charset=utf-8", json);
        }
    } 
    else if (metodo == "POST") {
        if (ruta == "/api/alumnos") {
            Alumno a = parsearAlumno(cuerpo);

            if (a.nombre.empty() || a.matricula.empty() || a.edad <= 0) {
                return construirRespuesta(400, "text/plain; charset=utf-8", "Datos invalidos");
            }

            for (const auto& reg : alumnos) {
                if (reg.matricula == a.matricula) {
                    return construirRespuesta(409, "text/plain; charset=utf-8", "La matricula ya existe");
                }
            }

            alumnos.push_back(a);

            if (!guardarAlumnos()) {
                alumnos.pop_back();
                return construirRespuesta(500, "text/plain; charset=utf-8", "Error al guardar en disco");
            }

            cout << "\n--- POST recibido ---" << endl;
            cout << "Nombre:    " << a.nombre << endl;
            cout << "Edad:      " << a.edad << endl;
            cout << "Matricula: " << a.matricula << endl;
            cout << "Correo:    " << a.correo << endl;
            cout << "---------------------\n" << endl;

            string msg = "Bienvenido " + a.nombre + ", matricula " + a.matricula;
            return construirRespuesta(200, "text/plain; charset=utf-8", msg);
        }
    }

    return construirRespuesta(404, "text/plain; charset=utf-8", "Endpoint no encontrado");
}

string extraerRuta(const string& peticion) {
    size_t finLinea = peticion.find("\r\n");
    string primera = (finLinea != string::npos) ? peticion.substr(0, finLinea) : peticion;
    size_t p1 = primera.find(' ');
    if (p1 == string::npos) return "/";
    size_t p2 = primera.find(' ', p1 + 1);
    if (p2 == string::npos) return "/";
    return primera.substr(p1 + 1, p2 - p1 - 1);
}

string extraerMetodo(const string& peticion) {
    size_t p1 = peticion.find(' ');
    if (p1 == string::npos) return "GET";
    return peticion.substr(0, p1);
}

string normalizarRuta(string ruta) {
    size_t q = ruta.find('?');
    if (q != string::npos) ruta = ruta.substr(0, q);
    size_t h = ruta.find('#');
    if (h != string::npos) ruta = ruta.substr(0, h);
    if (ruta == "/" || ruta.empty()) ruta = "/index.html";
    return ruta;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "Error al inicializar Winsock" << endl;
        return 1;
    }
#endif

    fs::create_directories("data");
    cargarAlumnos();

    socket_t servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor == INVALID_SOCKET) {
        cerr << "Error al crear el socket" << endl;
        return 1;
    }

    sockaddr_in direccion{};
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(PUERTO);

    if (bind(servidor, (sockaddr*)&direccion, sizeof(direccion)) != 0) {
        cerr << "Error en bind (puerto ocupado)" << endl;
        CLOSE_SOCKET(servidor);
        return 1;
    }

    if (listen(servidor, 10) != 0) {
        cerr << "Error en listen" << endl;
        CLOSE_SOCKET(servidor);
        return 1;
    }

    cout << "Servidor escuchando en http://localhost:" << PUERTO << endl;

    while (true) {
        sockaddr_in clienteDir{};
#ifdef _WIN32
        int tam = sizeof(clienteDir);
#else
        socklen_t tam = sizeof(clienteDir);
#endif

        socket_t cliente = accept(servidor, (sockaddr*)&clienteDir, &tam);
        if (cliente == INVALID_SOCKET) continue;

        string peticion = leerPeticionCompleta(cliente);
        if (peticion.empty()) {
            CLOSE_SOCKET(cliente);
            continue;
        }

        string metodo = extraerMetodo(peticion);
        string rutaOriginal = extraerRuta(peticion);
        string cuerpo = extraerCuerpo(peticion);
        
        bool esAPI = (rutaOriginal.rfind("/api/", 0) == 0);

        if (esAPI) {
            cout << "[API] " << metodo << " " << rutaOriginal << endl;
            string respuesta = manejarAPI(rutaOriginal, metodo, cuerpo);
            send(cliente, respuesta.c_str(), respuesta.size(), 0);
            CLOSE_SOCKET(cliente);
            continue;
        }

        string ruta = normalizarRuta(rutaOriginal);
        cout << "[REQ] " << ruta;

        if (ruta.find("..") != string::npos) {
            string resp = construirRespuesta(400, "text/plain; charset=utf-8", "Ruta no permitida");
            send(cliente, resp.c_str(), resp.size(), 0);
            cout << " -> [400] Bad Request" << endl;
            CLOSE_SOCKET(cliente);
            continue;
        }

        string rutaDisco = CARPETA_PUBLICA + ruta;

        if (!fs::exists(rutaDisco) || !fs::is_regular_file(rutaDisco)) {
            string cuerpo404 = "<h1>404 Not Found</h1><p>Recurso no encontrado</p>";
            string resp = construirRespuesta(404, "text/html; charset=utf-8", cuerpo404);
            send(cliente, resp.c_str(), resp.size(), 0);
            cout << " -> [404] Not Found" << endl;
            CLOSE_SOCKET(cliente);
            continue;
        }

        string contenido = leerArchivo(rutaDisco);
        string mime = obtenerMIME(rutaDisco);
        string resp = construirRespuesta(200, mime, contenido);
        
        send(cliente, resp.c_str(), resp.size(), 0);
        cout << " -> [200] OK (" << mime << ")" << endl;
        CLOSE_SOCKET(cliente);
    }

    CLOSE_SOCKET(servidor);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}