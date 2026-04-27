//
// Created by Richard Dzubko on 27.04.2026.
//

#include <coro/sync_wait.hpp>
#include <coro/thread_pool.hpp>
#include <tgbot/Bot.hpp>
#include <tgbot/net/TgLongPoll.hpp>
#include <cstdlib>

const TgBot::Bot bot{std::getenv("MVD_BOT_TOKEN")};

coro::task<> testMe() {
    auto me = co_await bot.getApi().getMe();
    printf("Bot username: %s\n", me->username.c_str());
}

coro::task<> testEcho() {
    bot.getEvents().onAnyMessage([](TgBot::Message::Ptr message) -> coro::task<> {
        printf("Recieved message: %s\n", message->text.c_str());
        co_await bot.getApi().sendMessage(message->chat->id, "Echo: " + message->text);
    });

    TgBot::TgLongPoll poll(bot);
    while (true) {
        try {
            printf("Long poll started...\n");
            poll.start();
        } catch (const std::exception& e) {
            printf("Error: %s\n", e.what());
        }
    }
}

int main() {
    coro::sync_wait(testMe());
    coro::sync_wait(testEcho());

    return 0;
}