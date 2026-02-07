#include "i18n.h"

#include <QHash>

namespace {

QString currentLang = "en";
bool initialized = false;
QHash<QString, QHash<QString, QString>> dicts;

void addDict(const QString &lang, const QHash<QString, QString> &map)
{
    dicts.insert(lang, map);
}

void ensureInit()
{
    if (initialized) {
        return;
    }
    initialized = true;

    QHash<QString, QString> ru;
    ru.insert("LD Launcher", "LD Launcher");
    ru.insert("Builds", "Сборки");
    ru.insert("Actions", "Действия");
    ru.insert("Select a build to see details", "Выберите сборку, чтобы увидеть детали");
    ru.insert("Download", "Скачать");
    ru.insert("Downloading", "Скачивание");
    ru.insert("Play", "Играть");
    ru.insert("Play with console", "Запуск с CMD");
    ru.insert("Stop", "Остановить");
    ru.insert("Remove", "Удалить");
    ru.insert("Delete from device", "Удалить с устройства");
    ru.insert("Remove from my builds", "Удалить из моих сборок");
    ru.insert("Delete local files for this build?\n\n%1", "Удалить локальные файлы этой сборки?\n\n%1");
    ru.insert("Remove this build from your list?\n\n%1", "Удалить эту сборку из списка?\n\n%1");
    ru.insert("Add", "Добавить");
    ru.insert("Buy", "Купить");
    ru.insert("Update", "Обновить");
    ru.insert("Settings", "Настройки");
    ru.insert("Support", "Поддержка");
    ru.insert("Log out", "Выйти");
    ru.insert("Music", "Музыка");
    ru.insert("Enable music", "Включить музыку");
    ru.insert("Music volume", "Громкость музыки");
    ru.insert("Verification", "Проверка");
    ru.insert("Admin", "Админ");
    ru.insert("Memory: Xms %1 MB | Xmx %2 MB (Total %3 MB)", "Память: Xms %1 МБ | Xmx %2 МБ (Всего %3 МБ)");
    ru.insert("Memory: Auto (Xms %1 MB | Xmx %2 MB) (Total %3 MB)",
              "Память: Авто (Xms %1 МБ | Xmx %2 МБ) (Всего %3 МБ)");
    ru.insert("Name: %1\nMinecraft: %2\nLoader: %3\nJava: %4\nSize: %5 MB\nPrice: %6\n\n%7",
              "Название: %1\nMinecraft: %2\nЗагрузчик: %3\nJava: %4\nРазмер: %5 МБ\nЦена: %6\n\n%7");
    ru.insert("Free", "Бесплатно");
    ru.insert("Paid $%1", "Платно: $%1");
    ru.insert("Locked", "Нет доступа");
    ru.insert("No image", "Без изображения");
    ru.insert("No tags", "Нет тегов");
    ru.insert("All tags", "Все теги");
    ru.insert("Loader: ?", "Загрузчик: ?");
    ru.insert("Loader: %1", "Загрузчик: %1");
    ru.insert("Unknown", "Неизвестно");
    ru.insert("Select a build first.", "Сначала выберите сборку.");
    ru.insert("Build ID is missing.", "Не указан ID сборки.");
    ru.insert("Download URL is missing.", "Ссылка на скачивание отсутствует.");
    ru.insert("Files downloaded.\nMinecraft + Fabric + Java 17 + build are ready.\n\nFolder: %1",
              "Файлы скачаны.\nMinecraft, Fabric, Java 17 и сборка готовы.\n\nПапка: %1");
    ru.insert("All files are already downloaded.\n\nFolder: %1", "Все файлы уже скачаны.\n\nПапка: %1");
    ru.insert("Failed to download: %1", "Не удалось скачать: %1");
    ru.insert("Download failed.", "Скачивание не удалось.");
    ru.insert("Failed to create directory for %1", "Не удалось создать папку для %1");
    ru.insert("Failed to write file: %1", "Не удалось записать файл: %1");
    ru.insert("Failed: %1", "Ошибка: %1");
    ru.insert("Failed to write file.", "Не удалось записать файл.");
    ru.insert("Failed to extract.", "Не удалось распаковать.");
    ru.insert("Archive not found: %1", "Архив не найден: %1");
    ru.insert("Downloaded file is not a ZIP archive: %1", "Скачанный файл не является ZIP-архивом: %1");
    ru.insert("Failed to extract archive: %1", "Не удалось распаковать архив: %1");
    ru.insert("Invalid version manifest.", "Неверный манифест версии.");
    ru.insert("Minecraft version not found: %1", "Версия Minecraft не найдена: %1");
    ru.insert("Invalid version JSON.", "Неверный JSON версии.");
    ru.insert("Invalid Fabric loader list.", "Неверный список загрузчиков Fabric.");
    ru.insert("Fabric loader version not found.", "Версия загрузчика Fabric не найдена.");
    ru.insert("Invalid Fabric profile JSON.", "Неверный JSON профиля Fabric.");
    ru.insert("Fabric profile id missing.", "Отсутствует идентификатор профиля Fabric.");
    ru.insert("File not found: %1", "Файл не найден: %1");
    ru.insert("Failed to read file: %1", "Не удалось прочитать файл: %1");
    ru.insert("Invalid JSON: %1", "Неверный JSON: %1");
    ru.insert("InternetOpen failed", "Не удалось открыть интернет-сессию");
    ru.insert("InternetOpenUrl failed: %1", "Не удалось открыть URL: %1");
    ru.insert("HTTP error %1: %2", "HTTP ошибка %1: %2");
    ru.insert("InternetReadFile failed", "Не удалось прочитать данные из сети");
    ru.insert("WinInet not available", "WinInet недоступен");
    ru.insert("No internet connection. Please check your network.",
              "Нет подключения к интернету. Проверьте сеть.");
    ru.insert("Folder not found: %1", "Папка не найдена: %1");
    ru.insert("Failed to open file: %1", "Не удалось открыть файл: %1");
    ru.insert("Failed to read resource: %1", "Не удалось прочитать ресурс: %1");
    ru.insert("This build is paid. Please contact support.", "Эта сборка платная. Свяжитесь с поддержкой.");
    ru.insert("Payment failed.", "Ошибка оплаты.");
    ru.insert("Build folder not found. Make sure you downloaded the build.\n\nExe: %1\nExpected: %2",
              "Папка сборки не найдена. Убедитесь, что сборка скачана.\n\nExe: %1\nОжидалось: %2");
    ru.insert("Minecraft files not found. Download the build first.", "Файлы Minecraft не найдены. Сначала скачайте сборку.");
    ru.insert("Java not found. Download Java 17 first.", "Java не найдена. Сначала скачайте Java 17.");
    ru.insert("Fabric version not found.\n\nVersions dir: %1\nFound: %2",
              "Версия Fabric не найдена.\n\nПапка versions: %1\nНайдено: %2");
    ru.insert("Main class not found.", "Главный класс не найден.");
    ru.insert("Minecraft client jar missing.", "Отсутствует файл client.jar Minecraft.");
    ru.insert("Failed to launch Minecraft.", "Не удалось запустить Minecraft.");
    ru.insert("Game is already running.", "Игра уже запущена.");
    ru.insert("Remove build and delete local files?\n\n%1", "Удалить сборку и локальные файлы?\n\n%1");
    ru.insert("Update this build? It will re-download all files.\n\n%1",
              "Обновить эту сборку? Все файлы будут загружены заново.\n\n%1");
    ru.insert("Preparing downloads...", "Подготовка загрузок...");
    ru.insert("Starting...", "Запуск...");
    ru.insert("Stopping...", "Остановка...");
    ru.insert("Running...", "Работает...");
    ru.insert("Exited with code %1", "Завершено с кодом %1");
    ru.insert("Process crashed.", "Процесс завершился с ошибкой.");
    ru.insert("Console", "Консоль");
    ru.insert("Close", "Закрыть");
    ru.insert("Saving version JSON", "Сохранение JSON версии");
    ru.insert("Downloading Minecraft client", "Скачивание Minecraft клиента");
    ru.insert("Downloading library", "Скачивание библиотеки");
    ru.insert("Downloading native", "Скачивание нативных библиотек");
    ru.insert("Extracting native", "Распаковка нативных библиотек");
    ru.insert("Saving asset index", "Сохранение индекса ресурсов");
    ru.insert("Downloading asset", "Скачивание ресурса");
    ru.insert("Saving Fabric profile", "Сохранение профиля Fabric");
    ru.insert("Downloading Fabric library", "Скачивание библиотеки Fabric");
    ru.insert("Downloading Java 17", "Скачивание Java 17");
    ru.insert("Extracting Java 17", "Распаковка Java 17");
    ru.insert("Downloading build", "Скачивание сборки");
    ru.insert("Extracting build", "Распаковка сборки");
    ru.insert("Installing core module", "Установка системного модуля");
    ru.insert("Downloading", "Скачивание");
    ru.insert("Extracting", "Распаковка");
    ru.insert("Writing", "Запись");
    ru.insert("File: %1", "Файл: %1");
    ru.insert("To: %1", "Куда: %1");
    ru.insert("Build Catalog", "Каталог сборок");
    ru.insert("Select a build", "Выберите сборку");
    ru.insert("Checkout opened in your browser.\nAfter payment, reopen the catalog to refresh access.",
              "Оплата открыта в браузере.\nПосле оплаты заново откройте каталог, чтобы обновить доступ.");
    ru.insert("This build is locked.", "Эта сборка недоступна.");
    ru.insert("Invalid payment response.", "Неверный ответ оплаты.");
    ru.insert("Invalid download response.", "Неверный ответ на скачивание.");
    ru.insert("Invalid catalog response.", "Неверный ответ каталога.");
    ru.insert("Invalid response from server.", "Неверный ответ сервера.");
    ru.insert("Missing access token.", "Отсутствует токен доступа.");
    ru.insert("Missing launch token.", "Отсутствует токен запуска.");
    ru.insert("Missing device id.", "Отсутствует ID устройства.");
    ru.insert("Invalid launch token response.", "Неверный ответ токена запуска.");
    ru.insert("Failed to get launch token.", "Не удалось получить токен запуска.");
    ru.insert("Device not authorized.", "Устройство не авторизовано.");
    ru.insert("Launch token expired.", "Токен запуска истёк.");
    ru.insert("Launch token invalid.", "Токен запуска недействителен.");
    ru.insert("Enter a valid email.", "Введите корректный email.");
    ru.insert("Password must be at least 6 characters.", "Пароль должен быть не менее 6 символов.");
    ru.insert("Passwords do not match.", "Пароли не совпадают.");
    ru.insert("Register", "Регистрация");
    ru.insert("Log in", "Вход");
    ru.insert("Already have an account? Log in", "Уже есть аккаунт? Войти");
    ru.insert("No account? Register", "Нет аккаунта? Зарегистрироваться");
    ru.insert("Sign in with Google", "Войти через Google");
    ru.insert("Waiting for Google login...", "Ожидаем вход через Google...");
    ru.insert("Google login failed.", "Вход через Google не удался.");
    ru.insert("Google login timed out. Please try again.", "Вход через Google не завершен. Попробуйте снова.");
    ru.insert("Email", "Email");
    ru.insert("Password", "Пароль");
    ru.insert("Confirm", "Подтверждение");
    ru.insert("Account", "Аккаунт");
    ru.insert("Server", "Сервер");
    ru.insert("Backend not found. Keep the backend folder next to the launcher or set LAUNCHER_API_URL.",
              "Backend не найден. Держите папку backend рядом с лаунчером или задайте LAUNCHER_API_URL.");
    ru.insert("Server is not running. Please start the backend and try again.", "Сервер не запущен. Запустите backend и попробуйте снова.");
    ru.insert("Server is not responding. Please try again later.", "Сервер не отвечает. Попробуйте позже.");
    ru.insert("An account with this email already exists.", "Аккаунт с этим email уже существует.");
    ru.insert("Invalid email or password.", "Неверный email или пароль.");
    ru.insert("Invalid input. Check email and password.", "Неверный ввод. Проверьте email и пароль.");
    ru.insert("Server error. Please try again later.", "Ошибка сервера. Попробуйте позже.");
    ru.insert("Network error: %1", "Сетевая ошибка: %1");
    ru.insert("Build not found.", "Сборка не найдена.");
    ru.insert("Access denied. Please log in and try again.", "Доступ запрещен. Войдите и попробуйте снова.");
    ru.insert("Invalid request. Please try again.", "Неверный запрос. Попробуйте снова.");
    ru.insert("Unknown error.", "Неизвестная ошибка.");
    ru.insert("Core module not found.", "Системный модуль не найден.");
    ru.insert("Failed to install core module.", "Не удалось установить системный модуль.");
    ru.insert("Failed to write auth file.", "Не удалось записать файл авторизации.");
    ru.insert("General", "Общие");
    ru.insert("Theme", "Тема");
    ru.insert("Font", "Шрифт");
    ru.insert("Enable animations", "Включить анимации");
    ru.insert("Animate background", "Анимировать фон");
    ru.insert("Language", "Язык");
    ru.insert("Search builds", "Поиск сборок");
    ru.insert("Memory", "Память");
    ru.insert("Performance", "Производительность");
    ru.insert("%1 MB | Minecraft %2 | %3 | Java %4", "%1 МБ | Minecraft %2 | %3 | Java %4");
    ru.insert("Total RAM: %1 MB\nAvailable: %2 MB\nRecommended: Xms %3 MB, Xmx %4 MB",
              "ОЗУ: %1 МБ\nДоступно: %2 МБ\nРекомендуется: Xms %3 МБ, Xmx %4 МБ");
    ru.insert("Min memory (Xms, MB)", "Мин. память (Xms, МБ)");
    ru.insert("Max memory (Xmx, MB)", "Макс. память (Xmx, МБ)");
    ru.insert("Use performance JVM profile", "Профиль JVM для производительности");
    ru.insert("GPU tips", "Подсказки по GPU");
    ru.insert("Windows: Settings → System → Display → Graphics → add javaw.exe or LDL.exe → set High performance → Save.",
              "Windows: Параметры → Система → Дисплей → Графика → добавить javaw.exe или LDL.exe → Высокая производительность → Сохранить.");
    ru.insert("Extra JVM arguments", "Доп. аргументы JVM");
    ru.insert("Warning: Xmx exceeds 80% of RAM. The system may stutter.",
              "Внимание: Xmx больше 80% ОЗУ. Система может тормозить.");
    ru.insert("e.g. -XX:+UseG1GC", "например -XX:+UseG1GC");
    ru.insert("Use recommended args", "Рекомендуемые аргументы");
    ru.insert("Auto memory (recommended)", "Авто-память (рекомендуется)");
    ru.insert("Yes", "Да");
    ru.insert("No", "Нет");
    ru.insert("Cancel", "Отмена");
    ru.insert("Save", "Сохранить");
    ru.insert("Add Build", "Добавить сборку");
    ru.insert("New Build", "Новая сборка");
    ru.insert("Build ID", "ID сборки");
    ru.insert("Repo (owner/repo)", "Репозиторий (owner/repo)");
    ru.insert("Asset name", "Имя файла");
    ru.insert("Asset name (optional)", "Имя файла (необязательно)");
    ru.insert("Update from latest", "Обновить из latest");
    ru.insert("Update successful.", "Обновление выполнено.");
    ru.insert("Update failed: %1", "Ошибка обновления: %1");
    ru.insert("Tip: keep asset name stable for auto-updates.", "Совет: имя файла должно быть стабильным.");
    ru.insert("Fill in all fields.", "Заполните все поля.");
    ru.insert("User email", "Почта пользователя");
    ru.insert("Reset device", "Сбросить устройство");
    ru.insert("Reset device binding", "Сброс привязки устройства");
    ru.insert("Device binding reset.", "Привязка устройства сброшена.");
    ru.insert("Device not bound.", "Устройство не привязано.");
    ru.insert("Unknown", "Неизвестно");
    ru.insert("User not found.", "Пользователь не найден.");
    ru.insert("Invalid email.", "Некорректная почта.");
    ru.insert("Use asset name as build name", "Использовать имя файла как название сборки");
    ru.insert("Name", "Название");
    ru.insert("Description", "Описание");
    ru.insert("Minecraft Version", "Версия Minecraft");
    ru.insert("Java Version", "Версия Java");
    ru.insert("Download URL", "Ссылка на скачивание");
    ru.insert("Checksum (sha256)", "Контрольная сумма (sha256)");
    ru.insert("Checksum must be a SHA-256 hex string (64 chars).",
              "Контрольная сумма должна быть SHA-256 (64 символа).");
    ru.insert("Size", "Размер");
    ru.insert("Image URL", "Ссылка на картинку");
    ru.insert("Tags (comma)", "Теги (через запятую)");
    ru.insert("Progress: %1 / %2 MB", "Прогресс: %1 / %2 МБ");
    ru.insert("Downloaded %1 MB", "Скачано %1 МБ");
    ru.insert("Speed: %1 MB/s", "Скорость: %1 МБ/с");
    ru.insert("ETA: %1", "Осталось: %1");
    ru.insert("ETA: --", "Осталось: --");
    ru.insert("Files: %1 / %2", "Файлы: %1 / %2");
    ru.insert("Verify files", "Проверить файлы");
    ru.insert("Change Java path", "Путь к Java");
    ru.insert("Select Java", "Выбор Java");
    ru.insert("Invalid Java path.", "Неверный путь к Java.");
    ru.insert("Custom Java path not found. Please update it.", "Путь к Java не найден. Укажите другой.");
    ru.insert("Java path", "Путь к Java");
    ru.insert("Missing", "нет");
    ru.insert("Not found", "не найдено");
    ru.insert("Verifying files...", "Проверка файлов...");
    ru.insert("Verification finished", "Проверка завершена");
    ru.insert("All files are OK.", "Все файлы в порядке.");
    ru.insert("Missing files: %1", "Отсутствуют файлы: %1");
    ru.insert("Corrupted files: %1", "Повреждены файлы: %1");
    ru.insert("Missing build files: %1", "Отсутствуют файлы сборки: %1");
    ru.insert("Corrupted build files: %1", "Повреждены файлы сборки: %1");
    ru.insert("Repairing files...", "Восстановление файлов...");
    ru.insert("Downloading missing file", "Скачивание отсутствующего файла");
    ru.insert("Downloading build archive", "Скачивание архива сборки");
    ru.insert("Extracting build archive", "Распаковка архива сборки");
    ru.insert("Restoring build files", "Восстановление файлов сборки");
    ru.insert("Some files cannot be repaired automatically.", "Некоторые файлы нельзя восстановить автоматически.");
    ru.insert("Missing files were downloaded.", "Отсутствующие файлы были скачаны.");
    ru.insert("Build files were restored.", "Файлы сборки восстановлены.");
    ru.insert("Build manifest not found. Reinstall the build to enable verification.",
              "Манифест сборки не найден. Переустановите сборку для проверки.");
    ru.insert("Import build", "Импорт сборки");
    ru.insert("Build imported.", "Сборка импортирована.");
    ru.insert("Build import failed.", "Не удалось импортировать сборку.");
    ru.insert("Download speed limit (KB/s, 0 = unlimited)", "Лимит скорости (КБ/с, 0 = без ограничений)");
    ru.insert("Select language", "Выберите язык");
    ru.insert("Choose your language", "Выберите язык");
    ru.insert("email@gmail.com", "email@gmail.com");

    QHash<QString, QString> uk;
    uk.insert("LD Launcher", "LD Launcher");
    uk.insert("Builds", "Збірки");
    uk.insert("Actions", "Дії");
    uk.insert("Select a build to see details", "Виберіть збірку, щоб побачити деталі");
    uk.insert("Download", "Завантажити");
    uk.insert("Downloading", "Завантаження");
    uk.insert("Play", "Грати");
    uk.insert("Play with console", "Запуск з CMD");
    uk.insert("Stop", "Зупинити");
    uk.insert("Remove", "Видалити");
    uk.insert("Delete from device", "Видалити з пристрою");
    uk.insert("Remove from my builds", "Видалити з моїх збірок");
    uk.insert("Delete local files for this build?\n\n%1", "Видалити локальні файли цієї збірки?\n\n%1");
    uk.insert("Remove this build from your list?\n\n%1", "Видалити цю збірку зі списку?\n\n%1");
    uk.insert("Add", "Додати");
    uk.insert("Buy", "Купити");
    uk.insert("Update", "Оновити");
    uk.insert("Settings", "Налаштування");
    uk.insert("Support", "Підтримка");
    uk.insert("Log out", "Вийти");
    uk.insert("Music", "Музика");
    uk.insert("Enable music", "Увімкнути музику");
    uk.insert("Music volume", "Гучність музики");
    uk.insert("Verification", "Перевірка");
    uk.insert("Admin", "Адмін");
    uk.insert("Memory: Xms %1 MB | Xmx %2 MB (Total %3 MB)", "Пам'ять: Xms %1 МБ | Xmx %2 МБ (Всього %3 МБ)");
    uk.insert("Memory: Auto (Xms %1 MB | Xmx %2 MB) (Total %3 MB)",
              "Пам'ять: Авто (Xms %1 МБ | Xmx %2 МБ) (Всього %3 МБ)");
    uk.insert("Name: %1\nMinecraft: %2\nLoader: %3\nJava: %4\nSize: %5 MB\nPrice: %6\n\n%7",
              "Назва: %1\nMinecraft: %2\nЗавантажувач: %3\nJava: %4\nРозмір: %5 МБ\nЦіна: %6\n\n%7");
    uk.insert("Free", "Безкоштовно");
    uk.insert("Paid $%1", "Платно: $%1");
    uk.insert("Locked", "Немає доступу");
    uk.insert("No image", "Без зображення");
    uk.insert("No tags", "Немає тегів");
    uk.insert("All tags", "Усі теги");
    uk.insert("Loader: ?", "Завантажувач: ?");
    uk.insert("Loader: %1", "Завантажувач: %1");
    uk.insert("Unknown", "Невідомо");
    uk.insert("Select a build first.", "Спочатку виберіть збірку.");
    uk.insert("Build ID is missing.", "Не вказано ID збірки.");
    uk.insert("Download URL is missing.", "Посилання на завантаження відсутнє.");
    uk.insert("Files downloaded.\nMinecraft + Fabric + Java 17 + build are ready.\n\nFolder: %1",
              "Файли завантажено.\nMinecraft, Fabric, Java 17 та збірка готові.\n\nПапка: %1");
    uk.insert("All files are already downloaded.\n\nFolder: %1", "Усі файли вже завантажено.\n\nПапка: %1");
    uk.insert("Failed to download: %1", "Не вдалося завантажити: %1");
    uk.insert("Download failed.", "Завантаження не вдалося.");
    uk.insert("Failed to create directory for %1", "Не вдалося створити папку для %1");
    uk.insert("Failed to write file: %1", "Не вдалося записати файл: %1");
    uk.insert("Failed: %1", "Помилка: %1");
    uk.insert("Failed to write file.", "Не вдалося записати файл.");
    uk.insert("Failed to extract.", "Не вдалося розпакувати.");
    uk.insert("Archive not found: %1", "Архів не знайдено: %1");
    uk.insert("Downloaded file is not a ZIP archive: %1", "Завантажений файл не є ZIP-архівом: %1");
    uk.insert("Failed to extract archive: %1", "Не вдалося розпакувати архів: %1");
    uk.insert("Invalid version manifest.", "Невірний маніфест версії.");
    uk.insert("Minecraft version not found: %1", "Версію Minecraft не знайдено: %1");
    uk.insert("Invalid version JSON.", "Невірний JSON версії.");
    uk.insert("Invalid Fabric loader list.", "Невірний список завантажувачів Fabric.");
    uk.insert("Fabric loader version not found.", "Версію завантажувача Fabric не знайдено.");
    uk.insert("Invalid Fabric profile JSON.", "Невірний JSON профілю Fabric.");
    uk.insert("Fabric profile id missing.", "Відсутній ідентифікатор профілю Fabric.");
    uk.insert("File not found: %1", "Файл не знайдено: %1");
    uk.insert("Failed to read file: %1", "Не вдалося прочитати файл: %1");
    uk.insert("Invalid JSON: %1", "Невірний JSON: %1");
    uk.insert("InternetOpen failed", "Не вдалося відкрити інтернет-сесію");
    uk.insert("InternetOpenUrl failed: %1", "Не вдалося відкрити URL: %1");
    uk.insert("HTTP error %1: %2", "HTTP помилка %1: %2");
    uk.insert("InternetReadFile failed", "Не вдалося прочитати дані з мережі");
    uk.insert("WinInet not available", "WinInet недоступний");
    uk.insert("No internet connection. Please check your network.",
              "Немає підключення до інтернету. Перевірте мережу.");
    uk.insert("Folder not found: %1", "Папку не знайдено: %1");
    uk.insert("Failed to open file: %1", "Не вдалося відкрити файл: %1");
    uk.insert("Failed to read resource: %1", "Не вдалося прочитати ресурс: %1");
    uk.insert("This build is paid. Please contact support.", "Ця збірка платна. Зверніться до підтримки.");
    uk.insert("Payment failed.", "Помилка оплати.");
    uk.insert("Build folder not found. Make sure you downloaded the build.\n\nExe: %1\nExpected: %2",
              "Папку збірки не знайдено. Переконайтесь, що збірку завантажено.\n\nExe: %1\nОчікувалось: %2");
    uk.insert("Minecraft files not found. Download the build first.", "Файли Minecraft не знайдено. Спочатку завантажте збірку.");
    uk.insert("Java not found. Download Java 17 first.", "Java не знайдено. Спочатку завантажте Java 17.");
    uk.insert("Fabric version not found.\n\nVersions dir: %1\nFound: %2",
              "Версію Fabric не знайдено.\n\nПапка versions: %1\nЗнайдено: %2");
    uk.insert("Main class not found.", "Головний клас не знайдено.");
    uk.insert("Minecraft client jar missing.", "Відсутній файл client.jar Minecraft.");
    uk.insert("Failed to launch Minecraft.", "Не вдалося запустити Minecraft.");
    uk.insert("Game is already running.", "Гра вже запущена.");
    uk.insert("Remove build and delete local files?\n\n%1", "Видалити збірку та локальні файли?\n\n%1");
    uk.insert("Update this build? It will re-download all files.\n\n%1",
              "Оновити цю збірку? Усі файли буде завантажено знову.\n\n%1");
    uk.insert("Preparing downloads...", "Підготовка завантажень...");
    uk.insert("Starting...", "Запуск...");
    uk.insert("Stopping...", "Зупинка...");
    uk.insert("Running...", "Працює...");
    uk.insert("Exited with code %1", "Завершено з кодом %1");
    uk.insert("Process crashed.", "Процес завершився з помилкою.");
    uk.insert("Console", "Консоль");
    uk.insert("Close", "Закрити");
    uk.insert("Saving version JSON", "Збереження JSON версії");
    uk.insert("Downloading Minecraft client", "Завантаження Minecraft клієнта");
    uk.insert("Downloading library", "Завантаження бібліотеки");
    uk.insert("Downloading native", "Завантаження нативних бібліотек");
    uk.insert("Extracting native", "Розпакування нативних бібліотек");
    uk.insert("Saving asset index", "Збереження індексу ресурсів");
    uk.insert("Downloading asset", "Завантаження ресурсу");
    uk.insert("Saving Fabric profile", "Збереження профілю Fabric");
    uk.insert("Downloading Fabric library", "Завантаження бібліотеки Fabric");
    uk.insert("Downloading Java 17", "Завантаження Java 17");
    uk.insert("Extracting Java 17", "Розпакування Java 17");
    uk.insert("Downloading build", "Завантаження збірки");
    uk.insert("Extracting build", "Розпакування збірки");
    uk.insert("Installing core module", "Встановлення системного модуля");
    uk.insert("Downloading", "Завантаження");
    uk.insert("Extracting", "Розпакування");
    uk.insert("Writing", "Запис");
    uk.insert("File: %1", "Файл: %1");
    uk.insert("To: %1", "Куди: %1");
    uk.insert("Build Catalog", "Каталог збірок");
    uk.insert("Select a build", "Виберіть збірку");
    uk.insert("Checkout opened in your browser.\nAfter payment, reopen the catalog to refresh access.",
              "Оплату відкрито в браузері.\nПісля оплати знову відкрийте каталог, щоб оновити доступ.");
    uk.insert("This build is locked.", "Ця збірка недоступна.");
    uk.insert("Invalid payment response.", "Невірна відповідь оплати.");
    uk.insert("Invalid download response.", "Невірна відповідь на завантаження.");
    uk.insert("Invalid catalog response.", "Невірна відповідь каталогу.");
    uk.insert("Invalid response from server.", "Невірна відповідь сервера.");
    uk.insert("Missing access token.", "Відсутній токен доступу.");
    uk.insert("Missing launch token.", "Відсутній токен запуску.");
    uk.insert("Missing device id.", "Відсутній ID пристрою.");
    uk.insert("Invalid launch token response.", "Невірна відповідь токена запуску.");
    uk.insert("Failed to get launch token.", "Не вдалося отримати токен запуску.");
    uk.insert("Device not authorized.", "Пристрій не авторизований.");
    uk.insert("Launch token expired.", "Токен запуску прострочено.");
    uk.insert("Launch token invalid.", "Токен запуску недійсний.");
    uk.insert("Enter a valid email.", "Введіть коректний email.");
    uk.insert("Password must be at least 6 characters.", "Пароль має бути не менше 6 символів.");
    uk.insert("Passwords do not match.", "Паролі не співпадають.");
    uk.insert("Register", "Реєстрація");
    uk.insert("Log in", "Вхід");
    uk.insert("Already have an account? Log in", "Вже є акаунт? Увійти");
    uk.insert("No account? Register", "Немає акаунта? Зареєструватися");
    uk.insert("Sign in with Google", "Увійти через Google");
    uk.insert("Waiting for Google login...", "Очікуємо вхід через Google...");
    uk.insert("Google login failed.", "Вхід через Google не вдався.");
    uk.insert("Google login timed out. Please try again.", "Вхід через Google не завершено. Спробуйте ще раз.");
    uk.insert("Email", "Email");
    uk.insert("Password", "Пароль");
    uk.insert("Confirm", "Підтвердження");
    uk.insert("Account", "Акаунт");
    uk.insert("Server", "Сервер");
    uk.insert("Backend not found. Keep the backend folder next to the launcher or set LAUNCHER_API_URL.",
              "Backend не знайдено. Тримайте папку backend поряд із лаунчером або задайте LAUNCHER_API_URL.");
    uk.insert("Server is not running. Please start the backend and try again.", "Сервер не запущено. Запустіть backend і спробуйте знову.");
    uk.insert("Server is not responding. Please try again later.", "Сервер не відповідає. Спробуйте пізніше.");
    uk.insert("An account with this email already exists.", "Акаунт з цим email вже існує.");
    uk.insert("Invalid email or password.", "Невірний email або пароль.");
    uk.insert("Invalid input. Check email and password.", "Невірні дані. Перевірте email та пароль.");
    uk.insert("Server error. Please try again later.", "Помилка сервера. Спробуйте пізніше.");
    uk.insert("Network error: %1", "Мережева помилка: %1");
    uk.insert("Build not found.", "Збірку не знайдено.");
    uk.insert("Access denied. Please log in and try again.", "Доступ заборонено. Увійдіть і спробуйте знову.");
    uk.insert("Invalid request. Please try again.", "Невірний запит. Спробуйте знову.");
    uk.insert("Unknown error.", "Невідома помилка.");
    uk.insert("Core module not found.", "Системний модуль не знайдено.");
    uk.insert("Failed to install core module.", "Не вдалося встановити системний модуль.");
    uk.insert("Failed to write auth file.", "Не вдалося записати файл авторизації.");
    uk.insert("General", "Загальні");
    uk.insert("Theme", "Тема");
    uk.insert("Font", "Шрифт");
    uk.insert("Enable animations", "Увімкнути анімації");
    uk.insert("Animate background", "Анімувати фон");
    uk.insert("Language", "Мова");
    uk.insert("Search builds", "Пошук збірок");
    uk.insert("Memory", "Пам'ять");
    uk.insert("Performance", "Продуктивність");
    uk.insert("%1 MB | Minecraft %2 | %3 | Java %4", "%1 МБ | Minecraft %2 | %3 | Java %4");
    uk.insert("Total RAM: %1 MB\nAvailable: %2 MB\nRecommended: Xms %3 MB, Xmx %4 MB",
              "ОЗП: %1 МБ\nДоступно: %2 МБ\nРекомендується: Xms %3 МБ, Xmx %4 МБ");
    uk.insert("Min memory (Xms, MB)", "Мін. пам'ять (Xms, МБ)");
    uk.insert("Max memory (Xmx, MB)", "Макс. пам'ять (Xmx, МБ)");
    uk.insert("Use performance JVM profile", "Профіль JVM для продуктивності");
    uk.insert("GPU tips", "Підказки по GPU");
    uk.insert("Windows: Settings → System → Display → Graphics → add javaw.exe or LDL.exe → set High performance → Save.",
              "Windows: Параметри → Система → Дисплей → Графіка → додати javaw.exe або LDL.exe → Висока продуктивність → Зберегти.");
    uk.insert("Extra JVM arguments", "Дод. аргументи JVM");
    uk.insert("Warning: Xmx exceeds 80% of RAM. The system may stutter.",
              "Увага: Xmx більше 80% ОЗП. Система може пригальмовувати.");
    uk.insert("e.g. -XX:+UseG1GC", "наприклад -XX:+UseG1GC");
    uk.insert("Use recommended args", "Рекомендовані аргументи");
    uk.insert("Auto memory (recommended)", "Автопам'ять (рекомендується)");
    uk.insert("Yes", "Так");
    uk.insert("No", "Ні");
    uk.insert("Cancel", "Скасувати");
    uk.insert("Save", "Зберегти");
    uk.insert("Add Build", "Додати збірку");
    uk.insert("New Build", "Нова збірка");
    uk.insert("Build ID", "ID збірки");
    uk.insert("Repo (owner/repo)", "Репозиторій (owner/repo)");
    uk.insert("Asset name", "Назва файлу");
    uk.insert("Asset name (optional)", "Назва файлу (необов'язково)");
    uk.insert("Update from latest", "Оновити з latest");
    uk.insert("Update successful.", "Оновлення виконано.");
    uk.insert("Update failed: %1", "Помилка оновлення: %1");
    uk.insert("Tip: keep asset name stable for auto-updates.", "Порада: назва файлу має бути стабільною.");
    uk.insert("Fill in all fields.", "Заповніть усі поля.");
    uk.insert("User email", "Пошта користувача");
    uk.insert("Reset device", "Скинути пристрій");
    uk.insert("Reset device binding", "Скидання прив'язки пристрою");
    uk.insert("Device binding reset.", "Прив'язку пристрою скинуто.");
    uk.insert("Device not bound.", "Пристрій не прив'язаний.");
    uk.insert("Unknown", "Невідомо");
    uk.insert("User not found.", "Користувача не знайдено.");
    uk.insert("Invalid email.", "Некоректна пошта.");
    uk.insert("Use asset name as build name", "Використовувати назву файлу як назву збірки");
    uk.insert("Name", "Назва");
    uk.insert("Description", "Опис");
    uk.insert("Minecraft Version", "Версія Minecraft");
    uk.insert("Java Version", "Версія Java");
    uk.insert("Download URL", "Посилання на завантаження");
    uk.insert("Checksum (sha256)", "Контрольна сума (sha256)");
    uk.insert("Checksum must be a SHA-256 hex string (64 chars).",
              "Контрольна сума має бути SHA-256 (64 символи).");
    uk.insert("Size", "Розмір");
    uk.insert("Image URL", "Посилання на зображення");
    uk.insert("Tags (comma)", "Теги (через кому)");
    uk.insert("Progress: %1 / %2 MB", "Прогрес: %1 / %2 МБ");
    uk.insert("Downloaded %1 MB", "Завантажено %1 МБ");
    uk.insert("Speed: %1 MB/s", "Швидкість: %1 МБ/с");
    uk.insert("ETA: %1", "Залишилось: %1");
    uk.insert("ETA: --", "Залишилось: --");
    uk.insert("Files: %1 / %2", "Файли: %1 / %2");
    uk.insert("Verify files", "Перевірити файли");
    uk.insert("Change Java path", "Шлях до Java");
    uk.insert("Select Java", "Вибір Java");
    uk.insert("Invalid Java path.", "Невірний шлях до Java.");
    uk.insert("Custom Java path not found. Please update it.", "Шлях до Java не знайдено. Вкажіть інший.");
    uk.insert("Java path", "Шлях до Java");
    uk.insert("Missing", "немає");
    uk.insert("Not found", "не знайдено");
    uk.insert("Verifying files...", "Перевірка файлів...");
    uk.insert("Verification finished", "Перевірку завершено");
    uk.insert("All files are OK.", "Усі файли в порядку.");
    uk.insert("Missing files: %1", "Відсутні файли: %1");
    uk.insert("Corrupted files: %1", "Пошкоджені файли: %1");
    uk.insert("Missing build files: %1", "Відсутні файли збірки: %1");
    uk.insert("Corrupted build files: %1", "Пошкоджені файли збірки: %1");
    uk.insert("Repairing files...", "Відновлення файлів...");
    uk.insert("Downloading missing file", "Завантаження відсутнього файлу");
    uk.insert("Downloading build archive", "Завантаження архіву збірки");
    uk.insert("Extracting build archive", "Розпакування архіву збірки");
    uk.insert("Restoring build files", "Відновлення файлів збірки");
    uk.insert("Some files cannot be repaired automatically.", "Деякі файли неможливо відновити автоматично.");
    uk.insert("Missing files were downloaded.", "Відсутні файли було завантажено.");
    uk.insert("Build files were restored.", "Файли збірки відновлено.");
    uk.insert("Build manifest not found. Reinstall the build to enable verification.",
              "Маніфест збірки не знайдено. Перевстановіть збірку для перевірки.");
    uk.insert("Import build", "Імпорт збірки");
    uk.insert("Build imported.", "Збірку імпортовано.");
    uk.insert("Build import failed.", "Не вдалося імпортувати збірку.");
    uk.insert("Download speed limit (KB/s, 0 = unlimited)", "Ліміт швидкості (КБ/с, 0 = без обмежень)");
    uk.insert("Select language", "Виберіть мову");
    uk.insert("Choose your language", "Оберіть мову");
    uk.insert("email@gmail.com", "email@gmail.com");

    addDict("ru", ru);
    addDict("uk", uk);
}

} // namespace

namespace I18n {

void setLanguage(const QString &lang)
{
    ensureInit();
    if (dicts.contains(lang)) {
        currentLang = lang;
    } else {
        currentLang = "en";
    }
}

QString language()
{
    return currentLang;
}

QString tr(const QString &key)
{
    ensureInit();
    if (currentLang == "en") {
        return key;
    }
    const auto map = dicts.value(currentLang);
    return map.contains(key) ? map.value(key) : key;
}

} // namespace I18n
