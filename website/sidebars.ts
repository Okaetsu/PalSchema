import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

const sidebars: SidebarsConfig = {
  homeSidebar: [
    'features',
    'installation',
    'installing-mods',
    'configuration',
  ],
  documentationSidebar: [
    'gettingstarted',
    'resources',
    {
      type: 'category',
      label: 'Types',
      items: [
        'types/fstring',
        'types/fname',
        'types/ftext',
        'types/boolproperty',
        'types/enumproperty',
        'types/numericproperty',
        'types/arrayproperty',
        'types/mapproperty',
        'types/structproperty',
        'types/objectproperty',
        'types/classproperty',
        'types/softobjectptr',
        'types/softclassptr',
      ],
    },
    {
      type: 'category',
      label: 'Guides',
      items: [
        {
          type: 'category',
          label: 'Items',
          items: [
            'guides/items/creatingabow',
          ],
        },
        {
          type: 'category',
          label: 'Blueprints',
          items: [
            'guides/blueprints/intro',
            'guides/blueprints/components',
          ],
        },
        {
          type: 'category',
          label: 'Raw Tables',
          items: [
            'guides/rawtables/intro',
            'guides/rawtables/wildcard',
          ],
        },
        {
          type: 'category',
          label: 'Buildings',
          items: [
            'guides/buildings/craftingstation',
          ],
        },
        {
          type: 'category',
          label: 'Enums',
          items: [
            'guides/enums/newenums',
          ],
        },
        {
          type: 'category',
          label: 'Localization',
          items: [
            'guides/translations/intro',
          ],
        },
        {
          type: 'category',
          label: 'Pals',
          items: [
            'guides/pals/addingranchsuitabilitynew',
          ],
        },
        {
          type: 'category',
          label: 'Misc',
          items: [
            'guides/resources/importingimages',
            'guides/helpguide/intro',
          ],
        },
        {
          type: 'category',
          label: 'Spawners',
          items: [
            'guides/spawners/overview',

          ],
        },
      ],
    },
  ]
};

export default sidebars;
