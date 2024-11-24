import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Game Engine",
  description: "Build cross-platform games with C++",

  // This website is served at https://domain.com/gengine/
  base: '/gengine/',

  themeConfig: {

    logo: './cpp_logo.png',

    // https://vitepress.dev/reference/default-theme-config
    // nav: [
    //   { text: 'Home', link: '/' },
    //   { text: 'Examples', link: '/markdown-examples' }
    // ],

    sidebar: [
      {
        text: 'Guide',
        items: [
          { text: 'Setup', link: '/setup' },
          { text: 'Examples', link: '/examples/' },
        ]
      },
      {
        text: 'API',
        items: [
          { text: 'Core', link: '/core/' },
          { text: 'GPU', link: '/gpu/' },
        ],
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/stickyfingies/gengine' }
    ]
  }
})
