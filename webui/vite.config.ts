import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [vue()],
  base: process.env.WEBUI_BASE_URL || '/',
  build: {
    sourcemap: true,
  },
  server: {
    fs: {
      allow: ['..'],
    },
  },
});
