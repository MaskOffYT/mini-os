# MiniOS 3.0 - Informe Técnico y Manual de Arquitectura

Integrantes: Nicolas Loaiza | Juan Pablo Leon

MiniOS 3.0 es un sistema operativo modular y por capas desarrollado con fines académicos en lenguaje C sobre entornos POSIX (Linux/Kali). Este taller implementa mecanismos avanzados de **seguridad basada en roles (RBAC)**, **auditoría centralizada**, **persistencia redundante (RAID 1)** y **virtualización**.

---

## 1. Matriz de Acceso y Principios de Protección

La matriz de seguridad gestiona los permisos de los dominios (roles) sobre los objetos protegidos del sistema mediante codificación de bits (`C`rear, `R`ead, `U`pdate, `D`elete, `E`xec).

### Tabla de la Matriz de Acceso

| ROL | REGISTROS | JUEGOS | PARTIDAS | PROCESOS | MATRIZ | SISTEMA | USUARIOS | BITACORA | VIRTUALIZACION |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **admin** | CRUD (0x0F) | E (0x10) | R (0x02) | CRUD (0x0F) | RU (0x06) | E (0x10) | CRUD (0x0F) | R (0x02) | E (0x10) |
| **operador** | CRU (0x07) | E (0x10) | CRU (0x07) | CRUD (0x0F) | R (0x02) | --- (0x00) | --- (0x00) | --- (0x00) | E (0x10) |
| **invitado** | R (0x02) | E (0x10) | --- (0x00) | R (0x02) | --- (0x00) | --- (0x00) | --- (0x00) | --- (0x00) | --- (0x00) |

### Justificación Basada en Principios de Protección

1. **Mínimo Privilegio**: 
   - El rol `invitado` posee únicamente privilegios de lectura (`R`) en registros y ejecución (`E`) en juegos, restringiendo la capacidad de alterar el sistema o acceder a información sensible.
   - Los operadores pueden gestionar sus propios procesos y partidas, pero no tienen acceso administrativo a usuarios ni a la matriz.
2. **Fallo Seguro (Fail-Safe)**: 
   - Si no existe una sesión activa (`!hay_sesion`) o la celda de la matriz no contiene el bit del derecho requerido, MiniOS deniega el acceso por defecto (`ACCESO DENEGADO`), evitando escalaciones de privilegios imprevistas.
3. **Mediación Completa**: 
   - Cada acción e intento de acceso a un recurso pasa obligatoriamente por `seg_verificar()`, garantizando que ningún componente eluda las reglas de control de acceso y generando automáticamente una traza en la bitácora (`bitacora.log`).
4. **Separación de Privilegios**: 
   - Las operaciones críticas del sistema —como apagar el equipo, modificar la matriz de acceso de seguridad y gestionar cuentas de usuario— están estrictamente reservadas para el rol `admin`. Además, se implementó una guarda de seguridad que impide al administrador quitarse a sí mismo los permisos de lectura y actualización sobre la matriz (`OBJ_MATRIZ`).

---

## 2. Autenticación, Almacenamiento Seguro y Defensa contra Fuerza Bruta

- **Hash con Sal**: Las contraseñas nunca se almacenan en texto plano en el archivo binario `datos/usuarios.dat`. Cada usuario cuenta con un valor aleatorio único (`sal`) combinado con un algoritmo hash didáctico (`djb2`), previniendo ataques mediante tablas arcoíris (*rainbow tables*).
- **Protección de Terminal (`/etc/shadow` style)**: Al ingresar credenciales, se desactiva el eco de la terminal (`~ECHO`), ocultando la contraseña en pantalla.
- **Defensa ante Fuerza Bruta**: Tras tres (`3`) intentos fallidos consecutivos, la cuenta del usuario se bloquea de manera automática. Únicamente el rol de administrador puede desbloquearla desde el submenú de gestión de usuarios.
- **Mensajes Genéricos**: El sistema arroja el mismo mensaje estándar ("Usuario o contraseña incorrectos") sin revelar si la cuenta realmente existe, mitigando ataques de enumeración de usuarios.

---

## 3. Informe de Virtualización

### Especificaciones del Entorno
- **Hipervisor Utilizado**: QEMU/KVM (Tipo 1) / VirtualBox (Tipo 2 según el entorno de prueba).
- **Método de Implementación**: Virtualización asistida por hardware mediante los modos root y non-root del procesador (`VT-x` en Intel / `AMD-V` en AMD) combinados con paginación anidada (`EPT / NPT`).
- **Diferencia en `/proc/cpuinfo` (`grep -cE 'vmx|svm'`)**: 
  - Al ejecutar el comando en el **sistema anfitrión (host)**, el resultado indica la cantidad de núcleos físicos con soporte de virtualización expuestos por el procesador.
  - Al ejecutarlo dentro de la **máquina virtual (guest)**, el resultado suele ser `0` (a menos que se habilite explícitamente la virtualización anidada / *Nested Virtualization*), ya que el hipervisor emula un entorno estándar y oculta las extensiones directas de hardware al sistema invitado a menos que se requiera.
- **Diferencia entre Máquina Virtual (VM) y Contenedor**:
  - Una **VM** virtualiza hardware completo mediante un hipervisor, corriendo su propio kernel de sistema operativo aislado e independiente.
  - Un **Contenedor** (ej. Docker) comparte el kernel del sistema operativo anfitrión y utiliza tecnologías de aislamiento a nivel de espacio de usuario (*namespaces* y *cgroups*), lo que lo hace mucho más ligero pero con menor aislamiento estricto.

---

## 4. Comparativa: Instantánea de VM (*Snapshot*) vs. Partida Guardada (`.sav`)

| Dimensión | Instantánea de VM (Snapshot) | Partida Guardada (`partida_<usuario>.sav`) |
| :--- | :--- | :--- |
| **Nivel de Abstracción** | Nivel de infraestructura / hipervisor (hardware virtualizado). | Nivel de aplicación / lógica del juego en MiniOS. |
| **Estado que Almacena** | Estado completo de la memoria RAM, registros de CPU, discos virtuales y configuración de red en un instante exacto. | Únicamente las variables de estado del juego (puntaje, posición, vidas, nivel). |
| **Persistencia y Portabilidad** | Depende del hipervisor y ocupa gigabytes; requiere congelar el estado completo de la máquina. | Archivo estructurado ligero (pocos bytes) gestionado de forma concurrente por hilos y protegido por mutex. |

---

## 5. Concurrencia (Autoguardado) y Redundancia (RAID 1)

- **9.1 Autoguardado Concurrente**: Un hilo en segundo plano (`pthread`) ejecuta un ciclo de guardado cada 15 segundos. Se utiliza un cerrojo (`pthread_mutex_t`) sobre la estructura de la partida para proteger la sección crítica. 
  - *¿Qué pasaría sin el mutex?* Ocurriría una **condición de carrera**. Mientras el hilo principal actualiza los datos del juego, el hilo de lectura podría capturar una estructura a medio modificar, corrompiendo el archivo de guardado en disco con bytes inconsistentes.
- **9.2 Disco Espejo (RAID 1)**: Todas las operaciones de escritura en el sistema de ficheros se replican de forma simultánea en `minios.disk` y `minios_espejo.disk`. Al arrancar MiniOS, se valida el número mágico del superbloque; si uno de los discos resulta dañado o ausente, el sistema ejecuta una autoreconstrucción mediante un volcado de bloques directo (`copiar_disco`).
