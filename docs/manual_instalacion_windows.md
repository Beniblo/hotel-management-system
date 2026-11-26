---

### 6. Inicialización de Git y verificación de la Parte 1A

Una vez guardados los 5 archivos anteriores, ejecuta estos comandos en la terminal PowerShell de VS Code desde la raíz `hotel-management-system`:

```powershell
# 1. Inicializar repositorio Git (rama main)
git init -b main

# 2. Verificar que Git ignore correctamente archivos de secretos
git check-ignore -v backend/.env frontend/.env backend/config/drogon.config.json

# 3. Revisar estado de archivos rastreados
git status

# 4. Crear primer commit limpio de la Parte 1A
git add .gitignore README.md backend/.env.example frontend/.env.example backend/config/drogon.config.example.json
git commit -m "chore(init): agregar gitignore, plantillas de entorno y configuracion base"
git status