import { sveltekit } from '@sveltejs/kit/vite';
import type { UserConfig } from 'vite';
import UnoCSS from '@unocss/vite'
import presetIcons from '@unocss/preset-icons'
import presetUno from '@unocss/preset-uno'

const config: UserConfig = {
  plugins: [
    UnoCSS({
      presets: [
        presetUno({}),
        presetIcons({}),
      ]
    }),
    sveltekit(),
  ]
};

export default config;
