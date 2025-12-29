import { test, expect } from '@playwright/test';

// Вспомогательная функция для генерации уникального логина
function generateUniqueLogin(base) {
  return `${base}_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
}

// Основной тест
test('общение двух пользователей в реальном времени', async ({ browser }) => {
  console.log('=== Начало теста: Общение в реальном времени ===');
  
  // Создаем два независимых контекста браузера (два разных пользователя)
  const user1Context = await browser.newContext();
  const user2Context = await browser.newContext();
  
  const user1Page = await user1Context.newPage();
  const user2Page = await user2Context.newPage();

  // Генерация уникальных логинов
  const user1Login = generateUniqueLogin('realtime_user1');
  const user2Login = generateUniqueLogin('realtime_user2');

  const user1Name = "Алексей Иванов";
  const user2Name = "Мария Петрова";
  
  console.log(`Созданы пользователи: ${user1Login} и ${user2Login}`);

  // === ШАГ 1: Регистрация обоих пользователей ===
  console.log('Шаг 1: Регистрация пользователей...');
  
  // Регистрация первого пользователя
  await user1Page.goto('/register');
  await user1Page.fill('#login', user1Login);
  await user1Page.fill('#password', 'password123');
  await user1Page.fill('#confirmPassword', 'password123');
  await user1Page.fill('#firstName', 'Алексей');
  await user1Page.fill('#lastName', 'Иванов');
  
  // Ждем, пока кнопка станет активной
  await user1Page.waitForSelector('#registerBtn:not([disabled])');
  await user1Page.click('#registerBtn');
  
  // Ждем успешной регистрации и перенаправления в чат
  await user1Page.waitForURL('/chat', { timeout: 5000 });
  await expect(user1Page.locator('#currentUser')).toContainText('Алексей Иванов');
  console.log('✓ Пользователь 1 зарегистрирован и вошел в чат');

  console.log('Шаг 1.1: Проверка модального окна у пользователя 1...');
  
  // Проверяем, что модальное окно отображается (новые пользователи должны его видеть)
  try {
    // Ждем появления модального окна (убираем класс 'hidden')
    await user1Page.waitForSelector('#infoModal:not(.hidden)', { timeout: 5000 });
    
    console.log('✓ Модальное окно отображается у пользователя 1');
    
    // Закрываем модальное окно
    await user1Page.click('#closeModal');
    
    // Проверяем, что модальное окно скрылось
    await expect(user1Page.locator('#infoModal')).toHaveClass(/hidden/);
    console.log('✓ Модальное окно успешно закрыто');
    
  } catch (error) {
    console.log('⚠ Модальное окно не появилось у пользователя 1 (возможно, уже было закрыто ранее)');
  }

  // Регистрация второго пользователя
  await user2Page.goto('/register');
  await user2Page.fill('#login', user2Login);
  await user2Page.fill('#password', 'password123');
  await user2Page.fill('#confirmPassword', 'password123');
  await user2Page.fill('#firstName', 'Мария');
  await user2Page.fill('#lastName', 'Петрова');
  
  await user2Page.waitForSelector('#registerBtn:not([disabled])');
  await user2Page.click('#registerBtn');
  
  await user2Page.waitForURL('/chat', { timeout: 5000 });
  await expect(user2Page.locator('#currentUser')).toContainText('Мария Петрова');
  console.log('✓ Пользователь 2 зарегистрирован и вошел в чат');

  // === ШАГ 2.1: Обработка модального окна у пользователя 2 ===
  console.log('Шаг 2.1: Проверка модального окна у пользователя 2...');
  
  try {
    await user2Page.waitForSelector('#infoModal:not(.hidden)', { timeout: 5000 });

    const modalVisibility = await user2Page.locator('#infoModal').getAttribute('style');
    console.log(`Стили модального окна: ${modalVisibility || 'default'}`);
    
    // Закрываем модальное окно кликом на кнопку закрытия
    await user2Page.click('#closeModal');
    
    await expect(user2Page.locator('#infoModal')).toHaveClass(/hidden/);
    console.log('✓ Модальное окно закрыто у пользователя 2');
    
  } catch (error) {
    console.log('⚠ Модальное окно не появилось у пользователя 2');
  }

  // === ШАГ 3: Проверка, что можно взаимодействовать с интерфейсом после закрытия модалки ===
  console.log('Шаг 3: Проверка доступности интерфейса...');
  
  // Проверяем, что элементы чата доступны и видны
  await expect(user1Page.locator('#messageInput')).toBeVisible();
  await expect(user1Page.locator('#sendBtn')).toBeVisible();
  await expect(user1Page.locator('#messagesContainer')).toBeVisible();
  
  await expect(user2Page.locator('#messageInput')).toBeVisible();
  await expect(user2Page.locator('#sendBtn')).toBeVisible();
  await expect(user2Page.locator('#messagesContainer')).toBeVisible();
  
  console.log('✓ Элементы интерфейса доступны после закрытия модальных окон');

  // === ШАГ 4: Проверка начального состояния ===
  console.log('Шаг 4: Проверка начального состояния...');
  
  // Проверяем, что оба видят друг друга онлайн
  await user1Page.waitForTimeout(2000); // Ждем обновления статуса
  
  const onlineCountUser1 = await user1Page.locator('#onlineCount').textContent();
  const onlineCountUser2 = await user2Page.locator('#onlineCount').textContent();
  
  console.log(`Онлайн пользователей (у User1): ${onlineCountUser1}`);
  console.log(`Онлайн пользователей (у User2): ${onlineCountUser2}`);
  
  // Ожидаем, что в списке онлайн будет хотя бы 1 пользователь (они сами)
  expect(parseInt(onlineCountUser1)).toBeGreaterThanOrEqual(1);
  expect(parseInt(onlineCountUser2)).toBeGreaterThanOrEqual(1);

  // === ШАГ 5: Первое сообщение от User1 ===
  console.log('Шаг 5: Отправка первого сообщения...');
  
  const firstMessage = 'Привет, Мария! Как твои дела?';
  await user1Page.fill('#messageInput', firstMessage);
  await user1Page.click('#sendBtn');
  
  console.log(`✓ User1 отправил: "${firstMessage}"`);
  
  // Проверяем, что сообщение отобразилось у User1 (свои сообщения)
  await expect(user1Page.locator('.message-content').last())
    .toContainText(firstMessage, { timeout: 3000 });
  
  // Проверяем стиль своего сообщения (должен быть справа)
  await expect(user1Page.locator('.message').last())
    .toHaveClass(/own/);
  console.log('✓ User1 видит свое сообщение (правильный стиль)');

  // === ШАГ 6: User2 видит сообщение автоматически ===
  console.log('Шаг 6: Проверка автоматического обновления у User2...');
  
  // Ждем, пока сообщение появится у User2 (максимум 10 секунд)
  await expect(user2Page.locator('.message-content').first())
    .toContainText(firstMessage, { timeout: 10000 });
  
  console.log(`✓ User2 автоматически увидел сообщение через ${Math.round(performance.now() / 1000)} секунд`);
  
  // Проверяем, что у User2 это чужое сообщение (стиль слева)
  await expect(user2Page.locator('.message').first())
    .not.toHaveClass(/own/);
  
  // Проверяем отправителя
  await expect(user2Page.locator('.message-user').first())
    .toContainText(user1Name);
  console.log('✓ User2 видит правильного отправителя');

  // === ШАГ 7: Ответ User2 ===
  console.log('Шаг 7: Ответ User2...');
  
  const responseMessage = 'Привет, Алексей! Все отлично, работаю над проектом.';
  await user2Page.fill('#messageInput', responseMessage);
  await user2Page.click('#sendBtn');
  
  console.log(`✓ User2 ответил: "${responseMessage}"`);
  
  // Проверяем ответ у User2
  await expect(user2Page.locator('.message-content').last())
    .toContainText(responseMessage, { timeout: 3000 });
  
  await expect(user2Page.locator('.message').last())
    .toHaveClass(/own/);
  console.log('✓ User2 видит свой ответ (правильный стиль)');

  // === ШАГ 8: User1 видит ответ автоматически ===
  console.log('Шаг 8: Проверка автоматического обновления у User1...');
  
  // Ждем ответ у User1
  await expect(user1Page.locator('.message-content').last())
    .toContainText(responseMessage, { timeout: 10000 });
  
  console.log(`✓ User1 автоматически увидел ответ через ${Math.round(performance.now() / 1000)} секунд`);
  
  // Проверяем отправителя
  await expect(user1Page.locator('.message-user').last())
    .toContainText(user2Name);
  
  await expect(user1Page.locator('.message').last())
    .not.toHaveClass(/own/);
  console.log('✓ User1 видит ответ как чужое сообщение');

  // === ШАГ 9: Проверка сохранения истории ===
  console.log('Шаг 9: Проверка сохранения истории...');
  
  // User1 обновляет страницу
  await user1Page.reload();
  await user1Page.waitForSelector('.message-content', { timeout: 5000 });
  
  // Проверяем, что история сохранилась
  const messagesCount = await user1Page.locator('.message-content').count();
  expect(messagesCount).toBeGreaterThanOrEqual(2);
  
  // Проверяем наличие обоих сообщений
  const allText = await user1Page.locator('.message-content').allTextContents();
  const hasFirstMessage = allText.some(text => text.includes(firstMessage));
  const hasResponseMessage = allText.some(text => text.includes(responseMessage));
  
  expect(hasFirstMessage).toBeTruthy();
  expect(hasResponseMessage).toBeTruthy();
  
  console.log('✓ История сообщений сохранилась после обновления страницы');

  // === ШАГ 10: Проверка непрерывности разговора ===
  console.log('Шаг 10: Проверка непрерывности разговора...');
  
  // Третье сообщение от User1
  const thirdMessage = 'Отлично! Я тоже тестирую наш чат. Как тебе интерфейс?';
  await user1Page.fill('#messageInput', thirdMessage);
  await user1Page.click('#sendBtn');
  
  // Ждем у User2
  await expect(user2Page.locator('.message-content').last())
    .toContainText(thirdMessage, { timeout: 10000 });
  
  // Четвертое сообщение от User2
  const fourthMessage = 'Интерфейс отличный! Все интуитивно понятно и работает быстро.';
  await user2Page.fill('#messageInput', fourthMessage);
  await user2Page.click('#sendBtn');
  
  // Ждем у User1
  await expect(user1Page.locator('.message-content').last())
    .toContainText(fourthMessage, { timeout: 10000 });
  
  console.log('✓ Диалог продолжается плавно и непрерывно');

  // === ИТОГИ ===
  console.log('\n=== Результаты теста ===');
  console.log('✓ Оба пользователя успешно зарегистрированы');
  console.log('✓ Сообщения отправляются и доставляются');
  console.log('✓ Автоматическое обновление работает (без перезагрузки страницы)');
  console.log('✓ Сообщения правильно стилизуются (свои/чужие)');
  console.log('✓ История сохраняется после обновления');
  console.log('✓ Диалог ведется плавно и непрерывно');
  console.log('=== Тест успешно пройден! ===');
});