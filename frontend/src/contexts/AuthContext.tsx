import React, {
  createContext,
  useContext,
  useState,
  useEffect,
  ReactNode,
} from "react";
import { supabase } from "../supabaseClient";

export interface User {
  username: string;
  email?: string;
  avatarUrl?: string;
  showEmail?: boolean;
  status?: "online" | "offline";
  minecraftLicense?: any;
}

interface AuthContextType {
  user: User | null;
  logout: () => Promise<void>;
  updateStatus: (status: "online" | "offline") => void;
  linkMicrosoft: (profile: any) => Promise<void>;
  unlinkMicrosoft: () => Promise<void>;
  updateProfile: (newUsername: string, newAvatarUrl?: string) => Promise<void>;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider = ({ children }: { children: ReactNode }) => {
  const [user, setUser] = useState<User | null>(null);

  const linkMicrosoft = async (profile: any) => {
    const { error } = await supabase.auth.updateUser({
      data: {
        minecraft_license: profile,
      },
    });
    if (error) throw error;
    setUser((prev) => (prev ? { ...prev, minecraftLicense: profile } : null));
  };

  const unlinkMicrosoft = async () => {
    const { error } = await supabase.auth.updateUser({
      data: { minecraft_license: null },
    });
    if (error) throw error;
    setUser((prev) => (prev ? { ...prev, minecraftLicense: undefined } : null));
  };

  useEffect(() => {
    // Fetch initial session
    supabase.auth.getSession().then(({ data: { session } }) => {
      if (session) {
        mapSessionToUser(session);
      }
    });

    // Listen for Supabase auth state changes (Google OAuth & Email OTP)
    const {
      data: { subscription },
    } = supabase.auth.onAuthStateChange((_event, session) => {
      if (session) {
        mapSessionToUser(session);
      } else {
        setUser(null);
      }
    });

    return () => subscription.unsubscribe();
  }, []);

  const mapSessionToUser = (session: any) => {
    const metadata = session.user.user_metadata;
    const email = session.user.email;

    // IMPORTANT: Read ldl_username FIRST.
    // Google OAuth overwrites metadata.name and metadata.full_name on every login.
    // Our custom name lives in ldl_username so it survives re-logins.
    const username =
      metadata.ldl_username ||
      metadata.full_name ||
      metadata.name ||
      metadata.username ||
      metadata.user_name ||
      (email ? email.split("@")[0] : "User");

    // ldl_avatar is our custom field — Google never sends it, so it survives re-logins.
    // Fall back to Google's picture / avatar_url only when no custom avatar is set.
    const avatarUrl =
      metadata.ldl_avatar ||
      metadata.avatar_url ||
      metadata.picture;

    const minecraftLicense = metadata.minecraft_license;

    setUser((prev) => ({
      username,
      email,
      avatarUrl,
      showEmail: true,
      status: prev?.status ?? (navigator.onLine ? "online" : "offline"),
      minecraftLicense,
    }));
  };

  const updateStatus = (status: "online" | "offline") => {
    setUser((prev) => (prev ? { ...prev, status } : null));
  };

  /**
   * Updates user display name and optional avatar URL.
   * Persists to Supabase user_metadata and immediately updates local state
   * so the UI reflects changes without waiting for onAuthStateChange.
   */
  const updateProfile = async (newUsername: string, newAvatarUrl?: string): Promise<void> => {
    // IMPORTANT: Google OAuth overwrites 'name', 'full_name', 'avatar_url', and 'picture'
    // on every login. To ensure user customizations survive, we store them in custom fields:
    // 'ldl_username' and 'ldl_avatar'.
    const updateData: Record<string, string> = {
      ldl_username: newUsername,
      name: newUsername, // Also update these just in case other things read them
      full_name: newUsername,
    };
    if (newAvatarUrl) {
      updateData.ldl_avatar = newAvatarUrl;
      updateData.avatar_url = newAvatarUrl;
      updateData.picture = newAvatarUrl;
    }

    const { error } = await supabase.auth.updateUser({ data: updateData });
    if (error) throw error;

    // Immediately reflect in local state — don't wait for onAuthStateChange
    setUser((prev) =>
      prev
        ? {
            ...prev,
            username: newUsername,
            ...(newAvatarUrl ? { avatarUrl: newAvatarUrl } : {}),
          }
        : null
    );
  };

  const logout = async () => {
    await supabase.auth.signOut();
    setUser(null);
  };

  // Automatically detect real network status
  useEffect(() => {
    const handleOnline = () => updateStatus("online");
    const handleOffline = () => updateStatus("offline");

    window.addEventListener("online", handleOnline);
    window.addEventListener("offline", handleOffline);

    return () => {
      window.removeEventListener("online", handleOnline);
      window.removeEventListener("offline", handleOffline);
    };
  }, []);

  return (
    <AuthContext.Provider
      value={{ user, logout, updateStatus, linkMicrosoft, unlinkMicrosoft, updateProfile }}
    >
      {children}
    </AuthContext.Provider>
  );
};

export const useAuth = () => {
  const context = useContext(AuthContext);
  if (context === undefined) {
    throw new Error("useAuth must be used within an AuthProvider");
  }
  return context;
};
