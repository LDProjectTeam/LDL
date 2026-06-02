import { createClient } from '@supabase/supabase-js';

const supabaseUrl = 'https://bqcmictdinprbjsrjbcq.supabase.co';
const supabaseAnonKey = 'sb_publishable_RjyuKZQ3UVSJfEH4lojJ0w_N5T2Olei'; // This is safe to publish in the frontend

export const supabase = createClient(supabaseUrl, supabaseAnonKey);
