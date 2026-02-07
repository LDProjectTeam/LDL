package com.ldp.coreutils;

import net.fabricmc.api.ModInitializer;
import net.fabricmc.loader.api.FabricLoader;

import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class CoreUtilsMod implements ModInitializer {
    private static final String AUTH_FILE = "ldp_auth.txt";

    @Override
    public void onInitialize() {
        try {
            Path gameDir = FabricLoader.getInstance().getGameDir();
            Path authPath = gameDir.resolve(AUTH_FILE);
            if (!Files.exists(authPath)) {
                fail("Authorization data not found.");
                return;
            }
            Map<String, String> data = parseKeyValues(Files.readAllLines(authPath, StandardCharsets.UTF_8));
            String launchToken = data.get("launchToken");
            String buildId = data.get("buildId");
            String api = data.get("api");
            String deviceId = data.get("deviceId");
            if (isBlank(launchToken) || isBlank(buildId) || isBlank(api) || isBlank(deviceId)) {
                fail("Authorization data is incomplete.");
                return;
            }

            String apiBase = api.trim();
            if (apiBase.endsWith("/")) {
                apiBase = apiBase.substring(0, apiBase.length() - 1);
            }
            String verifyUrl = apiBase + "/auth/verify-launch";
            String payload = "{\"buildId\":\"" + escapeJson(buildId.trim()) + "\"," +
                "\"deviceId\":\"" + escapeJson(deviceId.trim()) + "\"," +
                "\"token\":\"" + escapeJson(launchToken.trim()) + "\"}";

            HttpURLConnection conn = (HttpURLConnection) new URL(verifyUrl).openConnection();
            conn.setRequestMethod("POST");
            conn.setConnectTimeout(5000);
            conn.setReadTimeout(8000);
            conn.setDoOutput(true);
            conn.setRequestProperty("Content-Type", "application/json");

            try (OutputStream os = conn.getOutputStream()) {
                os.write(payload.getBytes(StandardCharsets.UTF_8));
            }

            int code = conn.getResponseCode();
            if (code != 200) {
                String detail = readBody(conn);
                if (detail == null || detail.isEmpty()) {
                    detail = "Access denied.";
                }
                fail(detail);
                return;
            }
        } catch (Exception ex) {
            fail("Access check failed.");
        }
    }

    private static Map<String, String> parseKeyValues(List<String> lines) {
        Map<String, String> map = new HashMap<>();
        for (String line : lines) {
            if (line == null) {
                continue;
            }
            String trimmed = line.trim();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                continue;
            }
            int idx = trimmed.indexOf('=');
            if (idx <= 0) {
                continue;
            }
            String key = trimmed.substring(0, idx).trim();
            String value = trimmed.substring(idx + 1).trim();
            if (!key.isEmpty()) {
                map.put(key, value);
            }
        }
        return map;
    }

    private static boolean isBlank(String value) {
        return value == null || value.trim().isEmpty();
    }

    private static String escapeJson(String value) {
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private static String readBody(HttpURLConnection conn) {
        InputStream stream = null;
        try {
            stream = conn.getErrorStream();
            if (stream == null) {
                stream = conn.getInputStream();
            }
            if (stream == null) {
                return "";
            }
            StringBuilder sb = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(stream, StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
            }
            return sb.toString();
        } catch (IOException ex) {
            return "";
        }
    }

    private static void fail(String message) {
        SwingUtilities.invokeLater(() -> {
            JOptionPane.showMessageDialog(null, message, "Launcher", JOptionPane.ERROR_MESSAGE);
        });
        System.exit(0);
    }
}
