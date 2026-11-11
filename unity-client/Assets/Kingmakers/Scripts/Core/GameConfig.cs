using System;
using UnityEngine;

namespace Kingmakers
{
    /// <summary>
    /// Settings shared by client and server. Command line overrides (server):
    ///   -server              run as dedicated server (also implied by -batchmode)
    ///   -port 7778           UDP port to listen on
    ///   -backend URL         backend base URL
    ///   -serverKey KEY       or env GAME_SERVER_KEY: key for /api/v2/server/*
    ///   -offline             accept anyone as a guest (local testing only)
    /// </summary>
    public static class GameConfig
    {
        public const string Version = "0.1.0";
        public const string DefaultBackendUrl = "http://139.180.185.57:8080";
        public const ushort DefaultGamePort = 7778;
        public const int CitySeed = 20261110;
        public const int MaxPlayers = 200;

        public static bool IsDedicatedServer => Application.isBatchMode || HasArg("-server");
        public static bool OfflineMode { get; set; } = HasArg("-offline");

        public static string BackendUrl
        {
            get
            {
                var fromArgs = Arg("-backend");
                if (!string.IsNullOrEmpty(fromArgs)) return fromArgs.TrimEnd('/');
                var saved = PlayerPrefs.GetString("km.backend", "");
                return string.IsNullOrEmpty(saved) ? DefaultBackendUrl : saved.TrimEnd('/');
            }
        }

        public static ushort ServerPort =>
            ushort.TryParse(Arg("-port"), out var p) ? p : DefaultGamePort;

        public static string ServerKey =>
            Arg("-serverKey") ?? Environment.GetEnvironmentVariable("GAME_SERVER_KEY") ?? "";

        public static bool HasArg(string name)
        {
            foreach (var a in Environment.GetCommandLineArgs())
                if (string.Equals(a, name, StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }

        public static string Arg(string name)
        {
            var args = Environment.GetCommandLineArgs();
            for (int i = 0; i < args.Length - 1; i++)
                if (string.Equals(args[i], name, StringComparison.OrdinalIgnoreCase)) return args[i + 1];
            return null;
        }
    }
}
