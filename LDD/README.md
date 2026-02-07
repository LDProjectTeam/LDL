# Minecraft Launcher

Кастомный лаунчер для запуска модпаков Minecraft на С++ с Qt.

## Требования

- CMake 3.21+
- Qt 6.x (Core, Gui, Network, Widgets)
- MSVC 2022 (или другой совместимый компилятор C++)
- Visual Studio 2022

## Структура проекта

```
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── config_manager.h/cpp
│   │   ├── downloader.h/cpp
│   │   ├── java_manager.h/cpp
│   │   └── launcher_core.h/cpp
│   └── ui/
│       ├── main_window.h/cpp
│       └── build_selector.h/cpp
├── builds.json (конфигурация сборок)
├── CMakeLists.txt
└── resources/
    └── resources.qrc
```

## Сборка

1. Откройте консоль в папке проекта
2. Создайте папку `build` и перейдите в неё:
   ```bash
   mkdir build
   cd build
   ```
3. Запустите CMake:
   ```bash
   cmake -G "Visual Studio 17 2022" ..
   ```
4. Откройте сгенерированный файл `.sln` в Visual Studio и скомпилируйте

## Конфигурация сборок

Отредактируйте `builds.json` и добавьте ссылки на ваши сборки с Google Drive:

```json
{
  "builds": [
    {
      "id": "build_id",
      "name": "Название сборки",
      "description": "Описание",
      "minecraftVersion": "1.20.1",
      "javaVersion": 17,
      "downloadUrl": "https://drive.google.com/uc?export=download&id=YOUR_FILE_ID",
      "checksum": "hash",
      "sizeBytes": 500000000
    }
  ]
}
```

## Функционал (в разработке)

- [x] Структура проекта
- [ ] UI для выбора сборок
- [ ] Загрузка сборок с Google Drive
- [ ] Управление версиями Java
- [ ] Запуск игры
- [ ] Проверка целостности файлов
- [ ] Автоматическое обновление лаунчера
