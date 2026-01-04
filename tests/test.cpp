#include <gtest/gtest.h>
#include <filesystem>

#include "database.h"
#include "sha256.h"

/* Запланирую че по тестам 
 * 1.1. Добавление юзера (добавляем -> чекаем по логину)
 * 1.2. Добавление юзера - добавление такого же юзера (должно выкатить ошибку)
 * 2.1. Логин юзера - несуществующий
 * 2.2. Логин юзера - неправильный пароль
 * 2.3. Логин юзера - успешный (чекаем что is_online 1)
 * 3.1. Выход юзера - неправильный токен
 * 3.2. Выход юзера - корректный (чекаем что is_online 0)
 * 4. Получение всех / онлайн пользователей:
 *   - генерируем N юзеров, их них M логиним сразу
 *   - чекаем, что одна функция вернет размер N, а вторая - M
 * 5. Получение юзера по параметрам
 *   - генерируем юзера
 *   - пытаемся получить по не тому id, ждем nullopt, также с логином и токеном
 *   - получаем по нужному id, логину, далее логиним юзера и по токену тоже получаем
 * 6. Количество сообщений
 *   - генерируем юзера и отправляем N сообщений (заодно и это потестим)
 *   - чекаем, что метод вернул число N
 * 7. Существование токена
 *   - создаем + логиним юзера
 *   - тест функции на true и false, ну тут все понятно
 * 8. Последние N сообщений
 *   - генерируем M сообщений, дальше смотрим на размер того, что пришло
 *   - попытка запросить X >= M -> M
 *   - попытка запросить X < M -> X
 * 9. Сообщения после id X
 *   - генерируем N сообщений
 *   - циклическая проверка по айди от 0 до N - должно возвращать от N до 0
 * 10. Тест очищения
 */


TEST(ClassTests, get_user_test)
{
    spdlog::set_level(spdlog::level::err);
    Database test("test.db");

    test.clear();

    // Generate and login user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);
    auto res = test.loginUser("testUser", "qwert");

    // Incorrect values
    ASSERT_EQ(test.getUserById(-1), std::nullopt);
    ASSERT_EQ(test.getUserByLogin("dummy"), std::nullopt);
    ASSERT_EQ(test.getUserByToken("dummyToken"), std::nullopt);

    // Correct values
    ASSERT_NE(test.getUserByLogin("testUser"), std::nullopt);
    User getUser = test.getUserByLogin("testUser").value();
    ASSERT_NE(test.getUserById(getUser.id), std::nullopt);
    ASSERT_NE(test.getUserByToken(res.first.token), std::nullopt);

    test.clear();
}

TEST(ClassTests, token_exists_test)
{
    Database test("test.db");

    test.clear();

    // Generate and login user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);
    auto res = test.loginUser("testUser", "qwert");

    // Do test
    ASSERT_EQ(test.isTokenExists("dummy"), false);
    ASSERT_EQ(test.isTokenExists(res.first.token), true);

    test.clear();
}

TEST(ClassTests, clear_test)
{
    Database test("test.db");

    test.clear();

    // Generate and login user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);
    auto res = test.loginUser("testUser", "qwert");
    test.sendMessage(test.getUserByToken(res.first.token)->id, "Test message");

    test.clear();

    ASSERT_EQ(test.getAllUsers().size(), 0);
    ASSERT_EQ(test.getMessageCount(), 0);
}

TEST(ServiceTests, add_test)
{
    Database test("test.db");

    test.clear();

    User user;

    // Normal work
    user.login = "testUser";
    user.password = "qwert";
    
    auto err = test.addUser(user);
    ASSERT_EQ(err.isError, false);

    // Repeat user (must give an error)
    err = test.addUser(user);
    ASSERT_EQ(err.isError, true);

    test.clear();
}

TEST(ServiceTests, login_test)
{
    Database test("test.db");

    test.clear();

    // Generate user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);

    // User does not exist
    auto res = test.loginUser("dummy", "dummy");

    ASSERT_EQ(res.second.isError, true);
    ASSERT_EQ(res.second.errorId, 400);

    // Incorrect password
    res = test.loginUser("testUser", "12345");

    ASSERT_EQ(res.second.isError, true);
    ASSERT_EQ(res.second.errorId, 401);

    // Normal work
    res = test.loginUser("testUser", "qwert");

    ASSERT_EQ(res.second.isError, false);
    ASSERT_EQ(test.getUserByLogin("testUser")->isOnline, true);

    test.clear();
}

TEST(ServiceTests, logout_test)
{
    Database test("test.db");

    test.clear();

    // Generate and login user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);
    auto res = test.loginUser("testUser", "qwert");

    // Incorrect token (for any reason, don't rlly care)
    err = test.logoutUser("dummyToken");

    ASSERT_EQ(err.isError, true);
    ASSERT_EQ(err.errorId, 401);

    // Correct work
    err = test.logoutUser(res.first.token);

    ASSERT_EQ(err.isError, false);
    ASSERT_EQ(test.getUserByLogin("testUser")->isOnline, false);

    test.clear();
}

TEST(ServiceTests, online_count_test)
{
    Database test("test.db");

    test.clear();

    const int N = 50, M = 30;

    // Add & login users (not a test)
    for (int i = 0; i < N; i++)
    {
        const std::string login = "test" + std::to_string(i);
        User user;

        user.login = login;
        user.password = "qwert";

        test.addUser(user);

        if (i < M)
        {
            test.loginUser(login, "qwert");
        }
    }

    // Do a test
    ASSERT_EQ(test.getAllUsers().size(), N);
    ASSERT_EQ(test.getOnlineUsers().size(), M);

    test.clear();
}

TEST(ServiceTests, messages_get_all_ways)
{
    Database test("test.db");

    test.clear();

    // Generate and login user (without test)
    User user;
    user.login = "testUser";
    user.password = "qwert";
    auto err = test.addUser(user);
    auto res = test.loginUser("testUser", "qwert");
    int userId = test.getUserByLogin("testUser")->id;

    // Send messages (passive test, if smt goes wrong, exception will be thrown and test will be failed)
    const int N = 30;

    for (int i = 0; i < N; i++)
    {
        test.sendMessage(userId, "Test text lorem ipsum");
    }

    // Test count
    ASSERT_EQ(test.getMessageCount(), N);

    // Test last messages
    ASSERT_EQ(test.getLastMessages(N / 2).size(), N / 2);
    ASSERT_EQ(test.getLastMessages(N * 2).size(), N);

    // Test messages after
    for (int i = 0; i <= N; i++)
    {
        ASSERT_EQ(test.getMessagesAfter(i).size(), N - i);
    }

    test.clear();
}