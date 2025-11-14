// Вспомогательные функции
class Utils {
    // Показать сообщение
    static showMessage(elementId, message, type = 'error') {
        const messageEl = document.getElementById(elementId);
        messageEl.textContent = message;
        messageEl.className = `message ${type}`;
        messageEl.classList.remove('hidden');
        
        // Автоматически скрыть через 5 секунд
        setTimeout(() => {
            messageEl.classList.add('hidden');
        }, 5000);
    }

    // Показать/скрыть loading состояние кнопки
    static setButtonLoading(button, isLoading) {
        const btnText = button.querySelector('.btn-text');
        const btnLoading = button.querySelector('.btn-loading');
        
        if (isLoading) {
            btnText.style.display = 'none';
            btnLoading.style.display = 'inline-block';
            button.disabled = true;
        } else {
            btnText.style.display = 'inline-block';
            btnLoading.style.display = 'none';
            button.disabled = false;
        }
    }

    // Сохранить токен в localStorage
    static saveToken(token) {
        localStorage.setItem('chat_token', token);
    }

    // Получить токен из localStorage
    static async getToken() {
        let token = localStorage.getItem('chat_token');

        if (token === null) {
            return null;
        }

        try {
            const response = await fetch('/api/check_token', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    token: token
                })
            });

            if (response.status !== 200) {
                throw Error("Failed to fetch check token!");
            }

            const data = await response.json();


            if (data.check_status === true) {
                return token;
            } else {
                this.removeToken();
                return null;
            }
        } catch (error) {
            console.error('Check token error:', error);
            return null;
        }
    }

    // Удалить токен (выход)
    static removeToken() {
        localStorage.removeItem('chat_token');
    }

    // Проверить авторизацию
    static async isAuthenticated() {
        const data = await this.getToken();

        if (data === null) {
            return false;
        }
        return true;
    }

    // Перенаправить если авторизован
    static async redirectIfAuthenticated() {
        if (await this.isAuthenticated()) {
            window.location.href = '/chat';
        }
    }

    static validateLogin(login) {
        const loginRegex = /^[a-zA-Z0-9_]{3,20}$/;
        return loginRegex.test(login);
    }

    // Валидация пароля
    static validatePassword(password) {
        return password.length >= 6;
    }

    // Проверка совпадения паролей
    static validatePasswordMatch(password, confirmPassword) {
        return password === confirmPassword;
    }

    // Валидация имени
    static validateName(name) {
        return name.length >= 2 && name.length <= 50;
    }

    // Показать/скрыть элемент
    static toggleElement(elementId, show) {
        const element = document.getElementById(elementId);
        if (show) {
            element.classList.remove('hidden');
        } else {
            element.classList.add('hidden');
        }
    }

    // Установить класс элемента
    static setElementClass(elementId, className) {
        const element = document.getElementById(elementId);
        element.className = className;
    }
}