import type {ReactNode} from 'react';
import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

type FeatureItem = {
  title: string;
  icon: string;
  description: ReactNode;
};

const FeatureList: FeatureItem[] = [
  {
    title: 'Type-Safe Modding',
    icon: '🛡️',
    description: (
      <>
        PalSchema provides comprehensive type definitions for Palworld, 
        ensuring your mods are robust and less prone to runtime errors.
      </>
    ),
  },
  {
    title: 'Comprehensive Guides',
    icon: '📖',
    description: (
      <>
        Learn how to mod step-by-step. From creating custom items to 
        working with blueprints and raw tables, our guides have you covered.
      </>
    ),
  },
  {
    title: 'Community Driven',
    icon: '🤝',
    description: (
      <>
        Built by the community, for the community. PalSchema is continuously 
        updated by modders who are passionate about exploring the game.
      </>
    ),
  },
];

function Feature({title, icon, description}: FeatureItem) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center">
        <div style={{ fontSize: '4rem', marginBottom: '1rem', filter: 'drop-shadow(0px 4px 6px rgba(0,0,0,0.1))' }}>{icon}</div>
      </div>
      <div className="text--center padding-horiz--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures(): ReactNode {
  return (
    <section className={styles.features} style={{ padding: '2rem 0', width: '100%' }}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
