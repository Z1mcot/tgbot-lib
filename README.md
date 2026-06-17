# tgbot-lib

C++ библиотека для работы с [Telegram Bot API](https://core.telegram.org/bots/api). Модели типов и методов API генерируются автоматически (`telegram-api-generator`).

**Текущая версия:** 1.1.0 (CMake) / 1.1.1 (`vcpkg.json`)  
**Стандарт:** C++23  
**Лицензия:** MIT

---

## Обзор

`tgbot-lib` предоставляет типизированный интерфейс ко всем методам Telegram Bot API: структуры данных, запросы, сериализация JSON и асинхронные вызовы через корутины.

### Основные компоненты

| Компонент | Путь | Описание |
|-----------|------|----------|
| `Bot` | `include/tgbot/Bot.hpp` | Точка входа: токен, `Api`, обработчики событий |
| `Api` | `include/tgbot/net/Api.hpp` | Все методы Telegram Bot API |
| `types/` | `include/tgbot/types/` | ~300 типов API (Message, User, Update и др.) |
| `requests/` | `include/tgbot/requests/` | ~170 структур запросов |
| `HttpClient` | `include/tgbot/net/HttpClient.hpp` | Абстракция HTTP-клиента |
| `LibCoroHttpClient` | `include/tgbot/net/LibCoroHttpClient.hpp` | Реализация HTTP-клиента по умолчанию |
| `TgLongPoll` | `include/tgbot/net/TgLongPoll.hpp` | Long polling для получения обновлений |
| `WebhookService` | `include/tgbot/net/WebhookService.hpp` | Обработка webhook-обновлений |
| `EventBroadcaster` | `include/tgbot/EventBroadcaster.hpp` | Регистрация обработчиков событий |
| `EventHandler` | `include/tgbot/EventHandler.hpp` | Диспетчеризация Update → обработчики |
| `tools/` | `include/tgbot/tools/` | Утилиты (чтение/запись файлов и др.) |

### Зависимости

| Библиотека | Назначение |
|------------|------------|
| [nlohmann-json](https://github.com/nlohmann/json) | Сериализация / десериализация JSON |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP/HTTPS клиент |
| [libcoro](https://github.com/jbaldwin/libcoro) | Корутины (`coro::task`) |
| OpenSSL | TLS для HTTPS |

Сборка через CMake и vcpkg (`vcpkg.json`).

### Пример использования

```cpp
#include <coro/sync_wait.hpp>
#include <tgbot/Bot.hpp>
#include <tgbot/net/TgLongPoll.hpp>

const TgBot::Bot bot{"YOUR_BOT_TOKEN"};

coro::task<> echo() {
    bot.getEvents().onAnyMessage([](TgBot::Message::Ptr msg) -> coro::task<> {
        co_await bot.getApi().sendMessage(msg->chat->id, "Echo: " + msg->text);
    });

    TgBot::TgLongPoll poll(bot);
    while (true) {
        poll.start();
    }
}

int main() {
    coro::sync_wait(echo());
}
```

Ручной тест: `tests/manual_test.cpp` (echo-бот через long poll, токен из `MVD_BOT_TOKEN`).

---

## Сетевой стек

### Раньше: Boost

В предыдущей версии библиотеки (и в родственных проектах, например tgbot-cpp) для сетевого взаимодействия использовался **Boost** (Boost.Asio / Boost.Beast). Это добавляло тяжёлую зависимость и усложняло сборку.

### Сейчас: cpp-httplib + libcoro

Сетевой слой переписан:

- **cpp-httplib** (`httplib::SSLClient`) — синхронные HTTP/HTTPS запросы
- **libcoro** — обёртка в `coro::task<std::string>`, все методы `Api` асинхронны
- **OpenSSL** — шифрование (макрос `CPPHTTPLIB_OPENSSL_SUPPORT`)

`HttpClient` — виртуальный интерфейс; можно подставить собственную реализацию. По умолчанию `Bot` использует `net::LibCoroHttpClient` (singleton в `Bot::_getDefaultHttpClient()`).

---

## multipart/form-data

Запросы с загрузкой файлов отправляются как `multipart/form-data`. Библиотека:

1. Формирует body через функции `toMultipart(request, boundary)`
2. Устанавливает `Content-Type: multipart/form-data; boundary=...`
3. Передаёт body в `HttpClient::makeRequest()` с корректным content type

**Методы API с multipart:**

| Метод | Описание |
|-------|----------|
| `setWebhook` | Загрузка SSL-сертификата (`InputFile`) |
| `setChatPhoto` | Установка фото чата |
| `uploadStickerFile` | Загрузка файла стикера |

Для работы с файлами используется `InputFile` (`include/tgbot/types/InputFile.hpp`) — данные, MIME-тип и имя файла. Файл с диска: `InputFile::fromFile(path, mimeType)`.

**multipart/form-data запросы работают** — HTTP-клиент корректно передаёт content type и бинарное содержимое в Telegram API.

---

## Изменения с 26 марта 2026

Базовая точка: коммит `644882a` (26.03.2026, v1.0.5).

### 26 марта — исправление линковки

- Добавлена линковка macOS-фреймворков (`CoreFoundation`, `CFNetwork`, `Security`) для OpenSSL/httplib
- Условная активация vcpkg toolchain через `LOCAL_DEV`

### 29 марта — реструктуризация CMake

- CMakeLists разбиты по модулям (`include/requests/`, `src/types/`, `src/net/` и др.)
- Добавлен `TelegramException` (`TgException`)
- Пути к моделям переведены на абсолютные в CMake

### 30 марта — исправление include-путей

- Починены `#include` в заголовках
- Повышение версии

### 6 апреля — крупное обновление (v1.1.0)

**Реструктуризация заголовков:**

- `include/Bot.hpp` → `include/tgbot/Bot.hpp`
- `include/net/` → `include/tgbot/net/`
- `include/types/` → `include/tgbot/types/`
- `include/requests/` → `include/tgbot/requests/`
- Единый стиль include: `#include <tgbot/...>`

**HTTP-клиент по умолчанию:**

- `LibCoroHttpClient` упрощён: один постоянный `httplib::SSLClient` вместо создания клиента и `thread_pool` на каждый запрос
- `Bot` создаётся без явного `HttpClient` — используется дефолтный клиент
- Сигнатура: `Bot(std::string token, HttpClient& httpClient = _getDefaultHttpClient(), ...)`

**API:**

- Методы `Api` возвращают типы напрямую (`Message::Ptr`, `bool` и т.д.), а не обёртку `TelegramResponse`
- Добавлена поддержка `multipart/form-data` для методов с загрузкой файлов

**Новые модели API (6 апреля):**

- `GetManagedBotTokenRequest`, `ReplaceManagedBotTokenRequest`
- `SavePreparedKeyboardButtonRequest`
- `KeyboardButtonRequestManagedBot`, `ManagedBotCreated`, `ManagedBotUpdated`
- `PollOptionAdded`, `PollOptionDeleted`, `PreparedKeyboardButton`

### 27 апреля — тестирование (v1.1.1)

- Добавлен `tests/manual_test.cpp` — ручной echo-бот (getMe + long poll)
- Целевой `tgbot-test` в CMakeLists.txt
- Финальное повышение версии

---

## Структура репозитория

```
tgbot-lib/
├── CMakeLists.txt          # Корневой CMake, tgbot-lib + tgbot-test
├── vcpkg.json              # Зависимости vcpkg
├── include/tgbot/          # Публичные заголовки
│   ├── Bot.hpp
│   ├── EventBroadcaster.hpp
│   ├── EventHandler.hpp
│   ├── net/                # Api, HttpClient, TgLongPoll, WebhookService
│   ├── requests/           # Структуры запросов API
│   ├── types/              # Типы Telegram API
│   └── tools/              # Утилиты
├── src/                    # Реализации (.cpp)
├── tests/
│   └── manual_test.cpp     # Ручной тест бота
└── PROJECT.md              # Этот файл
```

---

## Теги версий

`v1.0.0` … `v1.0.7`, `v1.1.0`, `v1.1.1`
