class AuthManager {
    static async login(login, password) {
        try {
            const response = await fetch('/api/auth/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    login: login,
                    password: password
                })
            });

            const data = await response.json();

            if (data.status === "success") {
                Utils.saveToken(data.auth_token);
                return { success: true };
            } else {
                return { 
                    success: false, 
                    error: data.error || 'Ошибка входа' 
                };
            }
        } catch (error) {
            console.error('Login error:', error);
            return { 
                success: false, 
                error: 'Ошибка сети. Попробуйте позже.' 
            };
        }
    }

    static async register(userData) {
        try {
            const response = await fetch('/api/auth/register', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(userData)
            });

            const data = await response.json();
            return data;
        } catch (error) {
            console.error('Register error:', error);
            return { 
                success: false, 
                error: 'Ошибка сети. Попробуйте позже.' 
            };
        }
    }

    static async logout() {
        try {
            const token = await Utils.getToken();
            if (token) {
                await fetch('/api/auth/logout', {
                    method: 'POST',
                    headers: {
                        'Authorization-Token': `${token}`
                    }
                });
            }
        } catch (error) {
            console.error('Logout error:', error);
        } finally {
            Utils.removeToken();
        }
    }
}

// Инициализация страницы входа
function initLoginPage() {
    const loginForm = document.getElementById('loginForm');
    const loginBtn = document.getElementById('loginBtn');

    // Перенаправить если уже авторизован
    Utils.redirectIfAuthenticated();

    loginForm.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const login = document.getElementById('login').value;
        const password = document.getElementById('password').value;

        // Валидация
        if (!login || !password) {
            Utils.showMessage('message', 'Заполните все поля', 'error');
            return;
        }

        // Показать loading
        Utils.setButtonLoading(loginBtn, true);

        // Выполнить вход
        const result = await AuthManager.login(login, password);

        // Скрыть loading
        Utils.setButtonLoading(loginBtn, false);

        if (result.success) {
            Utils.showMessage('message', 'Вход выполнен! Перенаправление...', 'success');
            setTimeout(() => {
                window.location.href = '/chat';
            }, 500);
        } else {
            Utils.showMessage('message', result.error, 'error');
        }
    });
}

function initRegisterPage() {
    const registerForm = document.getElementById('registerForm');
    const registerBtn = document.getElementById('registerBtn');
    const passwordInput = document.getElementById('password');
    const confirmPasswordInput = document.getElementById('confirmPassword');

    // Перенаправить если уже авторизован
    Utils.redirectIfAuthenticated();

    // Валидация паролей в реальном времени
    confirmPasswordInput.addEventListener('input', function() {
        validatePasswordMatch();
    });

    passwordInput.addEventListener('input', function() {
        validatePasswordMatch();
    });

    // Обработка отправки формы
    registerForm.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const formData = {
            login: document.getElementById('login').value.trim(),
            password: document.getElementById('password').value,
            first_name: document.getElementById('firstName').value.trim(),
            last_name: document.getElementById('lastName').value.trim()
        };

        // Валидация формы
        const validation = validateForm(formData);
        if (!validation.isValid) {
            Utils.showMessage('message', validation.errors.join(', '), 'error');
            return;
        }

        // Показать loading
        Utils.setButtonLoading(registerBtn, true);

        // Выполнить регистрацию
        const result = await AuthManager.register(formData);

        // Скрыть loading
        Utils.setButtonLoading(registerBtn, false);

        if (result.success) {
            Utils.showMessage('message', 'Регистрация успешна! Выполняется вход...', 'success');
            
            // Автоматически входим после регистрации
            setTimeout(async () => {
                const loginResult = await AuthManager.login(formData.login, formData.password);
                if (loginResult.success) {
                    window.location.href = '/chat.html';
                } else {
                    window.location.href = '/';
                }
            }, 1500);
            
        } else {
            Utils.showMessage('message', result.error || 'Ошибка регистрации', 'error');
        }
    });
}

// Валидация совпадения паролей в реальном времени
function validatePasswordMatch() {
    const password = document.getElementById('password').value;
    const confirmPassword = document.getElementById('confirmPassword').value;
    const passwordMatch = document.getElementById('passwordMatch');
    
    if (confirmPassword.length > 0) {
        if (password === confirmPassword) {
            passwordMatch.textContent = '✓ Пароли совпадают';
            passwordMatch.className = 'hint password-match';
            Utils.toggleElement('passwordMatch', true);
        } else {
            passwordMatch.textContent = '✗ Пароли не совпадают';
            passwordMatch.className = 'hint password-mismatch';
            Utils.toggleElement('passwordMatch', true);
        }
    } else {
        Utils.toggleElement('passwordMatch', false);
    }
}

// Валидация всей формы
function validateForm(formData) {
    const errors = [];

    // Валидация логина
    if (!formData.login) {
        errors.push('Логин обязателен');
    } else if (!Utils.validateLogin(formData.login)) {
        errors.push('Логин должен содержать от 3 до 20 символов (только буквы, цифры и подчеркивание)');
    }

    // Валидация пароля
    if (!formData.password) {
        errors.push('Пароль обязателен');
    } else if (!Utils.validatePassword(formData.password)) {
        errors.push('Пароль должен содержать не менее 6 символов');
    }

    // Валидация подтверждения пароля
    const confirmPassword = document.getElementById('confirmPassword').value;
    if (!Utils.validatePasswordMatch(formData.password, confirmPassword)) {
        errors.push('Пароли не совпадают');
    }

    // Валидация имени
    if (!formData.first_name) {
        errors.push('Имя обязательно');
    } else if (!Utils.validateName(formData.first_name)) {
        errors.push('Имя должно содержать от 2 до 50 символов');
    }

    // Валидация фамилии
    if (!formData.last_name) {
        errors.push('Фамилия обязательна');
    } else if (!Utils.validateName(formData.last_name)) {
        errors.push('Фамилия должна содержать от 2 до 50 символов');
    }

    return {
        isValid: errors.length === 0,
        errors: errors
    };
}

// Валидация в реальном времени для логина
document.addEventListener('DOMContentLoaded', function() {
    const loginInput = document.getElementById('login');
    if (loginInput) {
        loginInput.addEventListener('input', function() {
            const login = this.value;
            if (login.length > 0 && !Utils.validateLogin(login)) {
                this.style.borderColor = '#e74c3c';
            } else if (login.length > 0) {
                this.style.borderColor = '#27ae60';
            }
        });
    }
});