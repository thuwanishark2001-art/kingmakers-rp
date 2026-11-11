using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Kingmakers.EditorTools
{
    /// <summary>
    /// Command-line builds (used by CI):
    ///   -executeMethod Kingmakers.EditorTools.BuildScript.BuildAndroid
    ///   -executeMethod Kingmakers.EditorTools.BuildScript.BuildLinuxServer
    /// Optional: -customBuildPath &lt;file&gt; to choose the output file.
    /// Android signing uses the repository keystore (android-client/keystore) when present.
    /// </summary>
    public static class BuildScript
    {
        public static void BuildAndroid()
        {
            ProjectSetup.Run();
            EditorUserBuildSettings.buildAppBundle = false;

            var keystore = Path.GetFullPath(Path.Combine(Application.dataPath, "../../android-client/keystore/release.keystore"));
            if (File.Exists(keystore))
            {
                PlayerSettings.Android.useCustomKeystore = true;
                PlayerSettings.Android.keystoreName = keystore;
                PlayerSettings.Android.keystorePass = "kingmakers";
                PlayerSettings.Android.keyaliasName = "kingmakers";
                PlayerSettings.Android.keyaliasPass = "kingmakers";
            }

            Build(BuildTarget.Android, OutputPath("Builds/Android/KingmakersRP.apk"));
        }

        /// <summary>
        /// A normal Linux player; run it with -batchmode -nographics so it starts as the
        /// dedicated server (see GameConfig).
        /// </summary>
        public static void BuildLinuxServer()
        {
            ProjectSetup.Run();
            Build(BuildTarget.StandaloneLinux64, OutputPath("Builds/Server/KingsportServer.x86_64"));
        }

        static string OutputPath(string fallback)
        {
            var args = Environment.GetCommandLineArgs();
            for (int i = 0; i < args.Length - 1; i++)
                if (args[i] == "-customBuildPath") return args[i + 1];
            return fallback;
        }

        static void Build(BuildTarget target, string path)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path)));
            var options = new BuildPlayerOptions
            {
                scenes = EditorBuildSettings.scenes.Where(s => s.enabled).Select(s => s.path).ToArray(),
                target = target,
                targetGroup = BuildPipeline.GetBuildTargetGroup(target),
                locationPathName = path,
                options = BuildOptions.None,
            };
            var report = BuildPipeline.BuildPlayer(options);
            var summary = report.summary;
            Debug.Log($"[Build] {target}: {summary.result}, {summary.totalSize / (1024 * 1024)} MB, {summary.totalErrors} errors -> {path}");
            if (summary.result != BuildResult.Succeeded && Application.isBatchMode) EditorApplication.Exit(1);
        }
    }
}
