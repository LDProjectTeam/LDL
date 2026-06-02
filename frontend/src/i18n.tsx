import React, {
  createContext,
  useContext,
  useState,
  useCallback,
  useEffect,
  ReactNode,
} from "react";

export type Language = "ru" | "en" | "ua";

interface Translations {
  online: string;
  appLanguage: string;
  profileManagement: string;
  settings: string;
  legalInfo: string;
  support: string;
  whatsNew: string;
  logout: string;
  quitApp: string;
  install: string;
  play: string;
  launching: string;
  update: string;
  selectLanguageTitle: string;
  selectLanguageSubtitle: string;
  continue: string;
  featuredGame: string;
  badgeFeatured: string;
  badgeBeta: string;
  badgeInDev: string;
  descLostDeath1: string;
  descLostDeath2: string;
  descLostDeath3: string;
  descWitcher: string;
  descCyberpunk: string;
  // Settings Modal
  settingsGeneral: string;
  settingsAutoStart: string;
  settingsAutoStartDesc: string;
  settingsHwAccel: string;
  settingsHwAccelDesc: string;
  // Downloads
  settingsDownloads: string;
  settingsInstallDir: string;
  settingsInstallDirDesc: string;
  settingsChangeDir: string;
  // Optimizer
  settingsOptimizer: string;
  settingsAutoOptimize: string;
  settingsAutoOptimizeDesc: string;
  settingsManualRam: string;
  settingsManualRamDesc: string;
  settingsManualThreads: string;
  settingsManualThreadsDesc: string;
  settingsResetGameSettings: string;
  settingsResetGameSettingsDesc: string;
  settingsResetSuccess: string;
  // Profile Modal
  profileUsername: string;
  profileSaving: string;
  profileSaveBtn: string;
  profileLinkMicrosoft: string;
  profileLinkMicrosoftDesc: string;
  profileMicrosoftLinked: string;
  profileUseMicrosoftDataTitle: string;
  profileUseMicrosoftDataDesc: string;
  profileUnlinkMicrosoft: string;
  // Common
  versionStr: string;
  settingsRestartNotice: string;
  settingsRestartBadge: string;
  // Game States
  repair: string;
  comingSoon: string;
  installing: string;
  gameRunning: string;
  repairDesc: string;
  deleteGame: string;
  killGame: string;
  openDirectory: string;
  deleteConfirm: string;
  launchHint: string;
  profileUseMicrosoftDataApply: string;
  profileUseMicrosoftDataSkip: string;
  // Legal Modal
  legalTermsTitle: string;
  legalTermsP1: string;
  legalTermsP2: string;
  legalPrivacyTitle: string;
  legalPrivacyP1: string;
  legalPrivacyP2: string;
  legalLicenseTitle: string;
  legalLicenseP1: string;
  // Changelog Modal
  changelogVersion: string;
  // Auth Screen
  authLoginTab: string;
  authRegisterTab: string;
  authEmailPlaceholder: string;
  authUsernamePlaceholder: string;
  authSendCode: string;
  authCodePlaceholder: string;
  authEnter: string;
  authRegisterBtn: string;
  authCodeSent: string;
  authCodeError: string;
  authEmailInvalid: string;
  authWelcomeBack: string;
  authVerifyTitle: string;
  authResendCode: string;
  authGoogleBtn: string;
  authMicrosoftButton: string;
  authLoading: string;
  authSendCodeButton: string;
  authGetCodeButton: string;
  authBack: string;
  authEmailSentTo: string;
  authEmailSentHint: string;
  authOrViaEmail: string;
  // Support Modal
  supportWelcome: string;
  supportAutoReply: string;
  supportError: string;
  supportRetry: string;
  supportSendHint: string;
  supportNewLine: string;
  supportMessagePlaceholder: string;
  supportSentPlaceholder: string;
  supportNamePlaceholder: string;
  supportEmailPlaceholder: string;
  // Payment
  buyNow: string;
  waitingPayment: string;
  buyDesc: string;
  // TON Payment Modal
  tonPayTitle: string;
  tonPayAmount: string;
  tonPayScanHint: string;
  tonPayWallets: string;
  tonPayStarsBtn: string;
  tonPayWaiting: string;
  tonPayPaid: string;
  tonPayPaidDesc: string;
  tonPayDone: string;
  // Hardware Tier
  tierDetected: string;
  tierLow: string;
  tierMedium: string;
  tierHigh: string;
  tierUltra: string;
  vsyncOff: string;
  fpsCapped: string;
  tierMonitor: string;
  autoStr: string;
  // Profile errors
  profileErrTooBig: string;
  profileErrEmpty: string;
  profileErrSave: string;
  profileErrLink: string;
  profileErrUnlink: string;
  profileAvatarHint: string;
  // GameOverlay
  networkSpeed: string;
  diskSpeed: string;
  loading: string;
  // Auth errors
  authErrNoAccount: string;
  authErrExists: string;
}

export const translations: Record<Language, Translations> = {
  ru: {
    online: "В сети",
    appLanguage: "Язык приложения",
    profileManagement: "Управление профилем",
    settings: "Настройки",
    legalInfo: "Правовая информация",
    support: "Поддержка",
    whatsNew: "Что нового",
    logout: "Выйти из аккаунта",
    quitApp: "Выйти из приложения",
    install: "Установить",
    play: "Играть",
    launching: "Запуск...",
    update: "Обновить",
    selectLanguageTitle: "Выберите язык",
    selectLanguageSubtitle: "Вы сможете изменить это позже в настройках",
    continue: "Продолжить",
    featuredGame: "Рекомендуемая игра",
    badgeFeatured: "РЕКОМЕНДУЕМАЯ ИГРА",
    badgeBeta: "БЕТА ВЕРСИЯ",
    badgeInDev: "В РАЗРАБОТКЕ",
    descLostDeath1:
      "Игра, с которой все началось. Сможете ли вы пережить убийство богов?",
    descLostDeath2:
      "Продолжение признанного шедевра. Разгадайте тайны проклятого леса.",
    descLostDeath3: "",
    descWitcher:
      "Охотьтесь на монстров и зарабатывайте чеканную монету в этом темном фэнтезийном мире.",
    descCyberpunk: "Добро пожаловать в Найт-Сити. Станьте легендой.",
    settingsGeneral: "Общие",
    settingsAutoStart: "Запускать вместе с Windows",
    settingsAutoStartDesc:
      "Лаунчер будет автоматически открываться при включении компьютера, чтобы игры всегда были готовы к запуску.",
    settingsHwAccel: "Аппаратное ускорение",
    settingsHwAccelDesc:
      "Использовать видеокарту для отрисовки интерфейса. Отключите, если лаунчер тормозит или отображается некорректно.",
    settingsDownloads: "Загрузки",
    settingsInstallDir: "Папка установки игр",
    settingsInstallDirDesc:
      "Место, куда будут скачиваться все игры и сборки. Выберите диск, на котором больше свободного места.",
    settingsChangeDir: "Изменить...",
    settingsOptimizer: "Оптимизация Игры",
    settingsAutoOptimize: "Умная оптимизация системы",
    settingsAutoOptimizeDesc:
      "ВКЛ: лаунчер определяет мощность ПК и сам настраивает graphics (options.txt), потоки Sodium и параметры Java. ВЫКЛ: лаунчер не трогает файлы игры — вы настраиваете графику сами, а здесь задаёте только ОЗУ и ядра процессора.",
    settingsManualRam: "Выделение ОЗУ (ГБ)",
    settingsManualRamDesc:
      "Сколько оперативной памяти выделить Java-процессу игры. Рекомендуется оставлять 2–4 ГБ свободными для Windows.",
    settingsManualThreads: "Ядра процессора (0 = Авто)",
    settingsManualThreadsDesc:
      "Количество ядер/потоков для игры и Sodium. 0 = не ограничивать. Не ставьте максимум — оставьте 2 ядра для Windows.",
    settingsResetGameSettings: "Сбросить оптимизацию игр",
    settingsResetGameSettingsDesc:
      "При включённой оптимизации: сбрасывает применённые настройки, чтобы лаунчер переопределил тир и применил профиль заново при следующем запуске.",
    settingsResetSuccess: "Настройки игр сброшены!",
    profileUsername: "Имя пользователя",
    profileSaving: "Сохранение...",
    profileSaveBtn: "Сохранить изменения",
    profileLinkMicrosoft: "Привязать аккаунт Microsoft",
    profileLinkMicrosoftDesc: "Использовать лицензию Minecraft и скин персонажа",
    profileMicrosoftLinked: "Аккаунт Microsoft привязан",
    profileUseMicrosoftDataTitle: "Использовать данные Microsoft?",
    profileUseMicrosoftDataDesc: "Хотите изменить текущее имя и аватарку на те, которые указаны в вашем аккаунте Microsoft?",
    profileUnlinkMicrosoft: "Отвязать",
    versionStr: "Версия",
    settingsRestartNotice:
      "Для применения некоторых настроек требуется перезапуск",
    settingsRestartBadge: "Перезапуск",
    repair: "Восстановить",
    comingSoon: "Скоро",
    installing: "Установка...",
    gameRunning: "В игре",
    repairDesc:
      "Файлы повреждены или недоступны. Восстановить повреждённые файлы.",
    deleteGame: "Удалить сборку",
    killGame: "Завершить игру (Краш)",
    openDirectory: "Открыть папку",
    deleteConfirm: "Удалить? Все файлы будут удалены.",
    launchHint: "Первый запуск может занять несколько минут — пожалуйста, подождите.",
    profileUseMicrosoftDataApply: "Применить",
    profileUseMicrosoftDataSkip: "Оставить текущее",
    legalTermsTitle: "Условия использования (Terms of Service)",
    legalTermsP1:
      "LDLauncher — это проприетарное программное обеспечение. Любое копирование, распространение, модификация исходного кода или декомпиляция без письменного согласия правообладателя строго запрещены.",
    legalTermsP2:
      "Разработчики не несут ответственности за возможный ущерб, потерю данных или сбои в работе оборудования при использовании лаунчера.",
    legalPrivacyTitle: "Политика конфиденциальности (Privacy Policy)",
    legalPrivacyP1:
      "Мы уважаем вашу конфиденциальность. Лаунчер собирает минимальный объем технических данных для авторизации и загрузки игр. Ваши данные не передаются третьим лицам.",
    legalPrivacyP2:
      "Ваши пароли и платежные токены передаются исключительно в зашифрованном виде и не сохраняются на наших серверах в открытом доступе.",
    legalLicenseTitle: "Лицензирование и Торговые марки",
    legalLicenseP1:
      "Все права защищены. Исходный код и ресурсы лаунчера являются интеллектуальной собственностью LDProject. Покупка лаунчера предоставляет право на личное использование продукта, но не дает прав на распространение кода или игровых сборок Minecraft. Все сторонние товарные знаки и названия игр принадлежат их владельцам.",
    changelogVersion: "Версия",
    authLoginTab: "Вход",
    authRegisterTab: "Регистрация",
    authEmailPlaceholder: "Ваш Email",
    authUsernamePlaceholder: "Придумайте никнейм",
    authSendCode: "Получить код",
    authCodePlaceholder: "6-значный код",
    authEnter: "Войти",
    authRegisterBtn: "Создать аккаунт",
    authCodeSent: "Код отправлен на почту!",
    authCodeError: "Неверный код",
    authEmailInvalid: "Некорректный Email",
    authWelcomeBack: "С возвращением!",
    authVerifyTitle: "Подтверждение",
    authResendCode: "Отправить снова",
    authGoogleBtn: "Войти через Google",
    authMicrosoftButton: "Войти через Microsoft",
    authLoading: "Загрузка...",
    authSendCodeButton: "Получить код",
    authGetCodeButton: "Отправить снова",
    authBack: "Назад",
    authEmailSentTo: "Письмо отправлено на",
    authEmailSentHint:
      "Нажмите ссылку в письме для автоматического входа,\nили введите код вручную ниже",
    authOrViaEmail: "Или через Email",
    supportWelcome: "Привет! Чем я могу помочь? Опишите проблему, и мы свяжемся с вами в ближайшее время.",
    supportAutoReply: "✅ Сообщение отправлено! Мы получили вашу заявку и ответим на указанную почту в течение 24 часов.",
    supportError: "❌ Не удалось отправить сообщение. Проверьте интернет-соединение или напишите напрямую на ldprojectteams@gmail.com",
    supportRetry: "Попробовать снова",
    supportSendHint: "отправить",
    supportNewLine: "новая строка",
    supportMessagePlaceholder: "Опишите вашу проблему...",
    supportSentPlaceholder: "Сообщение отправлено!",
    supportNamePlaceholder: "Ваше имя",
    supportEmailPlaceholder: "Ваш e-mail",
    buyNow: "Купить",
    waitingPayment: "Ожидание оплаты...",
    buyDesc: "Откроется Telegram — вернитесь после оплаты",
    tonPayTitle: "Оплата",
    tonPayAmount: "Сумма",
    tonPayScanHint: "Отсканируй QR-код камерой телефона",
    tonPayWallets: "Tonkeeper, Telegram Wallet, любой TON-кошелёк",
    tonPayStarsBtn: "Оплатить Telegram Stars ⭐",
    tonPayWaiting: "Ожидаем оплату...",
    tonPayPaid: "Оплата получена!",
    tonPayPaidDesc: "разблокирован. Нажми «Установить» в лаунчере.",
    tonPayDone: "Готово",
    tierDetected: "Определён тир железа",
    tierLow: "Слабый ПК — упор на производительность",
    tierMedium: "Средний ПК — баланс качества и FPS",
    tierHigh: "Мощный ПК — высокое качество без потери FPS",
    tierUltra: "Топовый ПК — максимальное качество",
    vsyncOff: "выкл",
    fpsCapped: "ограничен по частоте монитора",
    tierMonitor: "Монитор",
    autoStr: "Auto",
    profileErrTooBig: "Файл слишком большой (макс. 2 МБ)",
    profileErrEmpty: "Имя не может быть пустым",
    profileErrSave: "Ошибка сохранения",
    profileErrLink: "Ошибка привязки",
    profileErrUnlink: "Ошибка отвязки",
    profileAvatarHint: "макс. 2 МБ · jpg / png / webp",
    networkSpeed: "Скорость сети:",
    diskSpeed: "Скорость диска:",
    loading: "Загрузка...",
    authErrNoAccount: "Аккаунт не найден. Перейдите на вкладку «Регистрация».",
    authErrExists: "Аккаунт уже существует. Перейдите на вкладку «Вход».",
  },
  en: {
    online: "Online",
    appLanguage: "App Language",
    profileManagement: "Profile Management",
    settings: "Settings",
    legalInfo: "Legal Info",
    support: "Support",
    whatsNew: "What's New",
    logout: "Log Out",
    quitApp: "Quit App",
    install: "Install",
    play: "Play",
    launching: "Launching...",
    update: "Update",
    selectLanguageTitle: "Select Language",
    selectLanguageSubtitle: "You can change this later in settings",
    continue: "Continue",
    featuredGame: "Featured Game",
    badgeFeatured: "FEATURED GAME",
    badgeBeta: "BETA VERSION",
    badgeInDev: "IN DEVELOPMENT",
    descLostDeath1:
      "The game that started it all. Can you survive the killing of gods?",
    descLostDeath2:
      "Sequel to the acclaimed masterpiece. Unravel the mysteries of the cursed forest.",
    descLostDeath3: "",
    descWitcher: "Hunt monsters and earn your coin in this dark fantasy world.",
    descCyberpunk: "Welcome to Night City. Become a legend.",
    settingsGeneral: "General",
    settingsAutoStart: "Start with Windows",
    settingsAutoStartDesc:
      "The launcher will automatically open when you turn on your computer so games are always ready to play.",
    settingsHwAccel: "Hardware Acceleration",
    settingsHwAccelDesc:
      "Use the GPU to render the interface. Disable this if the launcher lags or displays incorrectly.",
    settingsDownloads: "Downloads",
    settingsInstallDir: "Install Directory",
    settingsInstallDirDesc:
      "The location where all games and modpacks will be downloaded. Choose a drive with plenty of free space.",
    settingsChangeDir: "Change...",
    settingsOptimizer: "Game Optimization",
    settingsAutoOptimize: "Smart System Optimization",
    settingsAutoOptimizeDesc:
      "ON: the launcher detects your hardware tier and auto-configures graphics (options.txt), Sodium threads, and Java args. OFF: game files are not touched — you configure graphics yourself, and here you only set RAM and CPU cores.",
    settingsManualRam: "RAM Allocation (GB)",
    settingsManualRamDesc:
      "How much RAM to give the game's Java process. Leave 2–4 GB free for Windows.",
    settingsManualThreads: "CPU Cores (0 = Auto)",
    settingsManualThreadsDesc:
      "Number of CPU cores/threads for the game and Sodium. 0 = no limit. Don't set the maximum — leave 2 cores for Windows.",
    settingsResetGameSettings: "Reset game optimization",
    settingsResetGameSettingsDesc:
      "With optimization ON: clears applied settings so the launcher re-detects the tier and re-applies the profile on next launch.",
    settingsResetSuccess: "Game settings have been reset!",
    profileUsername: "Username",
    profileSaving: "Saving...",
    profileSaveBtn: "Save Changes",
    profileLinkMicrosoft: "Link Microsoft Account",
    profileLinkMicrosoftDesc: "Use Minecraft license and character skin",
    profileMicrosoftLinked: "Microsoft account linked",
    profileUseMicrosoftDataTitle: "Use Microsoft data?",
    profileUseMicrosoftDataDesc: "Do you want to change your current name and avatar to the ones from your Microsoft account?",
    profileUnlinkMicrosoft: "Unlink",
    versionStr: "Version",
    settingsRestartNotice: "A restart is required to apply some settings",
    settingsRestartBadge: "Restart",
    repair: "Repair",
    comingSoon: "Coming Soon",
    installing: "Installing...",
    gameRunning: "Running",
    repairDesc: "Some files are damaged or missing. Restore the missing files.",
    deleteGame: "Delete Modpack",
    killGame: "Kill Game (Force Stop)",
    openDirectory: "Open Folder",
    deleteConfirm: "Delete? All files will be removed.",
    launchHint: "First launch may take a few minutes — please wait.",
    profileUseMicrosoftDataApply: "Apply",
    profileUseMicrosoftDataSkip: "Keep Current",
    legalTermsTitle: "Terms of Service",
    legalTermsP1:
      "LDLauncher is proprietary software. Any copying, distribution, modification of the source code, or decompilation without prior written consent from the copyright holder is strictly prohibited.",
    legalTermsP2:
      "The developers are not responsible for any damage, data loss, or hardware failures resulting from the use of the launcher.",
    legalPrivacyTitle: "Privacy Policy",
    legalPrivacyP1:
      "We respect your privacy. The launcher collects a minimal amount of technical data necessary for authentication and game downloads. We do not share your data.",
    legalPrivacyP2:
      "Your passwords and payment tokens are transmitted securely in encrypted form and are never stored on our servers in plain text.",
    legalLicenseTitle: "Licensing & Trademarks",
    legalLicenseP1:
      "All rights reserved. The source code and launcher assets are the intellectual property of LDProject. Purchasing the launcher grants a personal license to use the product, but does not grant any rights to redistribute the code or Minecraft game builds. All third-party trademarks and game titles belong to their respective owners.",
    changelogVersion: "Version",
    authLoginTab: "Login",
    authRegisterTab: "Register",
    authEmailPlaceholder: "Your Email",
    authUsernamePlaceholder: "Choose a Username",
    authSendCode: "Get Code",
    authCodePlaceholder: "6-digit code",
    authEnter: "Sign In",
    authRegisterBtn: "Create Account",
    authCodeSent: "Code sent to email!",
    authCodeError: "Invalid code",
    authEmailInvalid: "Invalid Email",
    authWelcomeBack: "Welcome back!",
    authVerifyTitle: "Verification",
    authResendCode: "Resend Code",
    authGoogleBtn: "Sign in with Google",
    authMicrosoftButton: "Sign in with Microsoft",
    authLoading: "Loading...",
    authSendCodeButton: "Get Code",
    authGetCodeButton: "Resend Code",
    authBack: "Back",
    authEmailSentTo: "Email sent to",
    authEmailSentHint:
      "Click the link in the email for automatic sign-in,\nor enter the code manually below",
    authOrViaEmail: "Or via Email",
    supportWelcome: "Hey! How can I help you? Describe your issue and we'll get back to you shortly.",
    supportAutoReply: "✅ Message sent! We received your request and will reply to your email within 24 hours.",
    supportError: "❌ Failed to send message. Check your internet connection or email us directly at ldprojectteams@gmail.com",
    supportRetry: "Try again",
    supportSendHint: "send",
    supportNewLine: "new line",
    supportMessagePlaceholder: "Describe your issue...",
    supportSentPlaceholder: "Message sent!",
    supportNamePlaceholder: "Your name",
    supportEmailPlaceholder: "Your e-mail",
    buyNow: "Buy",
    waitingPayment: "Waiting for payment...",
    buyDesc: "Telegram will open — return here after payment",
    tonPayTitle: "Checkout",
    tonPayAmount: "Amount",
    tonPayScanHint: "Scan the QR code with your phone camera",
    tonPayWallets: "Tonkeeper, Telegram Wallet, any TON wallet",
    tonPayStarsBtn: "Pay with Telegram Stars ⭐",
    tonPayWaiting: "Waiting for payment...",
    tonPayPaid: "Payment received!",
    tonPayPaidDesc: "is unlocked. Press \"Install\" in the launcher.",
    tonPayDone: "Done",
    tierDetected: "Hardware tier detected",
    tierLow: "Low-end PC — focused on performance",
    tierMedium: "Mid-range PC — balanced quality and FPS",
    tierHigh: "High-end PC — great quality without FPS loss",
    tierUltra: "Top-end PC — maximum quality",
    vsyncOff: "off",
    fpsCapped: "capped to monitor Hz",
    tierMonitor: "Monitor",
    autoStr: "Auto",
    profileErrTooBig: "File too large (max 2 MB)",
    profileErrEmpty: "Name cannot be empty",
    profileErrSave: "Save error",
    profileErrLink: "Link error",
    profileErrUnlink: "Unlink error",
    profileAvatarHint: "max 2 MB · jpg / png / webp",
    networkSpeed: "Network speed:",
    diskSpeed: "Disk speed:",
    loading: "Loading...",
    authErrNoAccount: "Account not found. Please go to the \"Register\" tab.",
    authErrExists: "Account already exists. Please go to the \"Login\" tab.",
  },
  ua: {
    online: "В мережі",
    appLanguage: "Мова додатку",
    profileManagement: "Керування профілем",
    settings: "Налаштування",
    legalInfo: "Правова інформація",
    support: "Підтримка",
    whatsNew: "Що нового",
    logout: "Вийти з акаунту",
    quitApp: "Вийти з додатку",
    install: "Встановити",
    play: "Грати",
    launching: "Запуск...",
    update: "Оновити",
    selectLanguageTitle: "Оберіть мову",
    selectLanguageSubtitle: "Ви зможете змінити це пізніше в налаштуваннях",
    continue: "Продовжити",
    featuredGame: "Рекомендована гра",
    badgeFeatured: "РЕКОМЕНДОВАНА ГРА",
    badgeBeta: "БЕТА ВЕРСІЯ",
    badgeInDev: "В РОЗРОБЦІ",
    descLostDeath1:
      "Гра, з якої все почалося. Чи зможете ви пережити вбивство богів?",
    descLostDeath2:
      "Продовження визнаного шедевра. Розгадайте таємниці проклятого лісу.",
    descLostDeath3: "",
    descWitcher:
      "Полюйте на монстрів та заробляйте карбовану монету в цьому темному фентезійному світі.",
    descCyberpunk: "Ласкаво просимо до Найт-Сіті. Станьте легендою.",
    settingsGeneral: "Загальні",
    settingsAutoStart: "Запустити разом з Windows",
    settingsAutoStartDesc:
      "Лаунчер автоматично відкриватиметься під час увімкнення комп'ютера, щоб ігри завжди були готові до запуску.",
    settingsHwAccel: "Апаратне прискорення",
    settingsHwAccelDesc:
      "Використовувати відеокарту для відтворення інтерфейсу. Вимкніть, якщо лаунчер гальмує або відображається некоректно.",
    settingsDownloads: "Завантаження",
    settingsInstallDir: "Папка встановлення ігор",
    settingsInstallDirDesc:
      "Місце, куди будуть завантажуватися всі ігри та збірки. Оберіть диск, на якому найбільше вільного місця.",
    settingsChangeDir: "Змінити...",
    settingsOptimizer: "Оптимізація Гри",
    settingsAutoOptimize: "Розумна оптимізація системи",
    settingsAutoOptimizeDesc:
      "УВМ: лаунчер визначає потужність ПК і сам налаштовує graphics (options.txt), потоки Sodium та параметри Java. ВИМК: файли гри не чіпає — графіку налаштовуєте самі, а тут задаєте лише ОЗП та ядра.",
    settingsManualRam: "Виділення ОЗП (ГБ)",
    settingsManualRamDesc:
      "Скільки оперативної пам'яті виділити Java-процесу гри. Рекомендується залишати 2–4 ГБ вільними для Windows.",
    settingsManualThreads: "Ядра процесора (0 = Авто)",
    settingsManualThreadsDesc:
      "Кількість ядер/потоків для гри та Sodium. 0 = не обмежувати. Не ставте максимум — залиште 2 ядра для Windows.",
    settingsResetGameSettings: "Скинути оптимізацію ігор",
    settingsResetGameSettingsDesc:
      "При увімкненій оптимізації: скидає застосовані налаштування, щоб лаунчер перевизначив тир і застосував профіль заново при наступному запуску.",
    settingsResetSuccess: "Налаштування ігор скинуто!",
    profileUsername: "Ім'я користувача",
    profileSaving: "Збереження...",
    profileSaveBtn: "Зберегти зміни",
    profileLinkMicrosoft: "Прив'язати акаунт Microsoft",
    profileLinkMicrosoftDesc: "Використовувати ліцензію Minecraft та скін персонажа",
    profileMicrosoftLinked: "Акаунт Microsoft прив'язаний",
    profileUseMicrosoftDataTitle: "Використовувати дані Microsoft?",
    profileUseMicrosoftDataDesc: "Бажаєте змінити поточне ім'я та аватарку на ті, що вказані у вашому акаунті Microsoft?",
    profileUnlinkMicrosoft: "Від'язати",
    versionStr: "Версія",
    settingsRestartNotice:
      "Для застосування деяких налаштувань потрібне перезапуск",
    settingsRestartBadge: "Перезапуск",
    repair: "Відновити",
    comingSoon: "Скоро",
    installing: "Встановлення...",
    gameRunning: "У грі",
    repairDesc: "Файли пошкоджені або відсутні. Відновити пошкоджені файли.",
    deleteGame: "Видалити збірку",
    killGame: "Завершити гру (Краш)",
    openDirectory: "Відкрити теку",
    deleteConfirm: "Видалити? Усі файли будуть видалені.",
    launchHint: "Перший запуск може зайняти кілька хвилин — будь ласка, зачекайте.",
    profileUseMicrosoftDataApply: "Застосувати",
    profileUseMicrosoftDataSkip: "Залишити поточне",
    legalTermsTitle: "Умови використання (Terms of Service)",
    legalTermsP1:
      "LDLauncher — це пропрієтарне програмне забезпечення. Будь-яке копіювання, розповсюдження, модифікація вихідного коду або декомпіляція без письмової згоди правовласника суворо заборонені.",
    legalTermsP2:
      "Розробники не несуть відповідальності за можливу шкоду, втрату даних або збої в роботі вашого обладнання при використанні лаунчера.",
    legalPrivacyTitle: "Політика конфіденційності (Privacy Policy)",
    legalPrivacyP1:
      "Ми поважаємо вашу конфіденційність. Лаунчер збирає мінімальний обсяг технічних даних, необхідний для роботи авторизації та завантаження ігор. Ваші дані не передаються третім особам.",
    legalPrivacyP2:
      "Ваші паролі та платіжні дані передаються виключно в зашифрованому вигляді та не зберігаються на наших серверах у відкритому доступі.",
    legalLicenseTitle: "Ліцензування та Торгові марки",
    legalLicenseP1:
      "Усі права захищені. Вихідний код та асети лаунчера є інтелектуальною власністю LDProject. Купівля лаунчера надає право на особисте використання продукту, але не надає прав на розповсюдження коду або ігрових збірок Minecraft. Усі сторонні торгові марки та назви ігор належать їхнім законним власникам.",
    changelogVersion: "Версія",
    authLoginTab: "Вход",
    authRegisterTab: "Реєстрація",
    authEmailPlaceholder: "Ваш Email",
    authUsernamePlaceholder: "Придумайте никнейм",
    authSendCode: "Отримати код",
    authCodePlaceholder: "6-значний код",
    authEnter: "Увійти",
    authRegisterBtn: "Створити акаунт",
    authCodeSent: "Код відправлено на пошту!",
    authCodeError: "Невірний код",
    authEmailInvalid: "Некоректний Email",
    authWelcomeBack: "З поверненням!",
    authVerifyTitle: "Підтвердження",
    authResendCode: "Отправить снова",
    authGoogleBtn: "Увійти через Google",
    authMicrosoftButton: "Увійти через Microsoft",
    authLoading: "Завантаження...",
    authSendCodeButton: "Отримати код",
    authGetCodeButton: "Надіслати знову",
    authBack: "Назад",
    authEmailSentTo: "Лист надіслано на",
    authEmailSentHint:
      "Натисніть посилання в листі для автоматичного входу,\nабо введіть код вручну ниже",
    authOrViaEmail: "Або через Email",
    supportWelcome: "Привіт! Чим я можу допомогти? Опишіть проблему, і ми зв'яжемося з вами найближчим часом.",
    supportAutoReply: "✅ Повідомлення надіслано! Ми отримали вашу заявку і відповімо на вказану пошту протягом 24 годин.",
    supportError: "❌ Не вдалося надіслати повідомлення. Перевірте з'єднання з інтернетом або напишіть нам на ldprojectteams@gmail.com",
    supportRetry: "Спробувати знову",
    supportSendHint: "надіслати",
    supportNewLine: "новий рядок",
    supportMessagePlaceholder: "Опишіть вашу проблему...",
    supportSentPlaceholder: "Повідомлення надіслано!",
    supportNamePlaceholder: "Ваше ім'я",
    supportEmailPlaceholder: "Ваш e-mail",
    buyNow: "Купити",
    waitingPayment: "Очікування оплати...",
    buyDesc: "Telegram відкриється — поверніться після оплати",
    tonPayTitle: "Оплата",
    tonPayAmount: "Сума",
    tonPayScanHint: "Відскануйте QR-код камерою телефону",
    tonPayWallets: "Tonkeeper, Telegram Wallet, будь-який TON-гаманець",
    tonPayStarsBtn: "Сплатити Telegram Stars ⭐",
    tonPayWaiting: "Очікуємо оплату...",
    tonPayPaid: "Оплату отримано!",
    tonPayPaidDesc: "розблоковано. Натисни «Встановити» в лаунчері.",
    tonPayDone: "Готово",
    tierDetected: "Визначено тир заліза",
    tierLow: "Слабкий ПК — упор на продуктивність",
    tierMedium: "Середній ПК — баланс якості та FPS",
    tierHigh: "Потужний ПК — висока якість без втрати FPS",
    tierUltra: "Топовий ПК — максимальна якість",
    vsyncOff: "вимкнено",
    fpsCapped: "обмежено частотою монітора",
    tierMonitor: "Монітор",
    autoStr: "Auto",
    profileErrTooBig: "Файл занадто великий (макс. 2 МБ)",
    profileErrEmpty: "Ім’я не може бути порожнім",
    profileErrSave: "Помилка збереження",
    profileErrLink: "Помилка прив’язки",
    profileErrUnlink: "Помилка від’язки",
    profileAvatarHint: "макс. 2 МБ · jpg / png / webp",
    networkSpeed: "Швидкість мережі:",
    diskSpeed: "Швидкість диску:",
    loading: "Завантаження...",
    authErrNoAccount: "Акаунт не знайдено. Перейдіть на вкладку «Реєстрація».",
    authErrExists: "Акаунт вже існує. Перейдіть на вкладку «Вхід».",
  },
};

export const languages = [
  { code: "ru" as Language, label: "Русский", flag: "🇷🇺" },
  { code: "en" as Language, label: "English", flag: "🇬🇧" },
  { code: "ua" as Language, label: "Українська", flag: "🇺🇦" },
];

interface I18nContextType {
  t: Translations;
  lang: Language;
  setLanguage: (code: Language) => void;
  languages: typeof languages;
  isInitialized: boolean;
}

const I18nContext = createContext<I18nContextType | undefined>(undefined);

export const I18nProvider = ({ children }: { children: ReactNode }) => {
  // We start with null to know if we've checked localStorage yet
  const [lang, setLangState] = useState<Language>("ru");
  const [isInitialized, setIsInitialized] = useState(false);

  useEffect(() => {
    const saved = localStorage.getItem("ldl_language") as Language;
    if (saved && ["ru", "en", "ua"].includes(saved)) {
      setLangState(saved);
      setIsInitialized(true);
    } else {
      // Auto-detect language
      const systemLang = navigator.language.toLowerCase();
      let autoLang: Language = "en"; // default fallback
      if (systemLang.startsWith("ru")) autoLang = "ru";
      else if (systemLang.startsWith("uk") || systemLang.startsWith("ua"))
        autoLang = "ua";

      setLangState(autoLang);
      localStorage.setItem("ldl_language", autoLang);
      setIsInitialized(true);
    }
  }, []);

  const setLanguage = useCallback((code: Language) => {
    setLangState(code);
    localStorage.setItem("ldl_language", code);
    setIsInitialized(true); // Marks that user has made a choice
  }, []);

  return (
    <I18nContext.Provider
      value={{
        t: translations[lang],
        lang,
        setLanguage,
        languages,
        isInitialized,
      }}
    >
      {children}
    </I18nContext.Provider>
  );
};

export function useI18n() {
  const context = useContext(I18nContext);
  if (context === undefined) {
    throw new Error("useI18n must be used within an I18nProvider");
  }
  return context;
}
