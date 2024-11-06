import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Game Engine",
  description: "A VitePress Site",
  base: '/gengine/',
  // srcDir: "../",
  // srcExclude: ['**/dependencies/**', '**/node_modules/**', '**/README.md'],
  themeConfig: {

    logo: './cpp_logo.png',

    // https://vitepress.dev/reference/default-theme-config
    // nav: [
    //   { text: 'Home', link: '/' },
    //   { text: 'Examples', link: '/markdown-examples' }
    // ],

    sidebar: [
      {
        text: 'Engine Guide',
        items: [
          { text: 'Setup', link: '/setup' },
          { text: 'Examples', link: '/examples/' }
        ]
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/stickyfingies/gengine' }
    ]
  }
})
