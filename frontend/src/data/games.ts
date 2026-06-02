export interface Game {
    id: string;
    title: string;
    iconUrl?: string;
    accentColor: string;
    bannerUrl?: string;
    description?: string;
    descriptionKey?: string;
    badgeKey?: string;
    /** If true, user must purchase the game before installing */
    isPaid?: boolean;
    /** Display price string e.g. "125 ⭐" */
    price?: string;
    config?: {
        type: string;
        version: string;
        modLoader: string;
        modLoaderVersion: string;
        downloadUrl: string;
        useSupabaseGate?: boolean;
    };
}

import icon1 from '../assets/lost_death_1_icon.jpg';
import icon2 from '../assets/lost_death_2_icon.jpg';
import icon3 from '../assets/lost_death_3_icon.jpg';
import banner1 from '../assets/1.gif';
import banner2 from '../assets/2.png';

export const games: Game[] = [
    {
        id: 'lost-death-3',
        title: 'Lost Death 3',
        iconUrl: icon3,
        accentColor: '#FF4500', // 3 is Red
        descriptionKey: 'descLostDeath3',
        badgeKey: 'badgeInDev'
    },
    {
        id: 'lost-death-2',
        title: 'Lost Death 2',
        iconUrl: icon2,
        accentColor: '#1E90FF',
        bannerUrl: banner2,
        descriptionKey: 'descLostDeath2',
        badgeKey: 'badgeBeta',
        isPaid: true,
        price: '50 ⭐',
        config: {
            type: 'minecraft',
            version: '1.20.1',
            modLoader: 'fabric',
            modLoaderVersion: '0.19.2',
            downloadUrl: 'https://github.com/gg5230683-prog/LD2b2/releases/latest/download/LD2.zip',
            useSupabaseGate: true,
        }
    },
    {
        id: 'lost-death-1',
        title: 'Lost Death',
        iconUrl: icon1,
        accentColor: '#7CFC00', // 1 is Green
        bannerUrl: banner1,
        descriptionKey: 'descLostDeath1',
        badgeKey: 'badgeFeatured',
        config: {
            type: 'minecraft',
            version: '1.20.1',
            modLoader: 'fabric',
            modLoaderVersion: '0.19.2',
            downloadUrl: 'https://github.com/gg5230683-prog/LD1/releases/download/2/minecraft.zip',
            useSupabaseGate: true, // PAT_LD1 is resolved server-side via Supabase Edge Function
        }
    },
    {
        id: 'witcher-ld',
        title: 'Witcher LD',
        accentColor: '#C0C0C0',
        descriptionKey: 'descWitcher'
    },
    {
        id: 'cyberpunk-ld',
        title: 'Cyberpunk LD',
        accentColor: '#FFD700',
        descriptionKey: 'descCyberpunk'
    }
];
