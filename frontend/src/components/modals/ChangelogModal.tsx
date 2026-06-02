import BaseModal from './BaseModal';
import { useI18n } from '../../i18n';

interface ChangelogModalProps {
    isOpen: boolean;
    onClose: () => void;
}

export default function ChangelogModal({ isOpen, onClose }: ChangelogModalProps) {
    const { t, lang } = useI18n();

    const updatesByLang = {
        ru: [
            {
                version: '3.0.0',
                date: '15 мая 2026',
                type: 'stable',
                changes: [
                    'Интегрирована система оплаты: теперь можно покупать игры (Lost Death 2) прямо в лаунчере через Telegram Stars.',
                    'Завершена полная локализация всех элементов интерфейса (Настройки, Профиль, Оверлей, Авторизация).',
                    'Исправлен критический баг, из-за которого не применялся скин и ник при входе через Microsoft аккаунт.',
                    'Устранены мелкие дефекты отображения и опечатки в настройках оптимизатора.'
                ]
            },
            {
                version: '2.0.4',
                date: '25 апреля 2026',
                type: 'stable',
                changes: [
                    'Исправлен баг с запуском лаунчера на Linux (AppImage) из-за Read-Only файловой системы.',
                    'Полностью переработан дизайн меню пользователя: новые отступы, анимации и красные кнопки выхода.',
                    'Добавлены функциональные модальные окна для всех разделов (Настройки, Профиль, Информация, Что нового).',
                    'Настройки теперь реально работают: автозапуск, консоль отладки, аппаратное ускорение.',
                    'Автоматическое определение статуса сети (В сети / Не в сети).',
                    'Полная локализация интерфейса (RU / EN / UA).',
                ]
            },
            {
                version: '2.0.3',
                date: '10 апреля 2026',
                type: 'beta',
                changes: [
                    'Добавлена поддержка мультиязычности (Русский, Английский, Украинский).',
                    'Переработан Sidebar: добавлены индикаторы установки и прогресс-бары.',
                    'Улучшена работа с кэшем.',
                ]
            },
            {
                version: '2.0.0',
                date: '1 марта 2026',
                type: 'stable',
                changes: [
                    'Глобальный редизайн лаунчера.',
                    'Переход на новый стек технологий (Electron + React).',
                    'Улучшена производительность скачивания игр.',
                ]
            }
        ],
        en: [
            {
                version: '3.0.0',
                date: 'May 15, 2026',
                type: 'stable',
                changes: [
                    'Integrated in-app payment system: you can now purchase games (Lost Death 2) directly in the launcher via Telegram Stars.',
                    'Completed full localization of all interface elements (Settings, Profile, Game Overlay, Auth).',
                    'Fixed a critical bug where the Microsoft account skin and nickname were not applied.',
                    'Fixed minor UI bugs and typos in the optimizer settings.'
                ]
            },
            {
                version: '2.0.4',
                date: 'April 25, 2026',
                type: 'stable',
                changes: [
                    'Fixed launcher crash on Linux (AppImage) caused by Read-Only filesystem.',
                    'Completely redesigned user menu: new padding, animations, and red exit buttons.',
                    'Added functional modal windows for all sections (Settings, Profile, Legal, What\'s New).',
                    'Settings now actually work: autostart, debug console, hardware acceleration.',
                    'Automatic network status detection (Online / Offline).',
                    'Full interface localization (RU / EN / UA).',
                ]
            },
            {
                version: '2.0.3',
                date: 'April 10, 2026',
                type: 'beta',
                changes: [
                    'Added multi-language support (Russian, English, Ukrainian).',
                    'Redesigned Sidebar with installation indicators and progress bars.',
                    'Improved cache handling.',
                ]
            },
            {
                version: '2.0.0',
                date: 'March 1, 2026',
                type: 'stable',
                changes: [
                    'Global launcher redesign.',
                    'Migrated to new tech stack (Electron + React).',
                    'Improved game download performance.',
                ]
            }
        ],
        ua: [
            {
                version: '3.0.0',
                date: '15 травня 2026',
                type: 'stable',
                changes: [
                    'Інтегровано систему оплати: тепер можна купувати ігри (Lost Death 2) безпосередньо в лаунчері через Telegram Stars.',
                    'Завершено повну локалізацію всіх елементів інтерфейсу (Налаштування, Профіль, Оверлей, Авторизація).',
                    'Виправлено критичний баг, через який не застосовувався скін та нік при вході через Microsoft акаунт.',
                    'Виправлено дрібні дефекти відображення та друкарські помилки в налаштуваннях оптимізатора.'
                ]
            },
            {
                version: '2.0.4',
                date: '25 квітня 2026',
                type: 'stable',
                changes: [
                    'Виправлено баг із запуском лаунчера на Linux (AppImage) через файлову систему Read-Only.',
                    'Повністю перероблено дизайн меню користувача: нові відступи, анімації та червоні кнопки виходу.',
                    'Додано функціональні модальні вікна для всіх розділів (Налаштування, Профіль, Інформація, Що нового).',
                    'Налаштування тепер реально працюють: автозапуск, консоль налагодження, апаратне прискорення.',
                    'Автоматичне визначення статусу мережі (В мережі / Не в мережі).',
                    'Повна локалізація інтерфейсу (RU / EN / UA).',
                ]
            },
            {
                version: '2.0.3',
                date: '10 квітня 2026',
                type: 'beta',
                changes: [
                    'Додано підтримку мультимовності (Російська, Англійська, Українська).',
                    'Перероблено Sidebar: додано індикатори встановлення та прогрес-бари.',
                    'Покращено роботу з кешем.',
                ]
            },
            {
                version: '2.0.0',
                date: '1 березня 2026',
                type: 'stable',
                changes: [
                    'Глобальний редизайн лаунчера.',
                    'Перехід на новий стек технологій (Electron + React).',
                    'Покращено продуктивність завантаження ігор.',
                ]
            }
        ]
    };

    const updates = updatesByLang[lang] || updatesByLang.en;

    return (
        <BaseModal isOpen={isOpen} onClose={onClose} title={t.whatsNew} width="600px">
            <div className="flex flex-col space-y-8">
                {updates.map((update, index) => (
                    <div key={index} className="relative pl-6">
                        {index !== updates.length - 1 && (
                            <div className="absolute left-[7px] top-6 bottom-[-24px] w-px bg-crt-accent/30"></div>
                        )}
                        <div className={`absolute left-0 top-1.5 w-3.5 h-3.5 rounded-full border-2 border-[#2B2B2B] ${update.type === 'stable' ? 'bg-crt-glow' : 'bg-[#E50000]'}`}></div>

                        <div className="flex items-center gap-3 mb-3">
                            <h3 className="text-crt-glow text-glow-subtle font-bold text-lg">{t.changelogVersion} {update.version}</h3>
                            <span className="text-crt-accent text-xs font-medium bg-crt-bg px-2 py-0.5 rounded-md border border-crt-accent">
                                {update.date}
                            </span>
                        </div>

                        <ul className="space-y-2">
                            {update.changes.map((change, idx) => (
                                <li key={idx} className="text-crt-text text-[13px] leading-relaxed flex items-start gap-2">
                                    <span className="text-crt-accent mt-1">•</span>
                                    <span>{change}</span>
                                </li>
                            ))}
                        </ul>
                    </div>
                ))}
            </div>
        </BaseModal>
    );
}
