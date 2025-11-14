class ChatManager {
    constructor() {
        this.lastMessageId = 0;
        this.currentUser = null;
        this.refreshInterval = null;
        this.onlineRefreshInterval = null;
        this.isAutoScroll = true;
    }

    // Инициализация чата
    async init() {
        // Проверяем авторизацию
        if (!Utils.isAuthenticated()) {
            window.location.href = '/';
            return;
        }

        try {
            // Загружаем данные пользователя
            await this.loadUserData();
            
            // Загружаем историю сообщений
            await this.loadMessages();
            
            // Загружаем онлайн пользователей
            await this.loadOnlineUsers();
            
            // Загружаем статистику
            await this.loadStats();
            
            // Настраиваем обновления
            this.setupAutoRefresh();
            
            // Настраиваем обработчики событий
            this.setupEventListeners();
            
            // Показываем информационное модальное окно для новых пользователей
            this.showWelcomeModal();
            
        } catch (error) {
            console.error('Error initializing chat:', error);
            Utils.showMessage('Ошибка загрузки чата', 'error');
        }
    }

    // Загрузка данных пользователя
    async loadUserData() {
        try {
            const response = await fetch('/api/users/me', {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });

            if (!response.ok) {
                throw new Error('Failed to load user data');
            }

            const userData = await response.json();
            this.currentUser = userData;
            
            // Обновляем интерфейс
            document.getElementById('currentUser').textContent = 
                `${userData.first_name} ${userData.last_name} (@${userData.login})`;
                
        } catch (error) {
            console.error('Error loading user data:', error);
            throw error;
        }
    }

    // Загрузка сообщений
    async loadMessages() {
        try {
            const response = await fetch(`/api/messages?limit=100`, {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });

            if (!response.ok) {
                throw new Error('Failed to load messages');
            }

            const data = await response.json();
            this.displayMessages(data.messages);
            
            // Обновляем ID последнего сообщения
            if (data.messages.length > 0) {
                this.lastMessageId = data.messages[data.messages.length - 1].id;
            }
            
        } catch (error) {
            console.error('Error loading messages:', error);
            throw error;
        }
    }

    // Загрузка новых сообщений
    async loadNewMessages() {
        try {
            const response = await fetch(`/api/messages/new?after_id=${this.lastMessageId}`, {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });

            if (!response.ok) return;

            const data = await response.json();
            
            if (data.messages && data.messages.length > 0) {
                this.displayMessages(data.messages, true);
                this.lastMessageId = data.messages[data.messages.length - 1].id;
                
                // Воспроизводим звук нового сообщения (опционально)
                this.playNotificationSound();
            }
            
        } catch (error) {
            console.error('Error loading new messages:', error);
        }
    }

    // Отправка сообщения
    async sendMessage(messageText) {
        const sendBtn = document.getElementById('sendBtn');
        const messageInput = document.getElementById('messageInput');
        
        // Показываем loading
        Utils.setButtonLoading(sendBtn, true);
        
        try {
            const response = await fetch('/api/messages', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${Utils.getToken()}`
                },
                body: JSON.stringify({
                    message_text: messageText
                })
            });

            if (!response.ok) {
                throw new Error('Failed to send message');
            }

            // Очищаем поле ввода
            messageInput.value = '';
            
            // Обновляем сообщения
            await this.loadNewMessages();
            
        } catch (error) {
            console.error('Error sending message:', error);
            Utils.showMessage('Ошибка отправки сообщения', 'error');
        } finally {
            // Скрываем loading
            Utils.setButtonLoading(sendBtn, false);
        }
    }

    // Отображение сообщений
    displayMessages(messages, append = false) {
        const container = document.getElementById('messagesContainer');
        
        if (!append) {
            container.innerHTML = '';
        }

        if (messages.length === 0) {
            if (!append) {
                container.innerHTML = '<div class="loading">Нет сообщений. Будьте первым!</div>';
            }
            return;
        }

        messages.forEach(message => {
            const messageElement = this.createMessageElement(message);
            container.appendChild(messageElement);
        });

        // Автопрокрутка к последнему сообщению
        if (this.isAutoScroll) {
            this.scrollToBottom();
        }
    }

    // Создание элемента сообщения
    createMessageElement(message) {
        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${message.user_id === this.currentUser.id ? 'own' : ''}`;
        
        const avatarText = this.getAvatarText(message.user_login);
        const time = this.formatTime(message.timestamp);
        
        messageDiv.innerHTML = `
            <div class="message-header">
                <div class="message-avatar">${avatarText}</div>
                <div>
                    <span class="message-user">${message.user_login}</span>
                    <span class="message-time">${time}</span>
                </div>
            </div>
            <div class="message-content">${this.escapeHtml(message.message_text)}</div>
        `;
        
        return messageDiv;
    }

    // Загрузка онлайн пользователей
    async loadOnlineUsers() {
        try {
            const response = await fetch('/api/users/online', {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });

            if (!response.ok) return;

            const data = await response.json();
            this.displayOnlineUsers(data.online_users);
            document.getElementById('onlineCount').textContent = data.total_online;
            
        } catch (error) {
            console.error('Error loading online users:', error);
        }
    }

    // Отображение онлайн пользователей
    displayOnlineUsers(users) {
        const container = document.getElementById('onlineUsers');
        
        if (users.length === 0) {
            container.innerHTML = '<div class="loading">Нет пользователей онлайн</div>';
            return;
        }

        container.innerHTML = users.map(user => `
            <div class="user-item">
                <div class="user-avatar">${this.getAvatarText(user.login)}</div>
                ${user.login}
            </div>
        `).join('');
    }

    // Загрузка статистики
    async loadStats() {
        try {
            const messagesResponse = await fetch('/api/messages/count', {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });
            
            const usersResponse = await fetch('/api/users/count', {
                headers: {
                    'Authorization': `Bearer ${Utils.getToken()}`
                }
            });

            if (messagesResponse.ok) {
                const messagesData = await messagesResponse.json();
                document.getElementById('totalMessages').textContent = messagesData.count;
            }

            if (usersResponse.ok) {
                const usersData = await usersResponse.json();
                document.getElementById('totalUsers').textContent = usersData.count;
            }
            
        } catch (error) {
            console.error('Error loading stats:', error);
        }
    }

    // Настройка автоматического обновления
    setupAutoRefresh() {
        // Обновление сообщений каждые 3 секунды
        this.refreshInterval = setInterval(() => {
            this.loadNewMessages();
        }, 3000);

        // Обновление онлайн пользователей каждые 10 секунд
        this.onlineRefreshInterval = setInterval(() => {
            this.loadOnlineUsers();
        }, 10000);
    }

    // Настройка обработчиков событий
    setupEventListeners() {
        const messageForm = document.getElementById('messageForm');
        const messageInput = document.getElementById('messageInput');
        const logoutBtn = document.getElementById('logoutBtn');
        const closeModal = document.getElementById('closeModal');
        const infoModal = document.getElementById('infoModal');

        // Отправка сообщения
        messageForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const message = messageInput.value.trim();
            if (message) {
                this.sendMessage(message);
            }
        });

        // Обработка Enter и Shift+Enter
        messageInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                const message = messageInput.value.trim();
                if (message) {
                    this.sendMessage(message);
                }
            }
        });

        // Выход из системы
        logoutBtn.addEventListener('click', async () => {
            if (confirm('Вы уверены, что хотите выйти?')) {
                await AuthManager.logout();
                window.location.href = '/';
            }
        });

        // Закрытие модального окна
        closeModal.addEventListener('click', () => {
            infoModal.classList.add('hidden');
        });

        // Закрытие модального окна по клику вне его
        infoModal.addEventListener('click', (e) => {
            if (e.target === infoModal) {
                infoModal.classList.add('hidden');
            }
        });

        // Отслеживание прокрутки для отключения автоскролла
        const messagesContainer = document.getElementById('messagesContainer');
        messagesContainer.addEventListener('scroll', () => {
            const isAtBottom = messagesContainer.scrollHeight - messagesContainer.clientHeight <= messagesContainer.scrollTop + 10;
            this.isAutoScroll = isAtBottom;
        });
    }

    // Вспомогательные методы

    getAvatarText(login) {
        return login.substring(0, 2).toUpperCase();
    }

    formatTime(timestamp) {
        const date = new Date(timestamp);
        return date.toLocaleTimeString('ru-RU', { 
            hour: '2-digit', 
            minute: '2-digit' 
        });
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    scrollToBottom() {
        const container = document.getElementById('messagesContainer');
        container.scrollTop = container.scrollHeight;
    }

    playNotificationSound() {
        // Простой звук уведомления (можно заменить на реальный звуковой файл)
        try {
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const oscillator = audioContext.createOscillator();
            const gainNode = audioContext.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioContext.destination);
            
            oscillator.frequency.value = 800;
            gainNode.gain.value = 0.1;
            
            oscillator.start();
            gainNode.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 0.1);
            oscillator.stop(audioContext.currentTime + 0.1);
        } catch (error) {
            console.log('Audio not supported');
        }
    }

    showWelcomeModal() {
        // Показываем модальное окно только для новых пользователей
        const hasSeenWelcome = localStorage.getItem('chat_welcome_seen');
        if (!hasSeenWelcome) {
            const infoModal = document.getElementById('infoModal');
            infoModal.classList.remove('hidden');
            localStorage.setItem('chat_welcome_seen', 'true');
        }
    }

    // Очистка при размонтировании
    destroy() {
        if (this.refreshInterval) {
            clearInterval(this.refreshInterval);
        }
        if (this.onlineRefreshInterval) {
            clearInterval(this.onlineRefreshInterval);
        }
    }
}

// Глобальная инициализация
let chatManager;

function initChat() {
    chatManager = new ChatManager();
    chatManager.init();
}

// Очистка при закрытии страницы
window.addEventListener('beforeunload', () => {
    if (chatManager) {
        chatManager.destroy();
    }
});