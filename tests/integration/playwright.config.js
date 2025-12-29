import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: './',
  timeout: 60000,
  fullyParallel: false,
  retries: 0,
  workers: 1,
  reporter: 'line',
  use: {
    baseURL: 'http://localhost:8080',
    trace: 'on-first-retailure',
    screenshot: 'only-on-failure',
  },
  webServer: {
    command: 'npm run server:start',
    url: 'http://localhost:8080',
    reuseExistingServer: !process.env.CI,
    timeout: 60000,
  },
});