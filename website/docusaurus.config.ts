import type {Config} from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

import PrismLight from './src/theme/prismLight';
import PrismDark from './src/theme/prismDark';

const config: Config = {
  title: 'PalSchema',
  tagline: '',
  favicon: 'img/favicon.png',

  // Set the production url of your site here
  url: 'https://okaetsu.github.io',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/PalSchema/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'okaetsu', // Usually your GitHub org/user name.
  projectName: 'PalSchema', // Usually your repo name.

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',
  trailingSlash: false,

  // Even if you don't use internationalization, you can use this field to set
  // useful metadata like html lang. For example, if your site is Chinese, you
  // may want to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.ts',
          showLastUpdateTime: true,
          includeCurrentVersion: true,
          versions: {
            current: {
              label: 'Development',
              path: 'dev',
            }
          },
        },
        blog: {
          showReadingTime: true,
          feedOptions: {
            type: ['rss', 'atom'],
            xslt: true,
          },
          // Useful options to enforce blogging best practices
          onInlineTags: 'warn',
          onInlineAuthors: 'warn',
          onUntruncatedBlogPosts: 'warn',
        },
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    image: 'img/palschema_card.png',
    docs: {
      sidebar: {
        autoCollapseCategories: true,
      }
    },
    navbar: {
      title: 'PalSchema',
      logo: {
        alt: 'Logo',
        src: 'img/palschema.png',
      },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'homeSidebar',
          position: 'left',
          label: 'Home',
        },
        {
          type: 'docSidebar',
          sidebarId: 'documentationSidebar',
          position: 'left',
          label: 'Documentation',
        },
        {
          type: 'docsVersionDropdown',
          position: 'right',
          versions: ['current', '0.6.0'],
        },
        {
          href: 'https://github.com/Okaetsu/PalSchema',
          label: 'GitHub',
          position: 'right',
          className: 'header-github-link',
          'aria-label': 'GitHub Repository',
          'title': 'GitHub Repository'
        },
        {
          href: 'https://discord.gg/pR2yyCJgaY',
          label: 'Discord',
          position: 'right',
          className: 'header-discord-link',
          'aria-label': 'Discord Server',
          'title': 'Discord Server'
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Documentation',
          items: [
            {
              label: 'Getting Started',
              to: '/docs/gettingstarted',
            },
          ],
        },
        {
          title: 'More',
          items: [
            {
              label: 'GitHub',
              href: 'https://github.com/Okaetsu/PalSchema',
            },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} PalSchema, Inc. Built with Docusaurus. Not Affiliated with Pocket Pair, Inc.`,
    },
    prism: {
      theme: PrismLight,
      darkTheme: PrismDark,
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
